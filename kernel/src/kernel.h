#include <commons/log.h>
#include <commons/config.h>

#ifndef KERNEL_H
#define KERNEL_H

#include "plp.h"
#include "pcp.h"

t_list *cola_new;
t_list *cola_ready;
t_list *cola_exec;
t_list *cola_block;
t_list *cola_exit;

uint32_t multiprogramacion;

int checkArgs(int args);
int crearLogger(t_log **logger);
int cargarConfig(t_config **config, char *path);
t_datos_plp *crearConfiguracionPlp(t_config *config, t_log *logger);
t_datos_pcp *crearConfiguracionPcp(t_config *config, t_log *logger);

#endif /* KERNEL_H */
