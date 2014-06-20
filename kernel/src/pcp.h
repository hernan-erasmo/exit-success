#include <pthread.h>
#include <netdb.h>

#include <commons/log.h>
#include <commons/collections/list.h>

#include "../../utils/comunicacion.h"
#include "kernel.h"
#include "dispatcher.h"

#ifndef PCP_H
#define PCP_H

typedef struct datos_pcp{
	t_log *logger;
	char *puerto_escucha_cpu;
} t_datos_pcp;

typedef struct init_dispatcher {
	t_log *logger;
	t_list *cpus_ociosas;
}t_init_dispatcher;

void *pcp(void *datos_pcp);

#endif /* PCP_H */
