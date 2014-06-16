#include <netdb.h>

#include <commons/log.h>
#include <commons/config.h>

#ifndef CPU_H
#define CPU_H

int checkArgs(int args);
int crearLogger(t_log **logger);
int cargarConfig(t_config **config, char *path);

char *codificar_enviar_bytes(uint32_t base, uint32_t offset, int tamanio, void *buffer, uint32_t *tamanio_de_la_orden_completa);

#endif /* CPU_H */
