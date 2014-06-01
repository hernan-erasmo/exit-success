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
int init_escucha_programas(int *listenningSocket, char *puerto, struct addrinfo **serverInfo, t_log *logger);
int crearConexion(int *unSocket, struct sockaddr_in *socketInfo, char *ip, int puerto, t_log *logger);
int crearSocket(struct sockaddr_in *socketInfo, char *ip, int puerto);

int enviar_handshake(int unSocket, t_log *logger);
