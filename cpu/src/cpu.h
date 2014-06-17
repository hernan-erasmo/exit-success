#include <netdb.h>

#include <commons/log.h>
#include <commons/config.h>

#include <parser/metadata_program.h>
#include <parser/parser.h>

#include "../../utils/comunicacion.h"

#ifndef CPU_H
#define CPU_H

t_pcb pcb;

int checkArgs(int args);
int crearLogger(t_log **logger);
int cargarConfig(t_config **config, char *path);

#endif /* CPU_H */
