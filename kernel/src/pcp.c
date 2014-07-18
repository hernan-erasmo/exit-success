#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "pcp.h"

static uint32_t finalizar = 0;

void *pcp(void *datos_pcp)
{
	//Variables del select (copia descarada de la guía Beej)
	fd_set master;
	fd_set read_fds;
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	int sockActual;
	int newfd;
	int fdmax;

	//Variables de sockets
	int socket_umv = -1;
	struct addrinfo *serverInfo = NULL;
	int listenningSocket = -1;
	int status = 1;		// Estructura que manjea el status de los recieve.
	struct sockaddr_in addr; // Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	socklen_t addrlen = sizeof(addr);

	t_list *cpus_ociosas = NULL;

	//Variables de inicializacion
	t_log *logger = ((t_datos_pcp *) datos_pcp)->logger;
	char *puerto_escucha_cpu = ((t_datos_pcp *) datos_pcp)->puerto_escucha_cpu;

	pthread_t thread_dispatcher;
	pthread_mutex_t init_pcp = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t fin_pcp = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t encolar = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&init_pcp);
		log_info(logger, "[PCP] Creando el socket que escucha conexiones de CPUs.");
		if(crear_conexion_entrante(&listenningSocket, puerto_escucha_cpu, &serverInfo, logger, "PCP") != 0){
			goto liberarRecursos;
			pthread_exit(NULL);
		}		

		log_info(logger, "[PCP] Estoy escuchando conexiones de CPUs en el puerto %s", puerto_escucha_cpu);

		int valor_s_hay_cpus = 0;

		log_info(logger, "[PCP] Estoy creando el semáforo para la cola de Ready");
		if(sem_init(&s_ready_max, 0, multiprogramacion) || sem_init(&s_ready_nuevo, 0, 0) || sem_init(&s_hay_cpus, 0, 0)){
			log_info(logger, "[PCP] No se pudo inicializar un semáforo.");
			goto liberarRecursos;
			pthread_exit(NULL);
		}
		
		sem_getvalue(&s_hay_cpus, &valor_s_hay_cpus);
		log_info(logger, "[PCP] El valor del semáforo de las cpus libres es %d", valor_s_hay_cpus);
		log_info(logger, "[PCP] Se creó el semáforo para la cola de ready, con un valor de %d", multiprogramacion);

		log_info(logger, "[PCP] Creando el hilo Dispatcher, que va a manejar las colas de ready, block y exec.");
		cpus_ociosas = list_create();
		t_init_dispatcher *disp = malloc(sizeof(t_init_dispatcher));
			disp->logger = logger;
			disp->cpus_ociosas = cpus_ociosas;
		if(pthread_create(&thread_dispatcher, NULL, dispatcher, (void *) disp)) {
			log_error(logger, "[PLP] Error al crear el hilo dispatcher. Motivo: %s", strerror(errno));
			goto liberarRecursos;
			pthread_exit(NULL);
		}
		pthread_detach(thread_dispatcher);

	pthread_mutex_unlock(&init_pcp);

	FD_SET(listenningSocket, &master);
	fdmax = listenningSocket;

	//Bucle principal del PCP
	while(1){
		read_fds = master;
		log_info(logger, "[PCP] Estoy bloqueado en el select.");
		if(select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1){
			log_error(logger, "[PCP] El select tiró un error, y la verdad que no sé que hacer. Sigo corriendo.");
			continue;
		}
		log_info(logger, "[PCP] Me desbloqueé del select.");
		
		//Recorro tooooodos los descriptores que tengo
		for(sockActual = 0; sockActual <= fdmax; sockActual++){

			if(FD_ISSET(sockActual, &read_fds)){	//Si éste es el que está listo, entonces...
				
				//...me están avisando que se quiere conectar una cpu que nunca se conectó todavía.
				if(sockActual == listenningSocket){
					log_info(logger, "[PCP] Conexion nueva");		

					if((newfd = accept(listenningSocket, (struct sockaddr *) &addr, &addrlen)) != -1){
						FD_SET(newfd, &master);
						
						if(newfd > fdmax)
							fdmax = newfd;
					
						log_info(logger, "[PCP] Recibí una conexión! El socket es: %d", newfd);
					} else {
						log_error(logger, "[PCP] Error al aceptar una conexión entrante.");
					}

				} else {	//Ya tengo a este socket en mi lista de conexiones

					log_info(logger, "[PCP] Conexión vieja");
					
					t_paquete_programa mensaje_cpu;
					t_header_cpu *head_cpu;
					status = recvAll(&mensaje_cpu, sockActual);
					t_pcb *pcb_modificado = NULL;
					int tamanio_ready = 0;

					if(status){
						switch(mensaje_cpu.id)		//Reciclo el campo id de paquete_programa y lo uso para saber qué quiere hacer la CPU
						{						//No hace falta filtrar por identificador de programa. Acá sólo se conectan las CPUs.
							case 'O':	//La cpu dice que está ociosa
								; //¿Por qué un statement vacío? ver http://goo.gl/6SwXRB
								
								printf("La CPU con el socket %d se agrega a la cola de CPUs\n", sockActual);

								head_cpu = malloc(sizeof(t_header_cpu));
									head_cpu->socket = sockActual;
									head_cpu->estado = mensaje_cpu.id;	//Me parece que es medio al pedo esto, pero bueno, seguime la corriente.

								//Agrego la cpu a la lista de ociosas
								list_add(cpus_ociosas, head_cpu);
								log_info(logger, "[PCP] Hay una CPU libre, su socket es: %d.", head_cpu->socket);
								log_info(logger, "[PCP] La lista de CPUs ociosas tiene un tamaño de %d", list_size(cpus_ociosas));

								//mostrar_cola_ready(logger);

								//Informo al thread que despacha que hay una cpu disponible
								sem_post(&s_hay_cpus);
								sem_getvalue(&s_hay_cpus, &valor_s_hay_cpus);
								log_info(logger, "[PCP] El valor del semáforo de las cpus libres es %d", valor_s_hay_cpus);

								break;
							case 'S':	//La CPU quiere ejecutar una syscall
								;
									char *syscall_serializada = NULL;
									char *pcb_serializado = NULL;
									int esBloqueante = 0;
									int status_op = -1;
									void *respuesta_op = NULL;

									esBloqueante = extraerComandoYPcb(mensaje_cpu.mensaje, &syscall_serializada, &pcb_serializado);
									log_info(logger, "[PCP] El syscall que me solicitaron es: %s", syscall_serializada);

									if(esBloqueante){
										if(esBloqueante == -1){
											log_error(logger, "[PCP] extraerComandoYPcb no reconoció ningún comando en el mensaje.");
											break;											
										}

										pcb_modificado = malloc(sizeof(t_pcb));
										deserializarPcb(pcb_modificado, (void *) pcb_serializado);
										log_info(logger, "[PCP] El PCB del proceso que lo solicitó tiene los siguientes datos:");
										log_info(logger, "\tID:%d\n\tSocket:%d\n\tQuantum:%d\n\tPeso:%d\n\tSeg_cod:%d\n\tSeg_stack:%d\n\t...",pcb_modificado->id,pcb_modificado->socket,pcb_modificado->quantum,pcb_modificado->peso,pcb_modificado->seg_cod,pcb_modificado->seg_stack);	
									
										ejecutarSyscall(syscall_serializada,pcb_modificado,&status_op, &respuesta_op, sockActual, logger);
										
										if(status_op < 0){
											log_error(logger, "[PCP] Hubo un error al ejecutar la syscall. Me lavo las manos olímpicamente, eh!");
										}

									} else {	// NO es bloqueante, la cpu está esperando una respuesta YA!

										if(esBloqueante == -1){
											log_error(logger, "[PCP] extraerComandoYPcb no reconoció ningún comando en el mensaje.");
											break;											
										}

										int status_op = -1;
										ejecutarSyscall(syscall_serializada, NULL, &status_op, &respuesta_op, sockActual, logger);
									}

								break;
							case 'Q':	//Terminó la ráfaga de CPU, por quantum, y me está mandando el PCB para guardar
								;

								tamanio_ready = 0;
								pcb_modificado = malloc(sizeof(t_pcb));
								deserializarPcb(pcb_modificado, (void *) mensaje_cpu.mensaje);

								log_info(logger, "[PCP] Una CPU me mandó el PCB del proceso %d que terminó su quantum. (program counter=%d)", pcb_modificado->id, pcb_modificado->p_counter);
								pthread_mutex_lock(&encolar);
									tamanio_ready = list_size(cola_ready);
									list_add_in_index(cola_ready, tamanio_ready, pcb_modificado);	//Lo mando al fondo de la cola, es Round-Robin.
									sem_post(&s_ready_nuevo);
								pthread_mutex_unlock(&encolar);

								break;
							case 'F':	//La CPU informa que llegó al final de la ejecución de este proceso.
								;
								//Informamos a quien haya que informar (Como el programa, por ejemplo, leemos el stack y retornamos
								// la cantidad de variables y el valor de cada una. Soñar es gratis.)
								pcb_modificado = malloc(sizeof(t_pcb));
								deserializarPcb(pcb_modificado, (void *) mensaje_cpu.mensaje);

								enviarMensajePrograma(&(pcb_modificado->socket), "FINALIZAR", "Terminó la ejecución.\n");

								log_info(logger, "[PCP] Una CPU me mandó el PCB del proceso %d que terminó su ejecución. (program counter=%d)", pcb_modificado->id, pcb_modificado->p_counter);
								pthread_mutex_lock(&encolar);
									list_add(cola_exit, pcb_modificado);
									sem_post(&s_ready_max);
									sem_post(&s_exit);
								pthread_mutex_unlock(&encolar);

								break;
							case 'X':	//La CPU me avisa que se va por SIGUSR1, asi que ya no la considero más. Acaba de finalizar su quantum.
								;
									tamanio_ready = 0;
									pcb_modificado = malloc(sizeof(t_pcb));
									deserializarPcb(pcb_modificado, (void *) mensaje_cpu.mensaje);

									log_info(logger, "[PCP] Una CPU me mandó el PCB de su último proceso (id: %d) antes de salir por SIGUSR1. (program counter=%d)", pcb_modificado->id, pcb_modificado->p_counter);
									pthread_mutex_lock(&encolar);
										tamanio_ready = list_size(cola_ready);
										list_add_in_index(cola_ready, tamanio_ready, pcb_modificado);	//Lo mando al fondo de la cola, es Round-Robin.
										sem_post(&s_ready_nuevo);
									pthread_mutex_unlock(&encolar);
								
								break;
							case 'E':	//La CPU me avisa que finalizó por un error en la ejecución del script. (stack overflow, seg. fault, etc)
								;
								pcb_modificado = malloc(sizeof(t_pcb));
								deserializarPcb(pcb_modificado, (void *) mensaje_cpu.mensaje);

								enviarMensajePrograma(&(pcb_modificado->socket), "FINALIZAR", "La ejecución terminó de forma inesperada debido a un error. Revisar logs para más información.\n");

								log_info(logger, "[PCP] Una CPU me mandó el PCB del proceso %d que terminó su ejecución. (program counter=%d)", pcb_modificado->id, pcb_modificado->p_counter);
								pthread_mutex_lock(&encolar);
									list_add(cola_exit, pcb_modificado);
									sem_post(&s_ready_max);
									sem_post(&s_exit);
								pthread_mutex_unlock(&encolar);

								break;
							default:
								log_info(logger, "[PCP] No entiendo lo que me quiere decir una CPU");
						}
					} else {
						log_info(logger, "[PCP] El socket %d cerró su conexión y ya no está en mi lista de sockets.", sockActual);
						log_info(logger, "[PCP] Saco a la CPU con el socket %d de mi lista de CPUs.", sockActual);
						quitar_cpu(cpus_ociosas, sockActual);
						sem_getvalue(&s_hay_cpus, &valor_s_hay_cpus);
						log_info(logger, "[PCP] El valor del semáforo de las cpus libres antes del trywait %d", valor_s_hay_cpus);
						sem_trywait(&s_hay_cpus);
						sem_getvalue(&s_hay_cpus, &valor_s_hay_cpus);
						log_info(logger, "[PCP] El valor del semáforo de las cpus libres después del trywait %d", valor_s_hay_cpus);
						close(sockActual);
						FD_CLR(sockActual, &master);
					}
				}
			}
		}
	}

	pthread_exit(NULL);

	liberarRecursos:
		pthread_mutex_lock(&fin_pcp);
			sem_destroy(&s_ready_nuevo);
			sem_destroy(&s_ready_max);

			if(cpus_ociosas)
				list_destroy(cpus_ociosas);

			if(serverInfo != NULL)
				freeaddrinfo(serverInfo);

			if(listenningSocket != -1)
				close(listenningSocket);

		pthread_mutex_unlock(&fin_pcp);
}

int extraerComandoYPcb(char *mensaje, char **syscall_serializada, char **pcb_serializado)
{
	char *saveptr = NULL;
	int offset = 0;
	int limiteEntreComandoYPcb = 0;

	*syscall_serializada = strdup(strtok_r(mensaje, "|", &saveptr));
	limiteEntreComandoYPcb = strlen(*syscall_serializada);

	if(strstr(*syscall_serializada, "entradaSalida") != NULL){	//ES BLOQUEANTE (entradaSalida)
		*pcb_serializado = calloc(48, 1);
		memcpy(*pcb_serializado, mensaje + limiteEntreComandoYPcb + 1, 48);
		return 1;
	}
	
	//Esta tiene que estar antes que "wait". EL ORDEN IMPORTA. Si, ya lo sé. No, no me da vergüenza.
	if(memcmp(*syscall_serializada,"wait?",5) == 0){	//NO ES BLOQUEANTE (notar el signo de pregunta al final)
		*pcb_serializado = NULL;
		return 0;
	}

	//Esta tiene que estar después que "wait?". EL ORDEN IMPORTA. Si, ya lo sé. No, no me da vergüenza.
	if(memcmp(*syscall_serializada,"wait",4) == 0){	//ES BLOQUEANTE (wait)
		*pcb_serializado = calloc(48, 1);
		memcpy(*pcb_serializado, mensaje + limiteEntreComandoYPcb + 1, 48);
		return 1;
	}
	

	if(strstr(*syscall_serializada,"obtenerValorCompartida") != NULL){	//NO ES BLOQUEANTE (obtenerValorCompartida)
		*pcb_serializado = NULL;
		return 0;
	}
	
	if(strstr(*syscall_serializada,"asignarValorCompartida") != NULL){	//NO ES BLOQUEANTE (asignarValorCompartida)
		*pcb_serializado = NULL;
		return 0;
	}


	if(memcmp(*syscall_serializada,"imprimirTexto",13) == 0){	//NO ES BLOQUEANTE
		*pcb_serializado = NULL;
		return 0;
	}
	
	if(memcmp(*syscall_serializada,"imprimir",8) == 0){			//NO ES BLOQUEANTE
		*pcb_serializado = NULL;
		return 0;
	}	
	
	if(strstr(*syscall_serializada,"signal") != NULL){	//NO ES BLOQUEANTE (signal)
		*pcb_serializado = NULL;
		return 0;
	}	
	
	return -1;
}

void ejecutarSyscall(char *syscall_completa, t_pcb *pcb_a_atender, int *status_op, void **respuesta_op, int socket_respuesta, t_log *logger)
{
	char *saveptr;
	char *nombre_syscall = strtok_r(syscall_completa, ",", &saveptr);

	if(strcmp("entradaSalida", nombre_syscall) == 0){
		char *nombre_dispositivo = strtok_r(NULL, ",", &saveptr);
		char *tiempoEnUnidades = strtok_r(NULL, ",", &saveptr);
		log_info(logger, "[PCP] Voy a ejecutar la syscall %s (dispositivo: %s, tiempoEnUnidades: %s)", nombre_syscall, nombre_dispositivo, tiempoEnUnidades);
		*status_op = syscall_entradaSalida(nombre_dispositivo, pcb_a_atender, atoi(tiempoEnUnidades), logger);
	}

	if(strcmp("obtenerValorCompartida", nombre_syscall) == 0){
		char *nombre_compartida = strtok_r(NULL, ",", &saveptr);
		log_info(logger, "[PCP] Voy a ejecutar la syscall %s (variable: \'%s\')", nombre_syscall, nombre_compartida);
		*status_op = syscall_obtenerValorCompartida(nombre_compartida, socket_respuesta, logger);
	}

	if(strcmp("asignarValorCompartida", nombre_syscall) == 0){
		char *nombre_compartida = strtok_r(NULL, ",", &saveptr);
		char *valor_compartida = strtok_r(NULL, ",", &saveptr);
		log_info(logger, "[PCP] Voy a ejecutar la syscall %s (variable: \'%s\', valor: %d)", nombre_syscall, nombre_compartida, atoi(valor_compartida));
		*status_op = syscall_asignarValorCompartida(nombre_compartida, socket_respuesta, atoi(valor_compartida), logger);
	}

	if(strcmp("wait?", nombre_syscall) == 0){
		char *nombre_semaforo = strtok_r(NULL, ",", &saveptr);
		log_info(logger, "[PCP] Voy a ejecutar la syscall %s (nombre_semaforo: \'%s\')", nombre_syscall, nombre_semaforo);
		*status_op = syscall_wait(nombre_semaforo, NULL, socket_respuesta, logger);
	}

	if(strcmp("wait", nombre_syscall) == 0){
		char *nombre_semaforo = strtok_r(NULL, ",", &saveptr);
		log_info(logger, "[PCP] Voy a ejecutar la syscall %s (nombre_semaforo: \'%s\')", nombre_syscall, nombre_semaforo);
		*status_op = syscall_wait(nombre_semaforo, pcb_a_atender, socket_respuesta, logger);
	}

	if(strcmp("signal", nombre_syscall) == 0){
		char *nombre_semaforo = strtok_r(NULL, ",", &saveptr);
		log_info(logger, "[PCP] Voy a ejecutar la syscall %s (nombre_semaforo: \'%s\')", nombre_syscall, nombre_semaforo);
		*status_op = syscall_signal(nombre_semaforo, logger);
	}

	if(strcmp("imprimir", nombre_syscall) == 0){
		char *str_socket = strtok_r(NULL, ",", &saveptr);
		char *str_valor = strtok_r(NULL, ",", &saveptr);
		*status_op = syscall_imprimir(atoi(str_socket), str_valor);
	}

	if(strcmp("imprimirTexto", nombre_syscall) == 0){
		char *str_socket = strtok_r(NULL, ",", &saveptr);
		char *str_valor = strtok_r(NULL, ",", &saveptr);
		*status_op = syscall_imprimir(atoi(str_socket), str_valor);
	}

	//agregar acá nuevas syscalls

	free(syscall_completa);
	return;
}

void mostrar_cola_ready(t_log *logger)
{
	int i, cant_elementos = 0;
	cant_elementos = list_size(cola_ready);
	t_pcb *p = NULL;

	pthread_mutex_t listarDeCorrido = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&listarDeCorrido);
		log_info(logger, "[COLA_READY] La cola de ready tiene %d PCBs:", cant_elementos);
		for(i = 0; i < cant_elementos; i++){
			p = list_get(cola_ready, i);
			log_info(logger, "\tID: %d, Peso: %d", p->id, p->peso);
		}
	pthread_mutex_unlock(&listarDeCorrido);

	return;
}

void quitar_cpu(t_list *cpus_ociosas, int socket_buscado)
{
	int i, cant_headers = list_size(cpus_ociosas);
	t_header_cpu *header = NULL;
	int encontado = 0;

	for(i = 0; i < cant_headers; i++){
		header = list_get(cpus_ociosas, i);

		if(header->socket == socket_buscado){
			encontado = 1;
			break;
		}
	}

	if(encontado){
		t_header_cpu *header_a_eliminar = list_remove(cpus_ociosas, i);
	}

	return;
}
