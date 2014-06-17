#include <netinet/in.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <commons/log.h>

#ifndef COMUNICACION_H
#define COMUNICACION_H

typedef struct pcb {
	uint32_t id;				//Identificador del programa
	int socket;					//Socket de conexión al programa
	uint32_t peso;				//Resultado del cálculo del peso para ordenarlo al entrar a la cola ready.
	uint32_t seg_cod;			//Puntero al comienzo del segmento de código en la umv
	uint32_t seg_stack;			//Puntero al comienzo del segmento de stack en la umv
	void *cursor_stack;			//Puntero al primer byte del contexto de ejcución actual
	uint32_t seg_idx_cod;		//Puntero al comienzo del índice de código en la umv
	uint32_t seg_idx_etq;		//Puntero al comienzo del índice de etiquetas en la umv
	uint32_t p_counter;			//Contiene el nro de la próxima instrucción a ejecutar
	uint32_t size_ctxt_actual;	//Cant de variables (locales y parámetros) del contexto de ejecución actual
	uint32_t size_idx_etq;		//Cantidad de bytes que ocupa el índice de etiquetas
} t_pcb;

typedef struct paquete_programa {
	char id;
	char *mensaje;
	uint32_t sizeMensaje;
	uint32_t tamanio_total;
} t_paquete_programa;

int sendAll(int sock, char *buf, int *len);
int recvAll(t_paquete_programa *paquete, int sock);
int recvPcb(t_pcb *pcb, int sock);
void inicializar_paquete(t_paquete_programa *paq);
char *serializar_paquete(t_paquete_programa *paquete, t_log *logger);

//Sockets
int crear_conexion_saliente(int *unSocket, struct sockaddr_in *socketInfo, char *ip, int puerto, t_log *logger, char *id_proceso);
int crear_conexion_entrante(int *listenningSocket, char *puerto, struct addrinfo **serverInfo, t_log *logger, char *id_proceso);
int crear_socket(struct sockaddr_in *socketInfo, char *ip, int puerto);

#endif /* COMUNICACION_H */
