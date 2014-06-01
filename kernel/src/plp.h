#include <pthread.h>
#include <netdb.h>

#include <commons/log.h>
#include <parser/metadata_program.h>

#include "../../utils/comunicacion.h"

typedef struct datos_plp {
	char *puerto_escucha;
	char *ip_umv;
	int puerto_umv;
	t_log *logger;
} t_datos_plp;

typedef struct con_prog {
	char *codigo;
	t_medatada_program medatada;
} t_con_prog;

void *plp(void *puerto_prog);

int enviar_handshake(int unSocket, t_log *logger);
