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

		log_info(logger, "[PCP] Estoy creando el semáforo para la cola de Ready");
		if(sem_init(&s_ready_max, 0, multiprogramacion) || sem_init(&s_ready_nuevo, 0, 0) || sem_init(&s_hay_cpus, 0, 0)){
			log_info(logger, "[PCP] No se pudo inicializar un semáforo.");
			goto liberarRecursos;
			pthread_exit(NULL);
		}
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
					t_pcb *pcb_modificado = malloc(sizeof(t_pcb));

					if(status){
						switch(mensaje_cpu.id)		//Reciclo el campo id de paquete_programa y lo uso para saber qué quiere hacer la CPU
						{						//No hace falta filtrar por identificador de programa. Acá sólo se conectan las CPUs.
							case 'O':	//La cpu dice que está ociosa
								; //¿Por qué un statement vacío? ver http://goo.gl/6SwXRB
								
								head_cpu = malloc(sizeof(t_header_cpu));
									head_cpu->socket = sockActual;
									head_cpu->estado = mensaje_cpu.id;	//Me parece que es medio al pedo esto, pero bueno, seguime la corriente.

								//Agrego la cpu a la lista de ociosas
								list_add(cpus_ociosas, head_cpu);

								//Informo al thread que despacha que hay una cpu disponible
								sem_post(&s_hay_cpus);

								break;
							case 'T':	//La cpu dice que está trabajando, quiere solicitar un servicio.
								;

								/*
								*	Acá tendré que manejar una interfaz de atención de serivicios, parsear el campo 'operacion' del header.
								*	Un pijazo.
								*/
								break;
							case 'P':	//Terminó la ráfaga de CPU (por quantum, o por block) y me está mandando el PCB para guardar
								;
								int tamanio_ready = 0;
								deserializarPcb(pcb_modificado, (void *) mensaje_cpu.mensaje);

								log_info(logger, "[PCP] Una CPU me mandó el PCB del proceso: %d. (program counter=%d)", pcb_modificado->id, pcb_modificado->p_counter);
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
								deserializarPcb(pcb_modificado, (void *) mensaje_cpu.mensaje);

								enviarMensajePrograma(&(pcb_modificado->socket), "FINALIZAR", "Terminó la ejecución.\n");

								log_info(logger, "[PCP] Una CPU me mandó el PCB del proceso: %d. (program counter=%d)", pcb_modificado->id, pcb_modificado->p_counter);
								pthread_mutex_lock(&encolar);
									list_add(cola_exit, pcb_modificado);
									sem_post(&s_ready_max);
									sem_post(&s_exit);
								pthread_mutex_unlock(&encolar);

								break;
							case 'X':	//La CPU me avisa que se va, asi que ya no la considero más.
								;

								/*
								*	Acá hago lo que tenga que hacer en este caso. Ahora no se me ocurre nada que tenga que hacer.
								*	sem_wait(&s_hay_cpus);
								*/

							default:
								log_info(logger, "[PCP] No entiendo lo que me quiere decir una CPU");
						}
					} else {
						log_info(logger, "[PCP] El socket %d cerró su conexión y ya no está en mi lista de sockets.", sockActual);
						close(sockActual);
						FD_CLR(sockActual, &master);
					}

					if(mensaje_cpu.mensaje)
						free(mensaje_cpu.mensaje);
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
