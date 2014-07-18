#include <semaphore.h>
#include <pthread.h>
#include <commons/collections/list.h>

#include "vaciar_exit.h"
#include "kernel.h"

void *vaciar_exit(void *init)
{
	t_log *logger = ((t_worker_th *) init)->logger;
	pthread_mutex_t procesar_cola = PTHREAD_MUTEX_INITIALIZER;

	log_info(logger, "[WH_EXIT] Hola, soy el worker thread que va a vaciar la cola de exit.");

	int i;
	int size;
	t_pcb *pcb_para_destruir;

	while(1){
		log_info(logger, "[WH_EXIT] Me bloqueo y espero algún PCB.");
		sem_wait(&s_exit);
		
			pthread_mutex_lock(&procesar_cola);
				size = list_size(cola_exit);			
				//log_info(logger, "[WH_EXIT] Me desbloqueé. Agárrense, PCBs!. (tamaño de la cola exit = %d)", size);

				//acá habría que llamar a la umv para destruir los segmentos
				for(i = 0; i < size; i++){
					pcb_para_destruir = list_remove(cola_exit, i);
					log_info(logger, "[WH_EXIT] Entró el PCB del programa con ID=%d, lo voy a hacer percha!", pcb_para_destruir->id);
					solicitar_destruir_segmentos(socket_umv, pcb_para_destruir->id, 'P', logger);
				}
			
				//acá habría que liberar el pcb
				free(pcb_para_destruir);
			pthread_mutex_unlock(&procesar_cola);
		
		log_info(logger, "[WH_EXIT] Ya hay un PCB menos en el mundo.");
	}
}