#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>

#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>

#ifndef UMV_H
#define UMV_H

#include "consola.h"
#include "atencionConexiones.h"
#include "../../utils/comunicacion.h"


static uint32_t PROCESO_ACTIVO = 0;

int crearLogger(t_log **logger);
int cargarConfig(t_config **config, char *path);
void *inicializarMemoria(uint32_t size);
void inicializarConfigConsola(t_consola_init **c_init, uint32_t sizeMem, void *mem, t_list *listaSegmentos, char *algoritmo_comp, int *listenningSocket, int *noTerminar, int puerto);
void manejar_salida(int sig);

uint32_t enviar_bytes(t_list *listaSegmentos, uint32_t base, uint32_t offset, uint32_t tamanio, char *buffer);
t_segmento *buscar_segmento_solicitado(t_list *listaSegmentos, uint32_t base);
bool buscar_por_proceso_activo(void *seg);
int chequear_limites_escritura(t_segmento *seg, uint32_t offset, uint32_t tamanioAEscribir);

#endif /* UMV_H */
