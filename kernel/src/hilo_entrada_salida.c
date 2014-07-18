#include "hilo_entrada_salida.h"

void *hilo_entrada_salida(void *config_hilo_io)
{
	t_cola_io *init_io = (t_cola_io *) config_hilo_io;
	t_log *logger = init_io->logger;
	char *nombre_dispositivo = init_io->nombre_dispositivo;
	uint32_t esperaEnMillis = init_io->tiempo_espera;
	t_list *cola_dispositivo = init_io->cola_dispositivo;
	sem_t *s_cola_io = init_io->s_cola;
	int posicion = 0;

	pthread_mutex_t encolar;
	pthread_mutex_init(&encolar, NULL);

	log_info(logger, "[DISPOSITIVO-%s] Estoy listo. Mi valor de demora es: %d.", nombre_dispositivo, esperaEnMillis);
	
	while(1){
		log_info(logger, "[DISPOSITIVO-%s] Me bloqueo esperando a que algún programa solicite mis servicios.", nombre_dispositivo);
		
		sem_wait(s_cola_io);
			pthread_mutex_lock(&encolar);
				t_pcb_en_io *pcb_a_procesar = list_remove(cola_dispositivo, 0);
				log_info(logger, "[DISPOSITIVO-%s] El proceso con ID %d necesita de mis servicios.", nombre_dispositivo, pcb_a_procesar->pcb_en_espera->id);
			pthread_mutex_unlock(&encolar);

			//genero la estructura de tiempo, le paso como parámetro las unidades a dormir y que me devuelva el timespec que tengo
			//que meter en nanosleep()
			struct timespec *demora = generarRetardo(nombre_dispositivo, pcb_a_procesar->unidades_de_tiempo, esperaEnMillis, logger);
			nanosleep(demora, NULL);

			free(demora);

		//Ya pasó el tiempo, entonces pongo el pcb nuevamente en ready.
		//log_info(logger, "[DISPOSITIVO-%s] Ya terminé de atender al proceso %d. Lo pongo en ready.", nombre_dispositivo, pcb_a_procesar->pcb_en_espera->id);
		pthread_mutex_lock(&encolar);
			posicion = list_size(cola_ready);
			list_add_in_index(cola_ready,posicion,pcb_a_procesar->pcb_en_espera);	//lo mando al fondo, porque es round robin
			sem_post(&s_ready_nuevo);
		pthread_mutex_unlock(&encolar);
		
		//hay que liberar la memoria de pcb_a_procesar? SI, porque la allocaste en la definicion de la syscall en el kernel.
		free(pcb_a_procesar);
	}

	pthread_exit(NULL);
}

struct timespec * generarRetardo(char *nombre_dispositivo, int unidades, int milisegundosPorUnidad, t_log *logger)
{
	int segundos = 0;
	long nanosegundos = 0;
	int milisegundosAOperar = milisegundosPorUnidad * unidades;	

	struct timespec *demora = malloc(sizeof(struct timespec));

	if(milisegundosAOperar > 999){
		segundos = (milisegundosAOperar / 1000);
		nanosegundos = (milisegundosAOperar % 1000) * 1000000;	//1 ms = 1.000.000 ns
	} else {
		nanosegundos = milisegundosAOperar * 1000000;
	}

	log_info(logger, "[DISPOSITIVO-%s] Voy a operar durante %d milisegundos, que equivalen a %d segundos y %ld nanosegundos \n", nombre_dispositivo, milisegundosAOperar, segundos, nanosegundos);

	demora->tv_sec = (time_t) segundos;
	demora->tv_nsec = nanosegundos;

	return demora;
}
