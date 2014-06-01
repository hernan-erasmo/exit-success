#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>

#include "comunicacion.h"

/*
**	Copia descarada de la guía Beej.
*/
int sendAll(int s, char *buf, int *len)
{
	int total = 0;
	int bytesLeft = *len;
	int n;

	while(total < *len){
		n = send(s, buf+total, bytesLeft, 0);
		if (n == -1) {break;}
		total += n;
		bytesLeft -= n;
	}

	*len = total;

	return n == -1 ? -1 : 0;
}

/*
**	Recibe todos los datos de un paquete.
*/
int recvAll(t_paquete_programa *paquete, int sock)
{
	int status = 0;
	uint32_t recibido = 0;
	uint32_t bytesRecibidos = 0;
	uint32_t bytesARecibir = 0;
	uint32_t offset = 0;
	char *buffer = NULL;
	paquete->mensaje = NULL;

	uint32_t long_paquete;
	status = recv(sock, &(paquete->tamanio_total), sizeof(paquete->tamanio_total), 0);
	if(status < 0) {return status;}

	bytesARecibir = paquete->tamanio_total - sizeof(paquete->tamanio_total);	//El tamaño del paquete quitando el tamaño total
	buffer = calloc(bytesARecibir, 1);

	while(bytesARecibir > 0){
		recibido = recv(sock, buffer, bytesARecibir, 0);
		bytesRecibidos += recibido;
		bytesARecibir -= recibido;
	}

	//Cargamos el id del programa
	char id;
	memcpy(&(paquete->id), buffer + offset, sizeof(paquete->id));
	offset += sizeof(paquete->id);

	//Cargamos el tamaño del mensaje
	uint32_t long_mensaje;
	memcpy(&(paquete->sizeMensaje), buffer + offset, sizeof(paquete->sizeMensaje));
	offset += sizeof(paquete->sizeMensaje);

	//Cargamos el mensaje
	paquete->mensaje = calloc(paquete->sizeMensaje + 1, 1);
	memcpy(paquete->mensaje, buffer + offset, paquete->sizeMensaje);

	free(buffer);

	return bytesRecibidos;
}

void inicializar_paquete(t_paquete_programa *paq)
{
	paq->id = 'X';
	paq->mensaje = NULL;
	paq->sizeMensaje = 0;
	//paq->tamanio_total = 1 + sizeof(paquete->sizeMensaje) + sizeMensaje + sizeof(paquete->tamanio_total);
	paq->tamanio_total = 0;
	
	return;
}

char *serializar_paquete(t_paquete_programa *paquete, t_log *logger)
{
	char *serializedPackage = calloc(1 + sizeof(paquete->sizeMensaje) + paquete->sizeMensaje + sizeof(paquete->tamanio_total), sizeof(char)); //El tamaño del char id, el tamaño de la variable sizeMensaje, el tamaño del script y el tamaño de la variable tamaño_total
	//char *serializedPackage = calloc(1 + sizeof(paquete->sizeMensaje) + paquete->sizeMensaje, sizeof(char)); //El tamaño del char id, el tamaño de la variable sizeMensaje, el tamaño del script y el tamaño de la variable tamaño_total
	uint32_t offset = 0;
	uint32_t sizeToSend;

	sizeToSend = sizeof(paquete->tamanio_total);
	log_info(logger, "sizeof(paquete->tamanio_total = %d", sizeof(paquete->tamanio_total));
	memcpy(serializedPackage + offset, &(paquete->tamanio_total), sizeToSend);
	offset += sizeToSend;

	sizeToSend = sizeof(paquete->id);
	log_info(logger, "sizeof(paquete->id) = %d", sizeof(paquete->id));
	memcpy(serializedPackage + offset, &(paquete->id), sizeToSend);
	offset += sizeToSend;

	sizeToSend = sizeof(paquete->sizeMensaje);
	log_info(logger, "sizeof(paquete->sizeMensaje) = %d", sizeof(paquete->sizeMensaje));
	memcpy(serializedPackage + offset, &(paquete->sizeMensaje), sizeToSend);
	offset += sizeToSend;

	sizeToSend = paquete->sizeMensaje;
	log_info(logger, "paquete->size = %d", paquete->sizeMensaje);
	memcpy(serializedPackage + offset, paquete->mensaje, sizeToSend);

	return serializedPackage;
}

int crear_conexion_saliente(int *unSocket, struct sockaddr_in *socketInfo, char *ip, int puerto, t_log *logger, char* id_proceso)
{
	if ((*unSocket = crear_socket(socketInfo, ip, puerto)) < 0) {
		log_error(logger, "[%s] Error al crear socket. Motivo: %s", id_proceso, strerror(errno));
		return 1;
	}

	if (connect(*unSocket, (struct sockaddr *) socketInfo, sizeof(*socketInfo)) != 0) {
		log_error(logger, "[%s] Error al conectar socket. Motivo: %s", id_proceso, strerror(errno));
		return 1;
	}

	return 0;
}

int crear_socket(struct sockaddr_in *socketInfo, char *ip, int puerto)
{
	int sock = -1;
		
	if((sock = socket(AF_INET,SOCK_STREAM,0)) >= 0) {
		socketInfo->sin_family = AF_INET;
		socketInfo->sin_addr.s_addr = inet_addr(ip);
		socketInfo->sin_port = htons(puerto);
	}

	return sock;		
}