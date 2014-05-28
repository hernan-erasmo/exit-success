#include <stdint.h>

#include <commons/collections/list.h>

typedef struct segmento {
	uint32_t prog_id;
	uint32_t seg_id;	// Está porque soy un boludo
	uint32_t inicio;	// inicio == base == nro aleatorio que no se debe superponer con otro del mismo prog_id == quilombo
	uint32_t size;
	void *pos_mem_ppal;
	bool marcadoParaBorrar;
} t_segmento;

typedef struct esp_libre {
	void *dir;
	uint32_t size;
} t_esp_libre;

t_list *buscarEspaciosLibres(t_list *segmentos, void *mem_ppal, uint32_t size_mem_ppal);
t_esp_libre *crearInstanciaEspLibre(void *mem, uint32_t size);
t_esp_libre *buscar_primer_lugar_adecuado(t_list *espacios_libres, uint32_t size_requerido);
void eliminarEspacioLibre(void *esp_libre);
void mostrarInfoEspacioLibre(void *esp_libre);
uint32_t getSegId(t_list *listaSegmentos, uint32_t prog_id);
uint32_t getDirInicio(t_list *listaSegmentos, uint32_t prog_id, uint32_t size);
bool esAdecuadoInicio(uint32_t candidato, uint32_t candidato_size, uint32_t actual, uint32_t actual_size);
bool comparador_segmento_direccion_fisica_asc(void *seg_a, void *seg_b);
bool comparador_segmento_tamanio_asc(void *seg_a, void *seg_b);
bool comparador_esp_libre_tamanio_asc(void *esp_a, void *esp_b);
bool comparador_esp_libre_tamanio_desc(void *esp_a, void *esp_b);
void *marcar_para_borrar(void *seg);
bool marcado_para_borrar(void *seg);

/*
**	Interfaz de la UMV
*/

t_segmento *crearSegmento(uint32_t prog_id, uint32_t size, t_list *espacios_libres, t_list *listaSegmentos, char *algoritmo);	//¿Debería retornar un id de segmento?
void eliminarSegmento(void *seg);
void mostrarInfoSegmento(void *seg);
void dump_segmentos(t_list *listaSegmentos);
void destruirSegmentos(t_list *listaSegmentos, uint32_t prog_id);
void actualizarInfoSegmento(void **pos_actual, void **fin_actual, t_segmento *seg);
uint32_t compactar(t_list *segmentos, void *mem_ppal, uint32_t size_mem_ppal);
