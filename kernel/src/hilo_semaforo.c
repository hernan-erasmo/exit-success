#include "hilo_semaforo.h"

void *hilo_semaforo(void *init_semaforo)
{
	t_log *logger = ((t_semaforo_ansisop *) init_semaforo)->logger;
	char *nombre_sem = ((t_semaforo_ansisop *) init_semaforo)->nombre;
	int *valor_sem = ((t_semaforo_ansisop *) init_semaforo)->valor;
	t_list *lista_pcbs = ((t_semaforo_ansisop *) init_semaforo)->pcbs_en_wait;
	sem_t *liberar_pcb = ((t_semaforo_ansisop *) init_semaforo)->liberar;
	int posicion = 0;

	pthread_mutex_t encolar;
	pthread_mutex_init(&encolar, NULL);

	log_info(logger, "[SEMAFORO_ANSISOP-%s] Estoy listo. Mi valor inicial es: %d.", nombre_sem, *valor_sem);

	while(1){
		log_info(logger, "[SEMAFORO_ANSISOP-%s] Me bloqueo esperando liberar algún PCB.", nombre_sem);

		if(*valor_sem < 0)
			log_info(logger, "[SEMAFORO_ANSISOP-%s] En este momento hay %d en la cola.", nombre_sem, *valor_sem);
		else
			log_info(logger, "[SEMAFORO_ANSISOP-%s] No tengo PCBs en la cola. Mi valor es: %d", nombre_sem, *valor_sem);

		sem_wait(liberar_pcb);	//liberar_pcb arranca valiendo 0
			if(*valor_sem >= 0){
				*valor_sem = *valor_sem + 1;
				continue;
			}

			pthread_mutex_lock(&encolar);
				t_pcb *pcb_liberado = list_remove(lista_pcbs, 0);
				log_info(logger, "[SEMAFORO_ANSISOP-%s] Voy a liberar un PCB con id: %d", nombre_sem, pcb_liberado->id);
				
				posicion = list_size(cola_ready);
				list_add_in_index(cola_ready,posicion,pcb_liberado);	//lo mando al fondo, porque es round robin
				sem_post(&s_ready_nuevo);
				
				*valor_sem = *valor_sem + 1;

				_mostrar_cola_semaforo((t_semaforo_ansisop *) init_semaforo, logger);
			pthread_mutex_unlock(&encolar);
	}

	pthread_exit(NULL);
}
