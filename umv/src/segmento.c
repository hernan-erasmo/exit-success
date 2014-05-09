#include <stdlib.h>
#include <stdio.h>

#include "segmento.h"

t_list *buscarEspacioLibre(t_list *segmentos, void *mem_ppal, int32_t size_mem_ppal)
{
	int cant_segmentos = list_size(segmentos);
	t_list *lista_esp_libre = list_create();

	if(cant_segmentos == 0){
		list_add(lista_esp_libre, (void *) crearInstanciaEspLibre(mem_ppal, size_mem_ppal));
		return lista_esp_libre;
	}

	//No encontró ningún espacio libre, ¡qué raro!
	return NULL;
}

t_esp_libre *crearInstanciaEspLibre(void *mem, int32_t size)
{
	t_esp_libre *esp_libre = malloc(sizeof(t_esp_libre));
	esp_libre->dir = mem;
	esp_libre->size = size;

	return esp_libre;
}

void mostrarInfoEspacioLibre(void *esp_libre)
{
	printf("Dirección física: %p\n", ((t_esp_libre *) esp_libre)-> dir);
	printf("Tamaño del bloque: %d\n", ((t_esp_libre *) esp_libre)-> size);

	return;
}

void eliminarEspacioLibre(void *esp_libre)
{
	free((t_esp_libre *) esp_libre);

	return;
}

t_segmento *crearSegmento(int32_t prog_id, int32_t seg_id, int32_t size)
{
	t_segmento *seg = malloc(sizeof(t_segmento));
	seg->prog_id = prog_id;
	seg->seg_id = seg_id;
	seg->inicio = NULL;	//esto debe ser aleatorio y lo debe decidir la umv
	seg->size = size;
	seg->pos_mem_ppal = (void *) seg;	//la dirección donde comienza este segmento

	return seg;
}

void eliminarSegmento(void *seg)
{
	free((t_segmento *) seg);

	return;
}

void mostrarInfoSegmento(void *seg){
	printf("ID Programa: %d\n", ((t_segmento *) seg)->prog_id);
	printf("ID Segmento: %d\n", ((t_segmento *) seg)->seg_id);
	printf("Dir. inicio: %p\n", ((t_segmento *) seg)->inicio);
	printf("Tamaño: %d bytes\n", ((t_segmento *) seg)->size);
	printf("Pos. Memoria Ppal.: %p\n", ((t_segmento *) seg)->pos_mem_ppal);

	return;
}
