#include <stdint.h>

#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>

#include "consola.h"

int crearLogger(t_log **logger);
int cargarConfig(t_config **config, char *path);
void *inicializarMemoria(uint32_t size);
void inicializarConfigConsola(t_consola_init **c_init, uint32_t sizeMem, void *mem, t_list *listaSegmentos, char *algoritmo_comp);
