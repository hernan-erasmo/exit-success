#include <semaphore.h>
#include <pthread.h>
#include <commons/collections/list.h>

#include "kernel.h"

void *vaciar_exit(void *init);

void *vaciar_exit(void *init)
{
	pthread_mutex_t procesar_cola = PTHREAD_MUTEX_INITIALIZER;

	while(1){
		sem_wait(&s_exit);
			pthread_mutex_lock(&procesar_cola);
				//acá habría que llamar a la umv para destruir los segmentos
				//acá habría que liberar el pcb
			pthread_mutex_unlock(&procesar_cola);
	}
}