#include <semaphore.h>
#include <pthread.h>
#include <commons/collections/list.h>

#include "dispatcher.h"

#define SIZEOF_PCB_QUE_NO_ES_EL_MISMO_QUE_EL_DE_COMUNICACION_C 48

void *dispatcher(void *init)
{
	t_log *logger = ((t_init_dispatcher *) init)->logger;
	t_list *cpus_ociosas = ((t_init_dispatcher *) init)->cpus_ociosas;

	pthread_mutex_t despachar = PTHREAD_MUTEX_INITIALIZER;
	int valor_s_hay_cpus = 0;

	while(1){
		log_info(logger, "[DISPATCH] Bloqueado esperando algún PCB que entre a Ready");
		sem_wait(&s_ready_nuevo);
		log_info(logger, "[DISPATCH] Entró un PCB a ready, ahora me falta encontrar una CPU donde mandarlo.");

		mostrar_cola_ready(logger);
			
			log_info(logger, "[DISPATCH] Bloqueado esperando alguna CPU ociosa");	
			
			sem_getvalue(&s_hay_cpus, &valor_s_hay_cpus);
			//log_info(logger, "[DISPATCH] El valor del semáforo de las cpus libres antes de hacerle un wait es %d", valor_s_hay_cpus);
			sem_wait(&s_hay_cpus);
			log_info(logger, "[DISPATCH] Ya encontré una CPU ociosa.");

			pthread_mutex_lock(&despachar);
				t_header_cpu *cpu_destino = list_remove(cpus_ociosas, 0);

				//envío el primer pcb de ready a una cpu
				t_pcb *pcb_a_exec = list_remove(cola_ready, 0);	//no hago ningún post, porque no lo estoy mandando a exit
				char *pcb_serializado = serializar_pcb(pcb_a_exec, logger);
				uint32_t size = SIZEOF_PCB_QUE_NO_ES_EL_MISMO_QUE_EL_DE_COMUNICACION_C;
				
				if(sendAll(cpu_destino->socket, pcb_serializado, &size) < 0){
					log_error(logger, "[DISPATCH] Error al enviar el PCB del proceso (ID: %d) a la CPU. Motivo: %s", pcb_a_exec->id, strerror(errno));
					pthread_mutex_unlock(&despachar);
					pthread_exit(NULL);
				}
			pthread_mutex_unlock(&despachar);
	}

	pthread_exit(NULL);
}

void mostrar_cola_ready(t_log *logger)
{
	int i, cant_elementos = 0;
	cant_elementos = list_size(cola_ready);
	t_pcb *p = NULL;

	pthread_mutex_t listarDeCorrido = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&listarDeCorrido);
		log_info(logger, "[COLA_READY] La cola de ready tiene %d PCBs:", cant_elementos);
		for(i = 0; i < cant_elementos; i++){
			p = list_get(cola_ready, i);
			log_info(logger, "\tID: %d, Peso: %d", p->id, p->peso);
		}
	pthread_mutex_unlock(&listarDeCorrido);

	return;
}
