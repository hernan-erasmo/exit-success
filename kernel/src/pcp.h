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
int extraerComandoYPcb(char *mensaje, char **syscall_serializada, char **pcb_serializado);
void ejecutarSyscall(char *syscall_completa, t_pcb *pcb_a_atender, int *status_op, void **respuesta_op, int socket_respuesta, t_log *logger);
void quitar_cpu(t_list *cpus_ociosas, int socket_buscado);

#endif /* PCP_H */
