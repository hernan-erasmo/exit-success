#include <semaphore.h>
#include <pthread.h>
#include <netdb.h>

#include <commons/log.h>
#include <commons/collections/list.h>
#include <parser/metadata_program.h>

#include "../../utils/comunicacion.h"
#include "../../utils/interfaz_umv.h"
#include "kernel.h"
#include "vaciar_exit.h"
#include "wt_nar.h"

#ifndef PLP_H
#define PLP_H

int socket_umv;

typedef struct datos_plp {
	char *puerto_escucha;
	char *ip_umv;
	int puerto_umv;
	uint32_t tamanio_stack;
	t_log *logger;
} t_datos_plp;

typedef struct worker_th {
	t_list *cola_exit;
	t_log *logger;
} t_worker_th;

void *plp(void *puerto_prog);

t_pcb *crearPCB(t_metadata_program *metadata, int id_programa);

uint32_t crear_segmento_codigo(int socket_umv, t_pcb *pcb, t_paquete_programa *paquete, uint32_t contador_id_programa, t_log *logger);
uint32_t crear_segmento_stack(int socket_umv, t_pcb *pcb, uint32_t tamanio_stack, uint32_t contador_id_programa, t_log *logger);
uint32_t crear_segmento_indice_codigo(int socket_umv, t_pcb *pcb, t_metadata_program *metadatos, uint32_t contador_id_programa, t_log *logger);
uint32_t crear_segmento_etiquetas(int socket_umv, t_pcb *pcb, t_metadata_program *metadatos, uint32_t contador_id_programa, t_log *logger);

int atender_solicitud_programa(int socket_umv, t_paquete_programa *paquete, t_pcb *pcb, uint32_t tamanio_stack, pthread_mutex_t *mutex, t_log *logger);

void calcularPeso(t_pcb *pcb, t_metadata_program *metadatos);
bool ordenar_por_peso(void *a, void *b);
void mostrar_datos_cola(void *item);

#endif /* PLP_H */
