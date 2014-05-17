#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#include "plp.h"
#include "../../utils/comunicacion.h"

#define PACKAGESIZE 1024

int init_escucha_programas(int *listenningSocket, char *puerto, struct addrinfo **serverInfo, t_log *logger);
int recvAll(t_paquete_programa *paquete, int sock);

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
		log_info(logger, "[PLP] Creando el socket que escucha conexiones de programas.");
		if(init_escucha_programas(&listenningSocket, puerto, &serverInfo, logger) != 0){
			goto liberarRecursos;
			pthread_exit(NULL);
		}
		
		log_info(logger, "[PLP] Esperando conexiones de programas...");
	pthread_mutex_unlock(&init_plp);

	socketCliente = accept(listenningSocket, (struct sockaddr *) &addr, &addrlen);
	log_info(logger, "[PLP] Recibí una conexión!");

	t_paquete_programa paquete;

	while(status){
		//status = receive_and_deserialize(&paquete, socketCliente); //seguí acá con el ejemplo de DynamicSerialization del lado del server en el ejemplo de sockets de sisoputnfrba
		status = recvAll(&paquete, socketCliente);
		if(status){
			switch(paquete.id)
			{
				case 'P':
					/*
					log_info(logger, "[PLP] Un programa se conectó. Va a enviar %d bytes de datos.", paquete.sizeMensaje);
					log_info(logger, "[PLP] Dice:");
					printf("%s", paquete.mensaje);
					log_info(logger, "[PLP] Se recibieron %d bytes.", status);
					*/
					log_info(logger, "[PLP] Soy un programa, porque mi identificador es: %c", paquete.id);
					log_info(logger, "[PLP] El mensaje que traigo tiene un tamaño de %d bytes.", paquete.sizeMensaje);
					log_info(logger, "[PLP] El mensaje que traigo es:");
					printf("%s", paquete.mensaje);
					status = 0;
					break;
				default:
					log_info(logger, "[PLP] No es una conexión de un programa");
			}
		}
	}

	/*
	while (status){
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
	*/
	
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

	listen(*listenningSocket, 10);

	return 0;
}

int recvAll(t_paquete_programa *paquete, int sock)
{
	int status = 0;
	int buffer_size;
	char *buffer = malloc(buffer_size = sizeof(uint32_t));

	char id;
	status = recv(sock, &(paquete->id), sizeof(paquete->id), 0);	//recibe sólo un byte, el char de 'id'
	if(!status) return 0;

	uint32_t long_mensaje;
	status = recv(sock, &(paquete->sizeMensaje), sizeof(paquete->sizeMensaje), 0);
	if(!status) return 0;

	paquete->mensaje = calloc(paquete->sizeMensaje + 1, 1);
	status = recv(sock, paquete->mensaje, paquete->sizeMensaje, 0);
	if(!status) return 0;

	/*
	**
	*/

	free(buffer);

	return status;
}