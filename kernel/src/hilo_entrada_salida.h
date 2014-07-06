#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <time.h>

#include <commons/log.h>
#include <commons/collections/list.h>

#include "kernel.h"

void *hilo_entrada_salida(void *init_hilo_io);
struct timespec * generarRetardo(char *nombre_dispositivo, int unidades, int milisegundosPorUnidad, t_log *logger);
