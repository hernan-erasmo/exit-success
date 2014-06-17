#include <semaphore.h>
#include <pthread.h>
#include <netdb.h>

#include <commons/log.h>
#include <commons/collections/list.h>
#include <parser/metadata_program.h>

#include "../../utils/comunicacion.h"
#include "kernel.h"

#ifndef PLP_H
#define PLP_H

typedef struct datos_plp {
	char *puerto_escucha;
	char *ip_umv;
	int puerto_umv;
	uint32_t tamanio_stack;
	t_log *logger;
} t_datos_plp;

void *plp(void *puerto_prog);

t_pcb *crearPCB(t_metadata_program *metadata, int id_programa);

uint32_t solicitar_cambiar_proceso_activo(int socket_umv, uint32_t contador_id_programa, t_log *logger);
uint32_t solicitar_crear_segmento(int socket_umv, uint32_t id_programa, uint32_t tamanio_segmento, t_log *logger);
uint32_t solicitar_enviar_bytes(int socket_umv, uint32_t base, uint32_t offset, int tamanio, void *buffer, t_log *logger);
char *codificar_cambiar_proceso_activo(uint32_t contador_id_programa);
char *codificar_crear_segmento(uint32_t id_programa, uint32_t tamanio);
char *codificar_enviar_bytes(uint32_t base, uint32_t offset, int tamanio, void *buffer, uint32_t *tamanio_de_la_orden_completa);

uint32_t crear_segmento_codigo(int socket_umv, t_pcb *pcb, t_paquete_programa *paquete, uint32_t contador_id_programa, t_log *logger);
uint32_t crear_segmento_stack(int socket_umv, t_pcb *pcb, uint32_t tamanio_stack, uint32_t contador_id_programa, t_log *logger);
uint32_t crear_segmento_indice_codigo(int socket_umv, t_pcb *pcb, t_metadata_program *metadatos, uint32_t contador_id_programa, t_log *logger);
uint32_t crear_segmento_etiquetas(int socket_umv, t_pcb *pcb, t_metadata_program *metadatos, uint32_t contador_id_programa, t_log *logger);

int atender_solicitud_programa(int socket_umv, t_paquete_programa *paquete, t_pcb *pcb, uint32_t tamanio_stack, pthread_mutex_t *mutex, t_log *logger);

void calcularPeso(t_pcb *pcb, t_metadata_program *metadatos);
bool ordenar_por_peso(void *a, void *b);
void mostrar_datos_cola(void *item);

#endif /* PLP_H */
