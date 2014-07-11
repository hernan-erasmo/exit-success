#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

#include <commons/log.h>
#include <commons/collections/list.h>

#include "kernel.h"

void *hilo_semaforo(void *init_semaforo);
