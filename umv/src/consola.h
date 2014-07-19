#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "umv.h"

#ifndef CONSOLA_H
#define CONSOLA_H

typedef struct consola_init {
	t_list *listaSegmentos;
	void *mem_ppal;
	uint32_t tamanio_mem_ppal;
	char *algoritmo_comp;
	pid_t umv_pid;
	int *listenningSocket;
	int *noTerminar;
	int puerto;
	struct timespec *ret;
} t_consola_init;

void *consola(void *c_init);
char *getLinea(void);
t_list *buscarSegmentosConId(t_list *listaSegmentos, uint32_t id);
void imprimirArrayDeBytes(void *offset, uint32_t cantidad, FILE *archivo_out);
void imprimirArrayDeChars(void *offset, uint32_t cantidad, FILE *archivo_out);

void comando_cambiar_algoritmo(char **algoritmo);
void comando_cambiar_proceso_activo();
void comando_compactar(t_list *listaSegmentos, void *mem_ppal, uint32_t tamanio_mem_ppal);
void comando_crear_segmento(t_list *listaSegmentos, void *mem_ppal, uint32_t tamanio_mem_ppal, char *algoritmo);
void comando_destruir_segmentos(t_list *listaSegmentos);
void comando_dump_all(void *mem_ppal, uint32_t tamanio_mem_ppal);
void comando_dump_segmentos(t_list *listaSegmentos);
void comando_enviar_bytes();
void comando_info_memoria(void *mem_ppal, t_list *listaSegmentos, uint32_t tamanio_mem_ppal, char *algoritmo);
void comando_matar_umv(pid_t pid, t_consola_init *c_init);
void comando_retardo();

#endif /* CONSOLA_H */
