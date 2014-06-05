#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#include "plp.h"

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
	uint32_t id_programa = 1;

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

					if((newfd = accept(listenningSocket, (struct sockaddr *) &addr, &addrlen)) == -1){
						log_error(logger, "[PLP] Error al aceptar una conexión entrante.");
					} else {
						//creo un pcb con un nuevo id de programa, lo encolo en new y se lo paso a la
						//rutina de atención de solicitudes de programas para que lo modifique
						FD_SET(newfd, &master);
						
						if(newfd > fdmax)
							fdmax = newfd;
					
						log_info(logger, "[PLP] Recibí una conexión! El socket es: %d", newfd);
					}
					
				} else {	//Ya tengo a este socket en mi lista de conexiones
					log_info(logger, "[SELECT] Conexión vieja");
					t_paquete_programa paquete;
					inicializar_paquete(&paquete);

					status = recvAll(&paquete, sockActual);
					if(status){
						switch(paquete.id)
						{
							case 'P':	//Llegó una solicitud de un programa. Solicito a la UMV que cree los segmentos necesarios.
								if(atender_solicitud_programa(socket_umv, &paquete, NULL, logger) == 0){
									log_error(logger, "[PLP] No se pudo satisfacer la solicitud del programa");
								} else {
									log_info(logger, "[PLP] Solicitud atendida satisfactoriamente.");
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
		


		/*
		**	Acá habría que tener un select, entonces se haría algo así como
		**		if(la conexion viene de un socket que ya tengo en mi lista)
		**			busco en la cola de new al pcb cuyo socket es igual al que me está hablando,
		**			y envío ese pcb a la rutina de atención de solicitudes de programas
		**		else
		**			creo un pcb con un nuevo id de programa, lo encolo en new y se lo paso a la
		**			rutina de atención de solicitudes de programas para que lo modifique
		**
		**
		**		if(la rutina de atención de solicitudes de programas retornó error)
		**			mando el pcb a la cola de exit, y que ahí se encarge de liberar toda la memoria
		**			(la que alocó el pcb por su cuenta, y los segmentos que están en la umv. Que invoque
		**			funciones de la umv si es necesario). La cola de exit se encarga de liberar los segmentos.
		**
		*/ 

		//close(newfd);
		//free(newfd);
	}

	goto liberarRecursos;
	pthread_exit(NULL);

	liberarRecursos:
		pthread_mutex_lock(&fin_plp);
			if(list_is_empty(cola_new))
				list_destroy(cola_new);

			if(serverInfo != NULL)
				freeaddrinfo(serverInfo);

			if(listenningSocket != -1)
				close(listenningSocket);

			if(socketCliente != -1)
				close(socketCliente);

		pthread_mutex_unlock(&fin_plp);
}

int atender_solicitud_programa(int socket_umv, t_paquete_programa *paquete, t_pcb *pcb, t_log *logger)
{
	uint32_t base_segmento = 0;

	log_info(logger, "[PLP] Un programa envió un mensaje");

	/*
	**	Extraigo los metadatos usando el mensaje que viene en el paquete y llamo a solicitar_crear_segmento
	**	cuantas veces sea necesario.
	*/

	base_segmento = solicitar_crear_segmento(socket_umv, 1, 999, logger);
	if(base_segmento == 0){
		log_error(logger, "[PLP] La UMV no pudo crear un segmento necesario para este programa. Se aborta su creación.");
	} else {
		log_info(logger, "[PLP] La UMV creó un segmento con base %d", base_segmento);
	}

	return base_segmento;
}

uint32_t solicitar_crear_segmento(int socket_umv, uint32_t id_programa, uint32_t tamanio_segmento, t_log *logger)
{
	uint32_t bEnv = 0;
	char *orden = codificar_crear_segmento(id_programa, tamanio_segmento);
	
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
		return 1;
	}

	free(paqueteSaliente);
	free(orden);

	uint32_t bRec = 0;
	t_paquete_programa respuesta;
	log_info(logger, "[PLP] Esperando la respuesta de la UMV.");
	bRec = recvAll(&respuesta, socket_umv);
	log_info(logger, "[PLP] La UMV respondió %s", respuesta.mensaje);
	
	return atoi(respuesta.mensaje);
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
