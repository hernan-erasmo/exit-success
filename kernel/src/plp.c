#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#include "plp.h"

#define PACKAGESIZE 1024

int init_escucha_programas(int *listenningSocket, char *puerto, struct addrinfo **serverInfo, t_log *logger);

void *plp(void *datos_plp)
{
	//Variables de inicializacion
	char *puerto = ((t_datos_plp *) datos_plp)->puerto;
	t_log *logger = ((t_datos_plp *) datos_plp)->logger;

	//Variables de sockets
	struct addrinfo *serverInfo = NULL;
	int listenningSocket = -1;
	int socketCliente = -1;
	char package[PACKAGESIZE];
	int status = 1;		// Estructura que manjea el status de los recieve.
	struct sockaddr_in addr; // Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	socklen_t addrlen = sizeof(addr);	
	
	pthread_mutex_t init_plp = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t fin_plp = PTHREAD_MUTEX_INITIALIZER;

	//Meto en un mutex la inicialización del plp
	pthread_mutex_lock(&init_plp);
		log_info(logger, "[PLP] Inicializando el hilo PLP");

		if(init_escucha_programas(&listenningSocket, puerto, &serverInfo, logger) != 0){
			goto liberarRecursos;
			pthread_exit(NULL);
		}

		listen(listenningSocket, 10);
		
		log_info(logger, "[PLP] Esperando conexiones...");
		socketCliente = accept(listenningSocket, (struct sockaddr *) &addr, &addrlen);
		
		log_info(logger, "[PLP] Recibí una conexión!");

		while (status != 0){
			memset(package, 0, PACKAGESIZE);
			status = recv(socketCliente, (void*) package, PACKAGESIZE, 0);
			if (status > 0) 
				printf("%s", package);
			if (status < 0) {
				log_error(logger, "[PLP] Error en la transmisión. Motivo: %s", strerror(errno));
				goto liberarRecursos;
				pthread_exit(NULL);
			}
		}
	pthread_mutex_unlock(&init_plp);

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
		pthread_mutex_unlock(&fin_plp);
}

int init_escucha_programas(int *listenningSocket, char *puerto, struct addrinfo **serverInfo, t_log *logger)
{
	struct addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// No importa si uso IPv4 o IPv6
	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	// Notar que le pasamos NULL como IP, ya que le indicamos que use localhost en AI_PASSIVE
	if (getaddrinfo(NULL, puerto, &hints, serverInfo) != 0) {
		log_error(logger,"[PLP] No se pudo crear la estructura addrinfo. Motivo: %s", strerror(errno));
		return 1;
	}
	
	if ((*listenningSocket = socket((*serverInfo)->ai_family, (*serverInfo)->ai_socktype, (*serverInfo)->ai_protocol)) < 0) {
		log_error(logger, "[PLP] Error al crear socket para Programas. Motivo: %s", strerror(errno));
		return 1;
	}
	log_info(logger, "[PLP] Se creó el socket a la escucha del puerto: %s (Programas)", puerto);

	if(bind(*listenningSocket,(*serverInfo)->ai_addr, (*serverInfo)->ai_addrlen)) {
		log_error(logger, "[PLP] No se pudo bindear el socket para Programas a la dirección. Motivo: %s", strerror(errno));
		return 1;
	}

	return 0;
}