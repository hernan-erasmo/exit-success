#include <semaphore.h>
#include <pthread.h>
#include <commons/collections/list.h>

#include "wt_nar.h"
#include "kernel.h"

//Worker thread que espera hasta que haya lugar en ready para pasarle el primer pcb que haya en new.
void *wt_nar(void *init)
{
	pthread_mutex_t encolar = PTHREAD_MUTEX_INITIALIZER;
	t_pcb *pcb_a_encolar = NULL;

	sem_wait(&s_ready_max);
		pthread_mutex_lock(&encolar);
			pcb_a_encolar = list_remove(cola_new, 0);
			list_add(cola_ready, pcb_a_encolar);
		pthread_mutex_unlock(&encolar);
	sem_post(&s_ready_nuevo);

	pthread_exit(NULL);
}