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
	int socket;					//Socket de conexión al programa
	uint32_t peso;				//Resultado del cálculo del peso para ordenarlo al entrar a la cola ready.
	uint32_t seg_cod;			//Puntero al comienzo del segmento de código en la umv
	uint32_t seg_stack;			//Puntero al comienzo del segmento de stack en la umv
	void *cursor_stack;			//Puntero al primer byte del contexto de ejcución actual
	uint32_t seg_idx_cod;		//Puntero al comienzo del índice de código en la umv
	void *seg_idx_etq;			//Puntero al comienzo del índice de etiquetas en la umv
	uint32_t p_counter;			//Contiene el nro de la próxima instrucción a ejecutar
	uint32_t size_ctxt_actual;	//Cant de variables (locales y parámetros) del contexto de ejecución actual
	uint32_t size_idx_etq;		//Cantidad de bytes que ocupa el índice de etiquetas
} t_pcb;

void *plp(void *puerto_prog);

t_pcb *crearPCB(t_metadata_program *metadata, int id_programa);

uint32_t solicitar_cambiar_proceso_activo(int socket_umv, uint32_t contador_id_programa, t_log *logger);
uint32_t solicitar_crear_segmento(int socket_umv, uint32_t id_programa, uint32_t tamanio_segmento, t_log *logger);
uint32_t solicitar_enviar_bytes(int socket_umv, uint32_t base, uint32_t offset, int tamanio, char *buffer, t_log *logger);
char *codificar_cambiar_proceso_activo(uint32_t contador_id_programa);
char *codificar_crear_segmento(uint32_t id_programa, uint32_t tamanio);
char *codificar_enviar_bytes(uint32_t base, uint32_t offset, int tamanio, char *buffer);

uint32_t crear_segmento_codigo(int socket_umv, t_pcb *pcb, t_paquete_programa *paquete, uint32_t contador_id_programa, t_log *logger);
uint32_t crear_segmento_stack(int socket_umv, t_pcb *pcb, uint32_t tamanio_stack, uint32_t contador_id_programa, t_log *logger);
uint32_t crear_segmento_indice_codigo(int socket_umv, t_pcb *pcb, t_metadata_program *metadatos, uint32_t contador_id_programa, t_log *logger);

int atender_solicitud_programa(int socket_umv, t_paquete_programa *paquete, t_pcb *pcb, uint32_t tamanio_stack, t_log *logger);

void calcularPeso(t_pcb *pcb, t_metadata_program *metadatos);
bool ordenar_por_peso(void *a, void *b);
void mostrar_datos_cola(void *item);
