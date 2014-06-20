#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#include "plp.h"

static uint32_t contador_id_programa = 0;

void *plp(void *datos_plp)
{
	//Variables del select (copia descarada de la guía Beej)
	fd_set master;
	fd_set read_fds;
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	int sockActual;
	int newfd;
	int fdmax;

	//Variables de errores
	int errorConexion = 0;

	//Variables de inicializacion
	char *puerto_escucha = ((t_datos_plp *) datos_plp)->puerto_escucha;
	char *ip_umv = ((t_datos_plp *) datos_plp)->ip_umv;
	int puerto_umv = ((t_datos_plp *) datos_plp)->puerto_umv;
	uint32_t tamanio_stack = ((t_datos_plp *) datos_plp)->tamanio_stack;
	t_log *logger = ((t_datos_plp *) datos_plp)->logger;

	//Variables de sockets
	struct addrinfo *serverInfo = NULL;
	socket_umv = -1;
	int listenningSocket = -1;
	int socketCliente = -1;
	int status = 1;		// Estructura que manjea el status de los recieve.
	struct sockaddr_in addr; // Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	struct sockaddr_in dir_umv;
	socklen_t addrlen = sizeof(addr);	
	
	pthread_t thread_vaciar_exit;
	pthread_mutex_t init_plp = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t encolar = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t crear_programa_nuevo = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t fin_plp = PTHREAD_MUTEX_INITIALIZER;

	//Fin variables

	//Meto en un mutex la inicialización del plp
	pthread_mutex_lock(&init_plp);
		log_info(logger, "[PLP] Creando el socket que escucha conexiones de programas.");
		if(crear_conexion_entrante(&listenningSocket, puerto_escucha, &serverInfo, logger, "PLP") != 0){
			goto liberarRecursos;
			pthread_exit(NULL);
		}
		
		log_info(logger, "[PLP] Intentando conectar con la UMV...");
		errorConexion = crear_conexion_saliente(&socket_umv, &dir_umv, ip_umv, puerto_umv, logger, "PLP");
		if (errorConexion) {
			log_error(logger, "[PLP] Error al intentar conectar con la UMV");
			goto liberarRecursos;
			pthread_exit(NULL);
		}
		log_info(logger, "[PLP] Conexión establecida con la UMV!");

		log_info(logger, "[PLP] Estoy creando el semáforo para la cola de Exit");
		if(sem_init(&s_exit, 0, 0)){
			log_info(logger, "[PLP] No se pudo inicializar el semáforo para la cola Exit");
			goto liberarRecursos;
			pthread_exit(NULL);
		}
		log_info(logger, "[PLP] Se creó el semáforo para la cola de Exit.");

		log_info(logger, "[PLP] Estoy creando el worker thread para eliminar elementos de Exit");

		t_worker_th *wt = malloc(sizeof(t_worker_th));
			wt->logger = logger;
			wt->cola_exit = cola_exit;
		if(pthread_create(&thread_vaciar_exit, NULL, vaciar_exit, (void *) wt)) {
			log_error(logger, "[PLP] Error al crear el worker thread del PLP que elimina elementos de Exit. Motivo: %s", strerror(errno));
			goto liberarRecursos;
			pthread_exit(NULL);
		}

		log_info(logger, "[PLP] Se creó el worker thread para eliminar elementos de Exit");

		log_info(logger, "[PLP] Esperando conexiones de programas...");
	pthread_mutex_unlock(&init_plp);

	FD_SET(listenningSocket, &master);
	fdmax = listenningSocket;

	//bucle principal del PLP
	while(1){
		read_fds = master;
		log_info(logger, "[SELECT] Me bloqueé");
		if(select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1){
			log_error(logger, "[PLP] El select tiró un error, y la verdad que no sé que hacer. Sigo corriendo.");
			continue;
		}
		log_info(logger, "[SELECT] Salí del bloqueo");

		//Recorro tooooodos los descriptores que tengo
		for(sockActual = 0; sockActual <= fdmax; sockActual++){

			if(FD_ISSET(sockActual, &read_fds)){	//Si éste es el que está listo, entonces...
				
				//...me están avisando que se quiere conectar un programa que nunca se conectó todavía.
				if(sockActual == listenningSocket){
					log_info(logger, "[SELECT] Conexion nueva");		

					if((newfd = accept(listenningSocket, (struct sockaddr *) &addr, &addrlen)) != -1){
						FD_SET(newfd, &master);
						
						if(newfd > fdmax)
							fdmax = newfd;
					
						log_info(logger, "[PLP] Recibí una conexión! El socket es: %d", newfd);
					} else {
						log_error(logger, "[PLP] Error al aceptar una conexión entrante.");
					}

				} else {	//Ya tengo a este socket en mi lista de conexiones

					log_info(logger, "[SELECT] Conexión vieja");
					t_paquete_programa paquete;
					inicializar_paquete(&paquete);

					status = recvAll(&paquete, sockActual);
					if(status){
						switch(paquete.id)
						{
							case 'P':	//Asumo que todo lo que entre por acá es una solicitud de un programa nuevo.
						
								; //¿Por qué un statement vacío? ver http://goo.gl/6SwXRB
								t_pcb *pcb = malloc(sizeof(t_pcb));
								pcb->socket = sockActual;
								pcb->peso = 0;

								if(atender_solicitud_programa(socket_umv, &paquete, pcb, tamanio_stack, &crear_programa_nuevo, logger) != 0){
									log_error(logger, "[PLP] No se pudo satisfacer la solicitud del programa");
								
									pthread_mutex_lock(&encolar);
										list_add(cola_exit, pcb);
										sem_post(&s_exit);
									pthread_mutex_unlock(&encolar);
								} else {
									log_info(logger, "[PLP] Solicitud atendida satisfactoriamente.");
									
									//if (bloquearía ponerlo en ready)
										//lo pongo en new
										//list_sort(cola_new, ordenar_por_peso);
										//creo nuevo worker thread cuyo unico propósito sea quitar el primer elemento de new y ponerlo en ready
									//else
										pthread_mutex_lock(&encolar);
											list_add(cola_ready, pcb);
											//¿hay que modificar algún semáforo acá?
										pthread_mutex_unlock(&encolar);
														
									log_info(logger, "[PLP] Los siguientes procesos están en la cola New:");
									list_iterate(cola_new, mostrar_datos_cola);
								}
								break;

							case 'U':
								log_info(logger, "[PLP] Se recibió un mensaje de la UMV.");
								break;
							default:
								log_info(logger, "[PLP] No es una conexión de un programa");
						}
					} else {
						log_info(logger, "[PLP] El socket %d cerró su conexión y ya no está en mi lista de sockets.", sockActual);
						close(sockActual);
						FD_CLR(sockActual, &master);
					}

					if(paquete.mensaje)
						free(paquete.mensaje);	
				}
			}
		}
	}

	goto liberarRecursos;
	pthread_exit(NULL);

	liberarRecursos:
		pthread_mutex_lock(&fin_plp);

			if(serverInfo != NULL)
				freeaddrinfo(serverInfo);

			if(listenningSocket != -1)
				close(listenningSocket);

			if(socketCliente != -1)
				close(socketCliente);

			sem_destroy(&s_exit);
			
		pthread_mutex_unlock(&fin_plp);
}

int atender_solicitud_programa(int socket_umv, t_paquete_programa *paquete, t_pcb *pcb, uint32_t tamanio_stack, pthread_mutex_t *mutex, t_log *logger)
{
	log_info(logger, "[PLP] Un programa envió un mensaje");

	int i, huboUnError = 0;
	uint32_t base_segmento = 0;
	t_metadata_program *metadatos = NULL;

	contador_id_programa++;
	pcb->id = contador_id_programa;
	
	pthread_mutex_lock(mutex);
	
		while(1){

			if(solicitar_cambiar_proceso_activo(socket_umv, contador_id_programa, 'P', logger) == 0){
				huboUnError = 1;
				break;
			}

		
			if(crear_segmento_codigo(socket_umv, pcb, paquete, contador_id_programa, logger) == 0){
				huboUnError = 1;
				break;
			}

			if(crear_segmento_stack(socket_umv, pcb, tamanio_stack, contador_id_programa, logger) == 0){
				huboUnError = 1;
				break;
			}

			metadatos = metadata_desde_literal(paquete->mensaje);

			if(crear_segmento_indice_codigo(socket_umv, pcb, metadatos, contador_id_programa, logger) == 0){
				huboUnError = 1;
				break;
			}

			if(crear_segmento_etiquetas(socket_umv, pcb, metadatos, contador_id_programa, logger) == 0){
				huboUnError = 1;
				break;
			}
			
			break;
		}
	
	pthread_mutex_unlock(mutex);

	if(!huboUnError)
		calcularPeso(pcb, metadatos);
	else
		contador_id_programa--;

	if(metadatos)
		metadata_destruir(metadatos);

	return huboUnError;
}

uint32_t crear_segmento_codigo(int socket_umv, t_pcb *pcb, t_paquete_programa *paquete, uint32_t contador_id_programa, t_log *logger)
{
	int resultado = 0;
	int bytesEscritos = 0;

	if((pcb->seg_cod = solicitar_crear_segmento(socket_umv, contador_id_programa, strlen(paquete->mensaje), 'P', logger)) == 0){
		log_error(logger, "[PLP] La UMV no pudo crear el segmento de código para este programa. Se aborta su creación.");
		return resultado;
	}

	if(solicitar_enviar_bytes(socket_umv, pcb->seg_cod, 0, strlen(paquete->mensaje), paquete->mensaje, 'P', logger) == 0){
		log_error(logger, "[PLP] La UMV no permitió escribir el código en el segmento de código del programa.");
		return resultado;
	}

	resultado = 1;
	return resultado;
}

uint32_t crear_segmento_stack(int socket_umv, t_pcb *pcb, uint32_t tamanio_stack, uint32_t contador_id_programa, t_log *logger)
{
	int resultado = 0;

	if((pcb->seg_stack = solicitar_crear_segmento(socket_umv, contador_id_programa, tamanio_stack, 'P', logger)) == 0){
		log_error(logger, "[PLP] La UMV no pudo crear el segmento de stack para este programa. Se aborta su creación.");
		return resultado;
	}

	resultado = 1;
	return resultado;
}

uint32_t crear_segmento_indice_codigo(int socket_umv, t_pcb *pcb, t_metadata_program *metadatos, uint32_t contador_id_programa, t_log *logger)
{
	int i, resultado = 0;

	uint32_t tamanio_indice_codigo = metadatos->instrucciones_size * 8;
	if((pcb->seg_idx_cod = solicitar_crear_segmento(socket_umv, contador_id_programa, tamanio_indice_codigo, 'P', logger)) == 0){
		log_error(logger, "[PLP] La UMV no pudo crear el segmento para el índice de código de este programa. Se aborta su creación.");
		return resultado;
	}

	t_intructions *t = metadatos->instrucciones_serializado;

	uint32_t st;
	uint32_t off;
	
	for(i = 0; i < metadatos->instrucciones_size; i++){
		st = t[i].start;
		off = t[i].offset;

		if(solicitar_enviar_bytes(socket_umv, pcb->seg_idx_cod, i*8, sizeof(uint32_t), &st, 'P', logger) == 0){
			log_error(logger, "[PLP] La UMV no permitió escribir el índice de código en el segmento de índice de código del programa.");
			return resultado;
		}

		if(solicitar_enviar_bytes(socket_umv, pcb->seg_idx_cod, (i*8 + 4), sizeof(uint32_t), &off, 'P', logger) == 0){
			log_error(logger, "[PLP] La UMV no permitió escribir el índice de código en el segmento de índice de código del programa.");
			return resultado;
		}		
	}

	resultado = 1;
	return resultado;
}

uint32_t crear_segmento_etiquetas(int socket_umv, t_pcb *pcb, t_metadata_program *metadatos, uint32_t contador_id_programa, t_log *logger)
{
	int resultado = 0;

	uint32_t tamanio_indice_etiquetas = metadatos->etiquetas_size;

	if(tamanio_indice_etiquetas == 0){
		resultado = 1;
		return resultado;
	}

	if((pcb->seg_idx_etq = solicitar_crear_segmento(socket_umv, contador_id_programa, tamanio_indice_etiquetas, 'P', logger)) == 0){
		log_error(logger, "[PLP] La UMV no pudo crear el segmento para el índice de etiquetas de este programa. Se aborta su creación.");
		return resultado;
	}

	if(solicitar_enviar_bytes(socket_umv, pcb->seg_idx_etq, 0, metadatos->etiquetas_size, metadatos->etiquetas, 'P', logger) == 0)
	{
		log_error(logger, "[PLP] La UMV no permitió escribir el serializado de etiquetas para este programa. Se aborta su creación.");
		return resultado;
	}

	resultado = 1;
	return resultado;
}

void calcularPeso(t_pcb *pcb, t_metadata_program *metadatos)
{
	int cant_etiquetas = metadatos->etiquetas_size;
	int cant_funciones = metadatos->cantidad_de_funciones;
	int tot_lineas_cod = metadatos->instrucciones_size;

	pcb->peso = (5 * cant_etiquetas) + (3 * cant_funciones) + tot_lineas_cod;

	return;
}

bool ordenar_por_peso(void *a, void *b)
{
	t_pcb *pcb_a = (t_pcb *) a;
	t_pcb *pcb_b = (t_pcb *) b;

	return (pcb_a->peso < pcb_b->peso);
}

void mostrar_datos_cola(void *item)
{
	t_pcb *pcb = (t_pcb *) item;
	printf("ID del Programa: %d, Peso: %d\n", pcb->id, pcb->peso);

	return;
}
