#include <stdint.h>

typedef struct segmento {
	int32_t prog_id;
	int32_t seg_id;
	void *inicio;
	int32_t size;
	void *pos_mem_ppal;
} t_segmento;

t_segmento *crearSegmento(int32_t prog_id, int32_t seg_id, int32_t size);
void eliminarSegmento(void *seg);
void mostrarInfoSegmento(void *seg);
