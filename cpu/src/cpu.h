#include <netdb.h>

#include <commons/log.h>
#include <commons/config.h>

#include "../../utils/comunicacion.h"

#ifndef CPU_H
#define CPU_H

int checkArgs(int args);
int crearLogger(t_log **logger);
int cargarConfig(t_config **config, char *path);

#endif /* CPU_H */
