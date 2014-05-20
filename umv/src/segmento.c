#include <stdlib.h>
#include <stdio.h>

#include "segmento.h"

t_list *buscarEspaciosLibres(t_list *segmentos, void *mem_ppal, uint32_t size_mem_ppal)
{
	int cant_segmentos = list_size(segmentos);
	t_list *lista_esp_libre = list_create();
	t_segmento *seg_actual = NULL;
	t_segmento *seg_siguiente = NULL;
	int i = 0;
	int tamanio = 0;

	//Ordeno la lista de segmentos de menor a mayor de acuerdo a su dirección física
	//reordenarListaSegmentos(segmentos);

	if(cant_segmentos == 0){
	//Si no hay segmentos, entonces toda la memoria está disponible
		list_add(lista_esp_libre, (void *) crearInstanciaEspLibre(mem_ppal, size_mem_ppal));
		return lista_esp_libre;
	}

	if(cant_segmentos > 0){
		i = 0;
		tamanio = 0;
		seg_actual = list_get(segmentos, i);
		
		//Si hay espacio entre el principio de la memoria y el principio del primer segmento.
		if(seg_actual->pos_mem_ppal > mem_ppal){
			tamanio = (seg_actual->pos_mem_ppal - 1) - mem_ppal;
			list_add(lista_esp_libre, (void *) crearInstanciaEspLibre(mem_ppal, tamanio));
		}

		//Entra sólo si hay más de 1 segmento
		while (i < (cant_segmentos-1)) {
			seg_siguiente = list_get(segmentos, i+1);

			//Si hay espacio entre el segmento actual y el siguiente
			if((seg_actual->pos_mem_ppal + seg_actual->size + 1) < seg_siguiente->pos_mem_ppal){
				tamanio = seg_siguiente->pos_mem_ppal - (seg_actual->pos_mem_ppal + seg_actual->size);
				list_add(lista_esp_libre, (void *) crearInstanciaEspLibre((seg_actual->pos_mem_ppal + seg_actual->size + 1), tamanio));
			}

			i++;
			seg_actual = list_get(segmentos, i);
		}

		//Si hay espacio entre el último segmento y el fin de la memoria
		tamanio = (mem_ppal + size_mem_ppal) - (seg_actual->pos_mem_ppal + seg_actual->size);
		if(tamanio > 0)
			list_add(lista_esp_libre, (void *) crearInstanciaEspLibre((seg_actual->pos_mem_ppal + seg_actual->size + 1), tamanio));
	}

	return lista_esp_libre;
}

//Esta funcion ordena los elementos de la lista de segmentos (no crea otra nueva ordenada, reordena la lista que le pasas)
//de acuerdo a su dirección física "pos_mem_ppal" de menor a mayor.
void reordenarListaSegmentos(t_list *segmentos)
{
	list_sort(segmentos, comprarador_direccion_fisica_asc);

	return;
}

t_esp_libre *crearInstanciaEspLibre(void *mem, uint32_t size)
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

/*
**	Su funcionamiento depende del algoritmo First-Fit o Worst-Fit.
**	Busca espacio libre en memoria, elige el más adecuado de acuerdo
** 	al algoritmo de asignación y guarda ahí un nuevo segmento.
*/
t_segmento *crearSegmento(uint32_t prog_id, uint32_t size, t_list *espacio_libre)
{
	t_segmento *seg = malloc(sizeof(t_segmento));
	seg->prog_id = prog_id;
	seg->seg_id = 0;
	seg->inicio = 0;	//esto debe ser aleatorio y lo debe decidir la umv
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
	printf("Dir. inicio: %d\n", ((t_segmento *) seg)->inicio);
	printf("Tamaño: %d bytes\n", ((t_segmento *) seg)->size);
	printf("Pos. Memoria Ppal.: %p\n", ((t_segmento *) seg)->pos_mem_ppal);

	return;
}

bool comprarador_direccion_fisica_asc(void *seg_a, void *seg_b)
{
	t_segmento *a = (t_segmento *) seg_a;
	t_segmento *b = (t_segmento *) seg_b;

	return ((a->pos_mem_ppal) < (b->pos_mem_ppal));
}

bool comprarador_tamanio_asc(void *seg_a, void *seg_b)
{
	t_segmento *a = (t_segmento *) seg_a;
	t_segmento *b = (t_segmento *) seg_b;

	return ((a->size) < (b->size));
}