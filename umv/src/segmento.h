#include <stdint.h>

#include <commons/collections/list.h>

typedef struct segmento {
	int32_t prog_id;
	int32_t seg_id;
	uint32_t inicio;
	int32_t size;
	void *pos_mem_ppal;
} t_segmento;

typedef struct esp_libre {
	void *dir;
	int32_t size;
} t_esp_libre;

t_list *buscarEspaciosLibres(t_list *segmentos, void *mem_ppal, int32_t size_mem_ppal);
void reordenarListaSegmentos(t_list *segmentos);
t_esp_libre *crearInstanciaEspLibre(void *mem, int32_t size);
void eliminarEspacioLibre(void *esp_libre);
void mostrarInfoEspacioLibre(void *esp_libre);

t_segmento *crearSegmento(int32_t prog_id, int32_t seg_id, int32_t size);
void eliminarSegmento(void *seg);
void mostrarInfoSegmento(void *seg);
