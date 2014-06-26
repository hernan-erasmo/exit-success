#include <commons/log.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#include "comunicacion.h"

#ifndef INTERFAZ_UMV_H
#define INTERFAZ_UMV_H

uint32_t solicitar_cambiar_proceso_activo(int socket_umv, uint32_t contador_id_programa, char origen, t_log *logger);
uint32_t solicitar_crear_segmento(int socket_umv, uint32_t id_programa, uint32_t tamanio_segmento, char origen, t_log *logger);
uint32_t solicitar_enviar_bytes(int socket_umv, uint32_t base, uint32_t offset, int tamanio, void *buffer, uint32_t id_programa, char origen, t_log *logger);
uint32_t solicitar_destruir_segmentos(int socket_umv, uint32_t id_programa, char origen, t_log *logger);
void *solicitar_solicitar_bytes(int socket_umv, uint32_t base, uint32_t offset, int tamanio, char origen, t_log *logger);

char *codificar_cambiar_proceso_activo(uint32_t contador_id_programa);
char *codificar_crear_segmento(uint32_t id_programa, uint32_t tamanio);
char *codificar_enviar_bytes(uint32_t base, uint32_t offset, int tamanio, void *buffer, uint32_t id_programa, uint32_t *tamanio_de_la_orden_completa);
char *codificar_solicitar_bytes(uint32_t base, uint32_t offset, int tamanio);
char *codificar_destruir_segmentos(uint32_t id_programa);

#endif /* INTERFAZ_UMV_H */
