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
	int errorHandshake = 0;

	//Variables de inicializacion
	char *puerto_escucha = ((t_datos_plp *) datos_plp)->puerto_escucha;
	char *ip_umv = ((t_datos_plp *) datos_plp)->ip_umv;
	int puerto_umv = ((t_datos_plp *) datos_plp)->puerto_umv;
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

	log_info(logger, "[PLP] Intentando conectar con la UMV...");
	errorConexion = crear_conexion_saliente(&socket_umv, &dir_umv, ip_umv, puerto_umv, logger, "PLP");
	if (errorConexion) {
		log_error(logger, "[PLP] Error al intentar conectar con la UMV");
		goto liberarRecursos;
		pthread_exit(NULL);
	}
	log_info(logger, "[PLP] Conexión establecida con la UMV!");

	errorHandshake = enviar_handshake(socket_umv,logger);
	if (errorHandshake) {
		log_error(logger, "[PLP] Error al enviar el handshake UMV.");
		goto liberarRecursos;
		pthread_exit(NULL);
	}

	//Meto en un mutex la inicialización del plp
	pthread_mutex_lock(&init_plp);
		log_info(logger, "[PLP] Creando el socket que escucha conexiones de programas.");
		if(init_escucha_programas(&listenningSocket, puerto_escucha, &serverInfo, logger) != 0){
			goto liberarRecursos;
			pthread_exit(NULL);
		}
		
		log_info(logger, "[PLP] Esperando conexiones de programas...");
	pthread_mutex_unlock(&init_plp);

	//bucle principal del PLP
	while(1){
		socketCliente = accept(listenningSocket, (struct sockaddr *) &addr, &addrlen);
		log_info(logger, "[PLP] Recibí una conexión!");

		status = recvAll(&paquete, socketCliente);
		if(status){
			switch(paquete.id)
			{
				case 'P':
					log_info(logger, "[PLP] Un programa se conectó. Va a enviar %d bytes de datos.", paquete.tamanio_total);
					log_info(logger, "[PLP] Se recibieron %d bytes.", status);
					printf("%s", paquete.mensaje);
					break;
				case 'U':
					log_info(logger, "[PLP] Se recibió una conexión de la UMV.");
					break;
				default:
					log_info(logger, "[PLP] No es una conexión de un programa");
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

			if(paquete.mensaje)
				free(paquete.mensaje);
		pthread_mutex_unlock(&fin_plp);
}

int init_escucha_programas(int *listenningSocket, char *puerto_escucha, struct addrinfo **serverInfo, t_log *logger)
{
	struct addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// No importa si uso IPv4 o IPv6
	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	// Notar que le pasamos NULL como IP, ya que le indicamos que use localhost en AI_PASSIVE
	if (getaddrinfo(NULL, puerto_escucha, &hints, serverInfo) != 0) {
		log_error(logger,"[PLP] No se pudo crear la estructura addrinfo. Motivo: %s", strerror(errno));
		return 1;
	}
	
	if ((*listenningSocket = socket((*serverInfo)->ai_family, (*serverInfo)->ai_socktype, (*serverInfo)->ai_protocol)) < 0) {
		log_error(logger, "[PLP] Error al crear socket para Programas. Motivo: %s", strerror(errno));
		return 1;
	}
	log_info(logger, "[PLP] Se creó el socket a la escucha del puerto_escucha: %s (Programas)", puerto_escucha);

	if(bind(*listenningSocket,(*serverInfo)->ai_addr, (*serverInfo)->ai_addrlen)) {
		log_error(logger, "[PLP] No se pudo bindear el socket para Programas a la dirección. Motivo: %s", strerror(errno));
		return 1;
	}

	listen(*listenningSocket, 10);

	return 0;
}

void crear_paquete_handshake(t_paquete_programa *paq)
{
	paq->id = 'K';
	paq->sizeMensaje = 0;
	paq->tamanio_total = 1 + sizeof(paq->sizeMensaje) + sizeof(paq->tamanio_total);

	return;
}

int enviar_handshake(int unSocket, t_log *logger)
{
	uint32_t bEnv = 0;
	char *contenidoScript = "handshake";
	char *paqueteSerializado = NULL;
	uint32_t sizeMensaje = 0;

	t_paquete_programa paquete;
		sizeMensaje = strlen(contenidoScript);
		//contenidoScript = calloc(sizeMensaje + 1, sizeof(char)); //sizeMensaje + 1, el contenido del script mas el '\0'

	paquete.id = 'K';
	paquete.sizeMensaje = sizeMensaje;
	paquete.mensaje = contenidoScript;
	paquete.tamanio_total = 1 + sizeof(paquete.sizeMensaje) + sizeMensaje + sizeof(paquete.tamanio_total);
	
	paqueteSerializado = serializar_paquete(&paquete, logger);

	bEnv = paquete.tamanio_total;
	if(sendAll(unSocket, paqueteSerializado, &bEnv)){
		log_error(logger, "[PLP] Error en la transmisión del handshake. Motivo: %s", strerror(errno));
		return 1;
	}

	log_info(logger, "[PLP] Handshake finalizado. Enviados %d bytes.", bEnv);

	free(paqueteSerializado);

	return 0;
}
