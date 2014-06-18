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

	//Variables de inicializacion
	t_log *logger = ((t_datos_pcp *) datos_pcp)->logger;
	char *puerto_escucha_cpu = ((t_datos_pcp *) datos_pcp)->puerto_escucha_cpu;

	pthread_mutex_t init_pcp = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t fin_pcp = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&init_pcp);
		log_info(logger, "[PLP] Creando el socket que escucha conexiones de CPUs.");
		if(crear_conexion_entrante(&listenningSocket, puerto_escucha_cpu, &serverInfo, logger, "PCP") != 0){
			goto liberarRecursos;
			pthread_exit(NULL);
		}		

	log_info(logger, "[PCP] Estoy escuchando conexiones de CPUs en el puerto %s", puerto_escucha_cpu);

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
					t_paquete_programa paquete;
					inicializar_paquete(&paquete);

					status = recvAll(&paquete, sockActual);
					if(status){
						switch(paquete.id)
						{
							case 'C':	//Asumo que todo lo que entre por acá es una solicitud de una cpu nueva.
						
								; //¿Por qué un statement vacío? ver http://goo.gl/6SwXRB
								
								/*
								**	Acá adentro se hace todo lo que se debería hacer si una cpu que 
								**	ya está conectada te envía un mensaje.
								*/

								break;
							default:
								log_info(logger, "[PCP] No es una conexión de una CPU");
						}
					} else {
						log_info(logger, "[PCP] El socket %d cerró su conexión y ya no está en mi lista de sockets.", sockActual);
						close(sockActual);
						FD_CLR(sockActual, &master);
					}

					if(paquete.mensaje)
						free(paquete.mensaje);	
				}
			}
		}
	}

	pthread_exit(NULL);

	liberarRecursos:
		pthread_mutex_lock(&fin_pcp);

			if(serverInfo != NULL)
				freeaddrinfo(serverInfo);

			if(listenningSocket != -1)
				close(listenningSocket);

		pthread_mutex_unlock(&fin_pcp);
}
