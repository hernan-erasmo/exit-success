#include <pthread.h>
#include <netdb.h>

#include <commons/log.h>
#include <commons/collections/list.h>
#include <parser/metadata_program.h>

#include "../../utils/comunicacion.h"

typedef struct datos_plp {
	char *puerto_escucha;
	char *ip_umv;
	int puerto_umv;
	uint32_t tamanio_stack;
	t_log *logger;
} t_datos_plp;

typedef struct pcb {
	uint32_t id;				//Identificador del programa
	int *socket;				//Socket de conexión al programa
	void *seg_cod;				//Puntero al comienzo del segmento de código en la umv
	void *seg_stack;			//Puntero al comienzo del segmento de stack en la umv
	void *cursor_stack;			//Puntero al primer byte del contexto de ejcución actual
	void *seg_idx_cod;			//Puntero al comienzo del índice de código en la umv
	void *seg_idx_etq;			//Puntero al comienzo del índice de etiquetas en la umv
	uint32_t p_counter;			//Contiene el nro de la próxima instrucción a ejecutar
	uint32_t size_ctxt_actual;	//Cant de variables (locales y parámetros) del contexto de ejecución actual
	uint32_t size_idx_etq;		//Cantidad de bytes que ocupa el índice de etiquetas
} t_pcb;

void *plp(void *puerto_prog);

t_pcb *crearPCB(t_metadata_program *metadata, int id_programa);
uint32_t solicitar_crear_segmento(int socket_umv, uint32_t id_programa, uint32_t tamanio_segmento, t_log *logger);
char *codificar_crear_segmento(uint32_t id_programa, uint32_t tamanio);
int atender_solicitud_programa(int socket_umv, t_paquete_programa *paquete, t_pcb *pcb, t_log *logger);
