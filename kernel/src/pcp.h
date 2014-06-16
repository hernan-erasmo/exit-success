#include <pthread.h>
#include <netdb.h>

#include <commons/log.h>
#include <commons/collections/list.h>

#include "../../utils/comunicacion.h"

#ifndef PCP_H
#define PCP_H

typedef struct datos_pcp{
	t_list *cola_ready;
	t_list *cola_exec;
	t_list *cola_block;
	t_list *cola_exit;
	t_log *logger;
	char *puerto_escucha_cpu;
} t_datos_pcp;

void *pcp(void *datos_pcp);

#endif /* PCP_H */