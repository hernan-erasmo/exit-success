#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>

#include "comunicacion.h"

#define SIZEOF_PCB 48

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
	if(status <= 0 || paquete->tamanio_total <= 0) {return 0;}	//Para que carajo quiero seguir sufriendo, si yo mismo espero cero bytes.

	bytesARecibir = paquete->tamanio_total - sizeof(paquete->tamanio_total);	//El tamaño del paquete quitando el tamaño total
	printf("Espero recibir %d bytes por el socket %d.\n", bytesARecibir, sock);
	buffer = calloc(bytesARecibir, 1);

	while(bytesARecibir > 0){
		recibido = recv(sock, buffer, bytesARecibir, 0);
		bytesRecibidos += recibido;
		bytesARecibir -= recibido;
	}
	printf("Recibí %d bytes por el socket %d.\n", bytesRecibidos, sock);

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

int recvPcb(t_pcb *pcb, int sock)
{
	uint32_t recibido = 0;
	uint32_t bytesRecibidos = 0;
	uint32_t bytesARecibir = SIZEOF_PCB;	//(En el PCB, 12 campos * 4 bytes cada uno)
	void *buffer = NULL;

	buffer = calloc(bytesARecibir, 1);

	while(bytesARecibir > 0){
		recibido = recv(sock, buffer, bytesARecibir, 0);
		bytesRecibidos += recibido;
		bytesARecibir -= recibido;
	}
	printf("Recibí %d bytes por el socket %d.\n", bytesRecibidos, sock);

	deserializarPcb(pcb, buffer);

	return bytesRecibidos;
}

void deserializarPcb(t_pcb *pcb, void *buffer)
{
	uint32_t offset = 0;

	//Cargamos el id del programa en el pcb
	uint32_t id;
	memcpy(&(pcb->id), buffer + offset, 4);
	offset += 4;

	int socket;
	memcpy(&(pcb->socket), buffer + offset, sizeof(int));
	offset += sizeof(int);

	uint32_t quantum;
	memcpy(&(pcb->quantum), buffer + offset, 4);
	offset += 4;	
	
	uint32_t peso;
	memcpy(&(pcb->peso), buffer + offset, 4);
	offset += 4;

	uint32_t seg_cod;
	memcpy(&(pcb->seg_cod), buffer + offset, 4);	
	offset += 4;

	uint32_t seg_stack;
	memcpy(&(pcb->seg_stack), buffer + offset, 4);
	offset += 4;

	uint32_t cursor_stack;
	memcpy(&(pcb->cursor_stack), buffer + offset, 4);
	offset += 4;

	uint32_t seg_idx_cod;
	memcpy(&(pcb->seg_idx_cod), buffer + offset, 4);
	offset += 4;

	uint32_t seg_idx_etq;
	memcpy(&(pcb->seg_idx_etq), buffer + offset, 4);
	offset += 4;

	uint32_t p_counter;
	memcpy(&(pcb->p_counter), buffer + offset, 4);
	offset += 4;

	uint32_t size_ctxt_actual;
	memcpy(&(pcb->size_ctxt_actual), buffer + offset, 4);
	offset += 4;

	uint32_t size_idx_etq;
	memcpy(&(pcb->size_idx_etq), buffer + offset, 4);

	free(buffer);

	return;
}

void inicializar_paquete(t_paquete_programa *paq)
{
	paq->id = 'X';
	paq->mensaje = NULL;
	paq->sizeMensaje = 0;
	paq->tamanio_total = 0;
	
	return;
}

char *serializar_paquete(t_paquete_programa *paquete, t_log *logger)
{
	int tamanio_total = 1 + sizeof(paquete->sizeMensaje) + paquete->sizeMensaje + sizeof(paquete->tamanio_total);

	char *serializedPackage = calloc(tamanio_total, 1); //El tamaño del char id, el tamaño de la variable sizeMensaje, el tamaño del script y el tamaño de la variable tamaño_total
	uint32_t offset = 0;
	uint32_t sizeToSend;

	paquete->tamanio_total = tamanio_total;

	sizeToSend = sizeof(paquete->tamanio_total);
	memcpy(serializedPackage + offset, &(paquete->tamanio_total), sizeToSend);
	offset += sizeToSend;

	sizeToSend = sizeof(paquete->id);
	memcpy(serializedPackage + offset, &(paquete->id), sizeToSend);
	offset += sizeToSend;

	sizeToSend = sizeof(paquete->sizeMensaje);
	memcpy(serializedPackage + offset, &(paquete->sizeMensaje), sizeToSend);
	offset += sizeToSend;

	sizeToSend = paquete->sizeMensaje;
	
	if(sizeToSend > 0)
		memcpy(serializedPackage + offset, paquete->mensaje, sizeToSend);

	return serializedPackage;
}

char *serializar_pcb(t_pcb *pcb, t_log *logger)
{
	int tamanio_total = SIZEOF_PCB;

	void *serializedPackage = calloc(tamanio_total, 1);
	uint32_t offset = 0;
	uint32_t sizeToSend;

	sizeToSend = 4;
	memcpy(serializedPackage + offset, &(pcb->id), 4);
	offset += sizeToSend;

	sizeToSend = sizeof(int);
	memcpy(serializedPackage + offset, &(pcb->socket), sizeof(int));
	offset += sizeToSend;

	sizeToSend = 4;
	memcpy(serializedPackage + offset, &(pcb->quantum), 4);
	offset += sizeToSend;
	
	sizeToSend = 4;
	memcpy(serializedPackage + offset, &(pcb->peso), 4);
	offset += sizeToSend;

	sizeToSend = 4;
	memcpy(serializedPackage + offset, &(pcb->seg_cod), 4);
	offset += sizeToSend;

	sizeToSend = 4;
	memcpy(serializedPackage + offset, &(pcb->seg_stack), 4);
	offset += sizeToSend;

	sizeToSend = 4;
	memcpy(serializedPackage + offset, &(pcb->cursor_stack), 4);
	offset += sizeToSend;

	sizeToSend = 4;
	memcpy(serializedPackage + offset, &(pcb->seg_idx_cod), 4);
	offset += sizeToSend;	
	
	sizeToSend = 4;
	memcpy(serializedPackage + offset, &(pcb->seg_idx_etq), 4);
	offset += sizeToSend;	
	
	sizeToSend = 4;
	memcpy(serializedPackage + offset, &(pcb->p_counter), 4);
	offset += sizeToSend;	

	sizeToSend = 4;
	memcpy(serializedPackage + offset, &(pcb->size_ctxt_actual), 4);
	offset += sizeToSend;
		
	sizeToSend = 4;
	memcpy(serializedPackage + offset, &(pcb->size_idx_etq), 4);
	offset += sizeToSend;	

	return ((char *) serializedPackage);
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

int crear_conexion_entrante(int *listenningSocket, char *puerto_escucha, struct addrinfo **serverInfo, t_log *logger, char *id_proceso)
{
	int reutilizarSocket = 1;
	struct addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// No importa si uso IPv4 o IPv6
	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	// Notar que le pasamos NULL como IP, ya que le indicamos que use localhost en AI_PASSIVE
	if (getaddrinfo(NULL, puerto_escucha, &hints, serverInfo) != 0) {
		log_error(logger,"[%s] No se pudo crear la estructura addrinfo. Motivo: %s", id_proceso, strerror(errno));
		return 1;
	}
	
	if ((*listenningSocket = socket((*serverInfo)->ai_family, (*serverInfo)->ai_socktype, (*serverInfo)->ai_protocol)) < 0) {
		log_error(logger, "[%s] Error al crear socket. Motivo: %s", id_proceso, strerror(errno));
		return 1;
	}
	log_info(logger, "[%s] Se creó el socket a la escucha del puerto: %s", id_proceso, puerto_escucha);

	if(setsockopt(*listenningSocket, SOL_SOCKET, SO_REUSEADDR, &reutilizarSocket, sizeof(int)) == -1){
		log_error(logger, "[%s] Error al hacer que el socket se libere al finalizar el proceos. Motivo: %s", id_proceso, strerror(errno));
		return 1;	
	}

	if(bind(*listenningSocket,(*serverInfo)->ai_addr, (*serverInfo)->ai_addrlen)) {
		log_error(logger, "[%s] No se pudo bindear el socket a la dirección. Motivo: %s", id_proceso, strerror(errno));
		return 1;
	}

	listen(*listenningSocket, 10);

	return 0;
}
