#include <netinet/in.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <commons/log.h>

#ifndef COMUNICACION_H
#define COMUNICACION_H

typedef struct paquete_programa {
	char id;
	char *mensaje;
	uint32_t sizeMensaje;
	uint32_t tamanio_total;
} t_paquete_programa;

int sendAll(int sock, char *buf, int *len);
int recvAll(t_paquete_programa *paquete, int sock);
void inicializar_paquete(t_paquete_programa *paq);
char *serializar_paquete(t_paquete_programa *paquete, t_log *logger);

//Sockets
int crear_conexion_saliente(int *unSocket, struct sockaddr_in *socketInfo, char *ip, int puerto, t_log *logger, char *id_proceso);
int crear_conexion_entrante(int *listenningSocket, char *puerto, struct addrinfo **serverInfo, t_log *logger, char *id_proceso);
int crear_socket(struct sockaddr_in *socketInfo, char *ip, int puerto);

#endif /* COMUNICACION_H */
