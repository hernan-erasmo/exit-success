#include <stdio.h>
#include <pthread.h>

#include "segmento.h"

typedef struct consola_init {
	t_list *listaSegmentos;
	void *mem_ppal;
	uint32_t tamanio_mem_ppal;
} t_consola_init;

void *consola(void *c_init);
char *getLinea(void);
void crear_segmento(t_list *listaSegmentos, void *mem_ppal, uint32_t tamanio_mem_ppal);
void destruir_segmentos(t_list *listaSegmentos);
void info_memoria(void *mem_ppal, t_list *listaSegmentos, uint32_t tamanio_mem_ppal);
