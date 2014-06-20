#include <semaphore.h>
#include <pthread.h>
#include <commons/collections/list.h>

#include "dispatcher.h"
#include "kernel.h"

void *dispatcher(void *init)
{
	t_log *logger = ((t_init_dispatcher *) init)->logger;

	while(1){
		log_info(logger, "[DISPATCH] Bloqueado esperando algún PCB que entre a Ready");
		sem_wait(&s_ready_nuevo);
			log_info(logger, "[DISPATCH] Bloqueado esperando alguna CPU ociosa");
			sem_wait(&s_hay_cpus);
				//envío el primer pcb de ready a una cpu
	}

	pthread_exit(NULL);
}
