#include <stdio.h>
#include <errno.h>

#include "pcp.h"

static uint32_t finalizar = 0;

void *pcp(void *datos_pcp)
{
	//Variables de inicializacion
	t_log *logger = ((t_datos_pcp *) datos_pcp)->logger;
	t_list *cola_new = ((t_datos_pcp *) datos_pcp)->cola_ready;
	t_list *cola_block = ((t_datos_pcp *) datos_pcp)->cola_block;
	t_list *cola_exec = ((t_datos_pcp *) datos_pcp)->cola_exec;
	t_list *cola_exit = ((t_datos_pcp *) datos_pcp)->cola_exit;

	log_info(logger, "[PCP] Estoy adentro del PCP!");

	//Acá va a ir el bucle principal del PCP (que incluirá su select() para manejar las conexiones con las CPUs)

	pthread_exit(NULL);
}
