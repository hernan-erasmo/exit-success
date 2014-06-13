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
	int socket_umv = -1;
	int listenningSocket = -1;
	int socketCliente = -1;
	int status = 1;		// Estructura que manjea el status de los recieve.
	struct sockaddr_in addr; // Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	struct sockaddr_in dir_umv;
	socklen_t addrlen = sizeof(addr);	
	
	pthread_mutex_t init_plp = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t fin_plp = PTHREAD_MUTEX_INITIALIZER;

	t_list *cola_new = list_create();
	t_list *cola_exit = list_create();

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

								if(atender_solicitud_programa(socket_umv, &paquete, pcb, tamanio_stack, logger) != 0){
									log_error(logger, "[PLP] No se pudo satisfacer la solicitud del programa");

								/*
								**	mando el pcb a la cola de exit, y que ahí se encarge de liberar toda la memoria
								**	(la que alocó el pcb por su cuenta, y los segmentos que están en la umv. Que invoque
								**	funciones de la umv si es necesario). La cola de exit se encarga de liberar los segmentos.
								*/
									list_add(cola_exit, pcb);
								} else {
									log_info(logger, "[PLP] Solicitud atendida satisfactoriamente.");
									list_add(cola_new, pcb);

									list_sort(cola_new, ordenar_por_peso);

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
			if(list_is_empty(cola_new))
				list_destroy(cola_new);

			if(list_is_empty(cola_exit))
				list_destroy(cola_exit);

			if(serverInfo != NULL)
				freeaddrinfo(serverInfo);

			if(listenningSocket != -1)
				close(listenningSocket);

			if(socketCliente != -1)
				close(socketCliente);

		pthread_mutex_unlock(&fin_plp);
}

int atender_solicitud_programa(int socket_umv, t_paquete_programa *paquete, t_pcb *pcb, uint32_t tamanio_stack, t_log *logger)
{
	log_info(logger, "[PLP] Un programa envió un mensaje");

	int i, huboUnError = 0;
	uint32_t base_segmento = 0;
	t_metadata_program *metadatos = NULL;

	contador_id_programa++;
	pcb->id = contador_id_programa;
	
	while(1){
		if(solicitar_cambiar_proceso_activo(socket_umv, contador_id_programa, logger) == 0){
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

	if(!huboUnError)
		calcularPeso(pcb, metadatos);
	else
		contador_id_programa--;

	if(metadatos)
		metadata_destruir(metadatos);

	return huboUnError;
}

uint32_t solicitar_cambiar_proceso_activo(int socket_umv, uint32_t contador_id_programa, t_log *logger)
{
	uint32_t bEnv = 0;
	char *orden = codificar_cambiar_proceso_activo(contador_id_programa);
	uint32_t valorRetorno = 0;	

	t_paquete_programa paq_saliente;
		paq_saliente.id = 'P';
		paq_saliente.mensaje = orden;
		paq_saliente.sizeMensaje = strlen(orden);

	char *paqueteSaliente = serializar_paquete(&paq_saliente, logger);
	
	bEnv = paq_saliente.tamanio_total;
	if(sendAll(socket_umv, paqueteSaliente, &bEnv)){
		log_error(logger, "[PLP] Error en la solicitud de cambio de proceso activo. Motivo: %s", strerror(errno));
		free(paqueteSaliente);
		free(orden);
		return 0;
	}

	free(paqueteSaliente);
	free(orden);

	uint32_t bRec = 0;
	t_paquete_programa respuesta;
	log_info(logger, "[PLP] Esperando la respuesta de la UMV.");
	bRec = recvAll(&respuesta, socket_umv);
	log_info(logger, "[PLP] La UMV respondió %s", respuesta.mensaje);

	valorRetorno = atoi(respuesta.mensaje);
	free(respuesta.mensaje);

	return valorRetorno;	
}

char *codificar_cambiar_proceso_activo(uint32_t contador_id_programa)
{
	int offset = 0;
	
	char proc_activo[5];
	sprintf(proc_activo, "%d", contador_id_programa);
	char *comando = "cambiar_proceso_activo";

	int proc_activo_len = strlen(proc_activo);
	int comando_len = strlen(comando);

	//longitud del comando + el nulo de fin de cadena + la coma que separa + longitud de proc_activo
	char *orden_completa = calloc(comando_len + 1 + 1 + proc_activo_len, 1);
	memcpy(orden_completa + offset, comando, comando_len);
	offset += comando_len;

	memcpy(orden_completa + offset, ",", 1);
	offset += 1;	

	memcpy(orden_completa + offset, proc_activo, proc_activo_len);
	
	return orden_completa;	
}

uint32_t crear_segmento_codigo(int socket_umv, t_pcb *pcb, t_paquete_programa *paquete, uint32_t contador_id_programa, t_log *logger)
{
	int resultado = 0;
	int bytesEscritos = 0;

	if((pcb->seg_cod = solicitar_crear_segmento(socket_umv, contador_id_programa, strlen(paquete->mensaje), logger)) == 0){
		log_error(logger, "[PLP] La UMV no pudo crear el segmento de código para este programa. Se aborta su creación.");
		return resultado;
	}

	if(solicitar_enviar_bytes(socket_umv, pcb->seg_cod, 0, strlen(paquete->mensaje), paquete->mensaje, logger) == 0){
		log_error(logger, "[PLP] La UMV no permitió escribir el código en el segmento de código del programa.");
		return resultado;
	}

	resultado = 1;
	return resultado;
}

uint32_t crear_segmento_stack(int socket_umv, t_pcb *pcb, uint32_t tamanio_stack, uint32_t contador_id_programa, t_log *logger)
{
	int resultado = 0;

	if((pcb->seg_stack = solicitar_crear_segmento(socket_umv, contador_id_programa, tamanio_stack, logger)) == 0){
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
	if((pcb->seg_idx_cod = solicitar_crear_segmento(socket_umv, contador_id_programa, tamanio_indice_codigo, logger)) == 0){
		log_error(logger, "[PLP] La UMV no pudo crear el segmento para el índice de código de este programa. Se aborta su creación.");
		return resultado;
	}

	t_intructions *t = metadatos->instrucciones_serializado;

	uint32_t st;
	uint32_t off;
	
	for(i = 0; i < metadatos->instrucciones_size; i++){
		st = t[i].start;
		off = t[i].offset;

		if(solicitar_enviar_bytes(socket_umv, pcb->seg_idx_cod, i*8, sizeof(uint32_t), &st, logger) == 0){
			log_error(logger, "[PLP] La UMV no permitió escribir el índice de código en el segmento de índice de código del programa.");
			return resultado;
		}

		if(solicitar_enviar_bytes(socket_umv, pcb->seg_idx_cod, (i*8 + 4), sizeof(uint32_t), &off, logger) == 0){
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

	if((pcb->seg_idx_etq = solicitar_crear_segmento(socket_umv, contador_id_programa, tamanio_indice_etiquetas, logger)) == 0){
		log_error(logger, "[PLP] La UMV no pudo crear el segmento para el índice de etiquetas de este programa. Se aborta su creación.");
		return resultado;
	}

	//escribir datos aca

	resultado = 1;
	return resultado;
}

uint32_t solicitar_crear_segmento(int socket_umv, uint32_t id_programa, uint32_t tamanio_segmento, t_log *logger)
{
	uint32_t bEnv = 0;
	char *orden = codificar_crear_segmento(id_programa, tamanio_segmento);
	uint32_t valorRetorno = 0;

	t_paquete_programa paq_saliente;
		paq_saliente.id = 'P';
		paq_saliente.mensaje = orden;
		paq_saliente.sizeMensaje = strlen(orden);
	
	char *paqueteSaliente = serializar_paquete(&paq_saliente, logger);
	
	bEnv = paq_saliente.tamanio_total;
	if(sendAll(socket_umv, paqueteSaliente, &bEnv)){
		log_error(logger, "[PLP] Error en la solicitud de creación de segmento. Motivo: %s", strerror(errno));
		free(paqueteSaliente);
		free(orden);
		return 0;
	}

	free(paqueteSaliente);
	free(orden);

	uint32_t bRec = 0;
	t_paquete_programa respuesta;
	log_info(logger, "[PLP] Esperando la respuesta de la UMV.");
	bRec = recvAll(&respuesta, socket_umv);
	log_info(logger, "[PLP] La UMV respondió %s", respuesta.mensaje);

	valorRetorno = atoi(respuesta.mensaje);
	free(respuesta.mensaje);

	return valorRetorno;
}

uint32_t solicitar_enviar_bytes(int socket_umv, uint32_t base, uint32_t offset, int tamanio, void *buffer, t_log *logger)
{
	uint32_t bEnv = 0;
	char *orden = codificar_enviar_bytes(base,offset,tamanio,buffer);
	uint32_t valorRetorno = 0;

	t_paquete_programa paq_saliente;
		paq_saliente.id = 'P';
		paq_saliente.mensaje = orden;
		paq_saliente.sizeMensaje = strlen(orden);
	
	char *paqueteSaliente = serializar_paquete(&paq_saliente, logger);

	bEnv = paq_saliente.tamanio_total;
	if(sendAll(socket_umv, paqueteSaliente, &bEnv)){
		log_error(logger, "[PLP] Error en la solicitud de enviar bytes para el segmento de código. Motivo: %s", strerror(errno));
		free(paqueteSaliente);
		free(orden);
		return 0;
	}

	free(paqueteSaliente);
	free(orden);

	uint32_t bRec = 0;
	t_paquete_programa respuesta;
	log_info(logger, "[PLP] Esperando la respuesta de la UMV.");
	bRec = recvAll(&respuesta, socket_umv);
	log_info(logger, "[PLP] La UMV respondió %s", respuesta.mensaje);

	valorRetorno = atoi(respuesta.mensaje);
	free(respuesta.mensaje);

	return valorRetorno;
}

char *codificar_enviar_bytes(uint32_t base, uint32_t offset, int tamanio, void *buffer)
{
	int offset_codificacion = 0;

	char str_base[10];
	sprintf(str_base, "%d", base);
	char str_offset[10];
	sprintf(str_offset, "%d", offset);
	char str_tamanio[10];
	sprintf(str_tamanio, "%d", tamanio);
	char *comando = "enviar_bytes";

	int base_len = strlen(str_base);
	int offset_len = strlen(str_offset);
	int tamanio_len = strlen(str_tamanio);
	int comando_len = strlen(comando);
	int buffer_len = tamanio;

	char *orden_completa = calloc(comando_len + 1 + 1 + base_len + 1 + offset_len + 1 + tamanio_len + 1 + buffer_len, 1);
	memcpy(orden_completa + offset_codificacion, comando, comando_len);
	offset_codificacion += comando_len;
	memcpy(orden_completa + offset_codificacion, ",", 1);
	offset_codificacion += 1;

	memcpy(orden_completa + offset_codificacion, str_base, base_len);
	offset_codificacion += base_len;
	memcpy(orden_completa + offset_codificacion, ",", 1);
	offset_codificacion += 1;

	memcpy(orden_completa + offset_codificacion, str_offset, offset_len);
	offset_codificacion += offset_len;
	memcpy(orden_completa + offset_codificacion, ",", 1);
	offset_codificacion += 1;

	memcpy(orden_completa + offset_codificacion, str_tamanio, tamanio_len);
	offset_codificacion += tamanio_len;
	memcpy(orden_completa + offset_codificacion, ",", 1);
	offset_codificacion += 1;

	memcpy(orden_completa + offset_codificacion, buffer, buffer_len);
	offset_codificacion += buffer_len;
	
	return orden_completa;
}

char *codificar_crear_segmento(uint32_t id_programa, uint32_t tamanio)
{
	int offset = 0;
	
	char id[5];
	sprintf(id, "%d", id_programa);
	char tam[10];
	sprintf(tam, "%d", tamanio);
	char *comando = "crear_segmento";

	int id_len = strlen(id);
	int tam_len = strlen(tam);
	int comando_len = strlen(comando);

	//longitud del comando + el nulo de fin de cadena + la coma que separa + longitud de tamanio + la coma + longitud de id
	char *orden_completa = calloc(comando_len + 1 + 1 + tam_len + 1 + id_len, 1);
	memcpy(orden_completa + offset, comando, comando_len);
	offset += comando_len;

	memcpy(orden_completa + offset, ",", 1);
	offset += 1;	

	memcpy(orden_completa + offset, id, id_len);
	offset += id_len;

	memcpy(orden_completa + offset, ",", 1);
	offset += 1;

	memcpy(orden_completa + offset, tam, tam_len);

	return orden_completa;
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
