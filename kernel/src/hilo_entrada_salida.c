#include "hilo_entrada_salida.h"

void *hilo_entrada_salida(void *config_hilo_io)
{
	t_cola_io *init_io = (t_cola_io *) config_hilo_io;
	t_log *logger = init_io->logger;
	char *nombre_dispositivo = init_io->nombre_dispositivo;
	uint32_t esperaEnMillis = init_io->tiempo_espera;
	t_list *cola_dispositivo = init_io->cola_dispositivo;
	sem_t *s_cola_io = init_io->s_cola;

	pthread_mutex_t encolar;
	pthread_mutex_init(&encolar, NULL);

	log_info(logger, "[DISPOSITIVO-%s] Estoy listo. Mi valor de demora es: %d.", nombre_dispositivo, esperaEnMillis);
	if(sem_init(s_cola_io, 0, 0) != 0){
		log_error(logger, "[DISPOSITIVO-%s] Hubo un error al inicializar el semáforo de mi cola. Adiós para siempre!.", nombre_dispositivo);
		pthread_exit(NULL);
	}

	while(1){
		log_info(logger, "[DISPOSITIVO-%s] Me bloqueo esperando a que algún programa solicite mis servicios.", nombre_dispositivo);
		
		sem_wait(s_cola_io);
			pthread_mutex_lock(encolar);
				t_pcb_en_io *pcb_a_procesar = list_remove(cola_dispositivo, 0);
				log_info(logger, "[DISPOSITIVO-%s] El proceso con ID %d necesita de mis servicios.", nombre_dispositivo, pcb_a_procesar->pcb_en_espera->id);
			pthread_mutex_unlock(encolar);

			//genero la estructura de tiempo, le paso como parámetro las unidades a dormir y que me devuelva el timespec que tengo
			//que meter en nanosleep()
			//nanosleep(asdfajdfoasidf);

			//pongo el PCB nuevamente en ready.
			/*	mutex()
					list_add(cola_new, pcb_a_procesar->pcb_en_espera);
					sem_post(s_ready);
				finMutex()
			*/

			//libero pcb_a_procesar.
	}

	pthread_exit(NULL);
}
