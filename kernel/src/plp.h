#include <pthread.h>
#include <netdb.h>

#include <commons/log.h>
#include <parser/metadata_program.h>

typedef struct datos_plp {
	char *puerto;
	t_log *logger;
} t_datos_plp;

typedef struct con_prog {
	char *codigo;
	t_medatada_program medatada;
} t_con_prog;

void *plp(void *puerto_prog);
int init_escucha_programas(int *listenningSocket, char *puerto, struct addrinfo **serverInfo, t_log *logger);
