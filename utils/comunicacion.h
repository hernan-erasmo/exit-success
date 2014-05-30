#include <netinet/in.h>

typedef struct paquete_programa {
	char id;
	char *mensaje;
	uint32_t sizeMensaje;
	uint32_t tamanio_total;
} t_paquete_programa;

int sendAll(int sock, char *buf, int *len);
int recvAll(t_paquete_programa *paquete, int sock);
void inicializar_paquete(t_paquete_programa *paq);
