#include <semaphore.h>
#include <pthread.h>
#include <commons/collections/list.h>

#include "dispatcher.h"
#include "kernel.h"

#define SIZEOF_PCB_QUE_NO_ES_EL_MISMO_QUE_EL_DE_COMUNICACION_C 48

void *dispatcher(void *init)
{
	t_log *logger = ((t_init_dispatcher *) init)->logger;
	t_list *cpus_ociosas = ((t_init_dispatcher *) init)->cpus_ociosas;

	while(1){
		log_info(logger, "[DISPATCH] Bloqueado esperando algún PCB que entre a Ready");
		sem_wait(&s_ready_nuevo);
			
			log_info(logger, "[DISPATCH] Bloqueado esperando alguna CPU ociosa");	
			sem_wait(&s_hay_cpus);
				t_header_cpu *cpu_destino = list_remove(cpus_ociosas, 0);

				//envío el primer pcb de ready a una cpu
				t_pcb *pcb_a_exec = list_remove(cola_ready, 0);
				char *pcb_serializado = serializar_pcb(pcb_a_exec, logger);
				uint32_t size = SIZEOF_PCB_QUE_NO_ES_EL_MISMO_QUE_EL_DE_COMUNICACION_C;
				
				if(sendAll(cpu_destino->socket, pcb_serializado, &size) < 0){
					log_error(logger, "[DISPATCH] Error al enviar el PCB del proceso (ID: %d) a la CPU. Motivo: %s", pcb_a_exec->id, strerror(errno));
					pthread_exit(NULL);
				}
	}

	pthread_exit(NULL);
}
