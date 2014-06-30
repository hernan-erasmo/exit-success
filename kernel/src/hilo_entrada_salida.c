#include "hilo_entrada_salida.h"

void *hilo_entrada_salida(void *config_hilo_io)
{
	t_cola_io *init_io = (t_cola_io *) config_hilo_io;
	t_log *logger = init_io->logger;
	char *nombre_dispositivo = init_io->nombre_dispositivo;
	uint32_t esperaEnSegundos = init_io->tiempo_espera;
	t_list *cola_dispositivo = init_io->cola_dispositivo;

	log_info(logger, "[DISPOSITIVO-%s] Estoy listo. Mi valor de demora es: %d.", nombre_dispositivo, esperaEnSegundos);

	pthread_exit(NULL);
}
