#include <commons/log.h>
#include <commons/config.h>

#ifndef KERNEL_H
#define KERNEL_H

#include "plp.h"

int checkArgs(int args);
int crearLogger(t_log **logger);
int cargarConfig(t_config **config, char *path);
t_datos_plp *crearConfiguracionPlp(t_config *config, t_log *logger, t_list *cola_new, t_list *cola_exit);

#endif /* KERNEL_H */
