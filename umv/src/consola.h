#include <stdio.h>
#include <pthread.h>

#include "segmento.h"

typedef struct consola_init {
	t_list *listaSegmentos;
	void *mem_ppal;
	uint32_t tamanio_mem_ppal;
	char *algoritmo_comp;
} t_consola_init;

void *consola(void *c_init);
char *getLinea(void);
t_list *buscarSegmentosConId(t_list *listaSegmentos, uint32_t id);

void comando_cambiar_algoritmo(char **algoritmo);
void comando_compactar(t_list *listaSegmentos, void *mem_ppal, uint32_t tamanio_mem_ppal);
void comando_crear_segmento(t_list *listaSegmentos, void *mem_ppal, uint32_t tamanio_mem_ppal, char *algoritmo);
void comando_destruir_segmentos(t_list *listaSegmentos);
void comando_dump_segmentos(t_list *listaSegmentos);
void comando_info_memoria(void *mem_ppal, t_list *listaSegmentos, uint32_t tamanio_mem_ppal, char *algoritmo);
