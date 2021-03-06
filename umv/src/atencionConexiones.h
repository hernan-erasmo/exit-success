#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <commons/log.h>
#include <commons/collections/list.h>

#include "umv.h"

#ifndef ATENCIONCONEXIONES_H
#define ATENCIONCONEXIONES_H

pthread_mutex_t op_atomica;

typedef struct param_memoria {
	t_list *listaSegmentos;
	void *mem_ppal;
	uint32_t tamanio_mem_ppal;
	char *algoritmo_comp;
} t_param_memoria;

typedef struct config_conexion {
	int *socket;
	t_param_memoria *parametros_memoria;
	t_log *logger;
	pthread_t *hilo;
} t_config_conexion;

void *atencionConexiones(void *config);

void handler_plp(int sock, int *respuesta, char *orden, t_param_memoria *parametros_memoria, t_log *logger);
void handler_cpu(int sock, void *respuesta, char *orden, t_param_memoria *parametros_memoria, t_log *logger);
void handler_cambiar_proceso_activo(uint32_t *respuesta, char *orden, t_param_memoria *parametros_memoria, char **savePtr1, t_log *logger);
void handler_crear_segmento(uint32_t *respuesta, char *orden, t_param_memoria *parametros_memoria, char **savePtr1, t_log *logger);
void handler_enviar_bytes(int *respuesta, char *orden, t_param_memoria *parametros_memoria, char **savePtr1, t_log *logger);
void handler_solicitar_bytes(void **respuesta, t_param_memoria *parametros_memoria, uint32_t *tam, char **savePtr1, t_log *logger);
void handler_destruir_segmentos(uint32_t *respuesta, t_param_memoria *parametros_memoria, char **savePtr1, t_log *logger);
void enviar_respuesta_numerica(int *socket, uint32_t respuesta, t_log *logger);
void enviar_respuesta_buffer(int *socket, void *respuesta, uint32_t *tam_buffer, t_log *logger);

#endif /* ATENCIONCONEXIONES_H */
