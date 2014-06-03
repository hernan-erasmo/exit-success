#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#include "plp.h"

void *plp(void *datos_plp)
{
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
	
	t_paquete_programa paquete;
	inicializar_paquete(&paquete);

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

	uint32_t base_segmento = 0;

	//bucle principal del PLP
	while(1){
		int *sc = malloc(sizeof(int));
		if((*sc = accept(listenningSocket, (struct sockaddr *) &addr, &addrlen)) == -1){
			log_error(logger, "[PLP] Error al aceptar una conexión entrante.");
			continue;
		}
		//socketCliente = accept(listenningSocket, (struct sockaddr *) &addr, &addrlen);
		log_info(logger, "[PLP] Recibí una conexión!");

		//status = recvAll(&paquete, socketCliente);
		status = recvAll(&paquete, *sc);
		if(status){
			switch(paquete.id)
			{
				case 'P':
					/*
					**	Llegó una solicitud de un programa. Solicito a la UMV que cree los segmentos necesarios.
					*/
					log_info(logger, "[PLP] Un programa se conectó. Va a enviar %d bytes de datos.", paquete.tamanio_total);
					log_info(logger, "[PLP] Se recibieron %d bytes.", status);

					base_segmento = solicitar_crear_segmento(socket_umv, id_programa, 999, logger);
					if(base_segmento == 0){
						log_error(logger, "[PLP] La UMV no pudo crear un segmento necesario para este programa. Se aborta su creación.");
						//hay que destruir los segmentos que ya haya creado!!!!!!!!!1
						break;
					} else {
						log_info(logger, "[PLP] La UMV creó un segmento con base %d", base_segmento);
					}

					/*
					**	Acá se crea el PCB y se envía a la
					** 	UMV las solicitudes para crear los segmentos del programa nuevo.
					*/
					
					break;
				case 'U':
					log_info(logger, "[PLP] Se recibió un mensaje de la UMV.");
					break;
				default:
					log_info(logger, "[PLP] No es una conexión de un programa");
			}
		}

		close(*sc);
		free(sc);
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

			if(paquete.mensaje)
				free(paquete.mensaje);
		pthread_mutex_unlock(&fin_plp);
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
	printf("benv: %d\n", bEnv);
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
