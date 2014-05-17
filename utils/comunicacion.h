#include <netinet/in.h>

typedef struct paquete_programa {
	char id;
	char *mensaje;
	uint32_t sizeMensaje;
	uint32_t tamanio_total;
} t_paquete_programa;

int sendAll(int sock, char *buf, int *len);
