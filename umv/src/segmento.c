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
	void *posicion_segmento_actual=NULL;
	void *fin_segmento_actual = NULL;

	//Ordeno la lista de segmentos de menor a mayor de acuerdo a su dirección física
	list_sort(segmentos, comparador_segmento_direccion_fisica_asc);

	if(cant_segmentos == 0){
	//Si no hay segmentos, entonces toda la memoria está disponible
		list_add(lista_esp_libre, (void *) crearInstanciaEspLibre(mem_ppal, size_mem_ppal));
		return lista_esp_libre;
	}

	if(cant_segmentos > 0){
		i = 0;
		tamanio = 0;
		seg_actual = list_get(segmentos, i);
		
		posicion_segmento_actual = seg_actual->pos_mem_ppal;
		fin_segmento_actual = posicion_segmento_actual + seg_actual->size - 1;
		
		//Si hay espacio entre el principio de la memoria y el principio del primer segmento.
		if(posicion_segmento_actual > mem_ppal){
			tamanio = posicion_segmento_actual - mem_ppal;
			list_add(lista_esp_libre, (void *) crearInstanciaEspLibre(mem_ppal, tamanio));
		}

		//Entra sólo si hay más de 1 segmento
		while (i < (cant_segmentos-1)) {
			seg_siguiente = list_get(segmentos, i+1);

			//Si hay espacio entre el segmento actual y el siguiente
			if((fin_segmento_actual + 1) < seg_siguiente->pos_mem_ppal){
				tamanio = seg_siguiente->pos_mem_ppal - (fin_segmento_actual);
				list_add(lista_esp_libre, (void *) crearInstanciaEspLibre((fin_segmento_actual + 1), tamanio));
			}

			i++;
			seg_actual = list_get(segmentos, i);
			posicion_segmento_actual = seg_actual->pos_mem_ppal;
			fin_segmento_actual = posicion_segmento_actual + seg_actual->size - 1;
		}

		//Si hay espacio entre el último segmento y el fin de la memoria
		tamanio = (mem_ppal + size_mem_ppal - 1) - (fin_segmento_actual);
		if(tamanio > 0)
			list_add(lista_esp_libre, (void *) crearInstanciaEspLibre((fin_segmento_actual + 1), tamanio));
	}

	return lista_esp_libre;
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
	t_esp_libre *esp = (t_esp_libre *) esp_libre;
	printf("\t%d bytes libres entre %p y %p\n", esp->size, esp->dir, (esp->size + esp->dir - 1));

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
t_segmento *crearSegmento(uint32_t prog_id,
						  uint32_t size,
						  t_list *espacios_libres,	//lista de espacios libres
						  t_list *listaSegmentos,	//tabla de segmentos
						  char *algoritmo)
{
	t_esp_libre *esp_libre = NULL;
	t_segmento *seg = NULL;

	if(strcasecmp(algoritmo, "first-fit") == 0) {
		//ordenar la lista de espacio libre de menor a mayor por cantidad de espacio libre
		list_sort(espacios_libres, comparador_esp_libre_tamanio_asc);
		
		//tomar el primer elemento que pueda contner el segmento requerido y crear el segmento ahí
		esp_libre = buscar_primer_lugar_adecuado(espacios_libres, size);
		
		//si no hay ninguno que pueda hacerlo, no hay espacio libre para el nuevo segmento.
		if(esp_libre == NULL){
			printf("UMV> El programa con ID %d quiere crear un segmento de %d bytes, pero no hay espacio.\n", prog_id, size);
			return seg;
		}

	} else if (strcasecmp(algoritmo, "worst-fit") == 0) {
		//ordenar la lista de espacio libre de mayor a menor por cantidad de espacio libre
		//tomar el primer elemento de la lista y si contiene espacio suficiente, crear segmento ahí
		//si no tiene espacio suficiente, no hay espacio libre para el nuevo segmento.
		return seg;
	} else {
		printf("No encontré el algoritmo de asignación.\n");
		return seg;
	}

	seg = malloc(sizeof(t_segmento));
	seg->size = size;
	
	seg->prog_id = prog_id;
	seg->seg_id = getSegId(listaSegmentos);	//calcular en funcion del id de programa
	seg->inicio = 0;	//esto debe ser aleatorio y lo debe decidir la umv. Es el que conoce el programa, y no cambia.
	seg->pos_mem_ppal = esp_libre->dir;	//la dirección donde comienza este segmento
	seg->marcadoParaBorrar = 0;

	return seg;
}

void eliminarSegmento(void *seg)
{
	free((t_segmento *) seg);

	return;
}

void mostrarInfoSegmento(void *seg){
	printf("\n%-15d%-15d%-15d%-20d%-20p", ((t_segmento *) seg)->prog_id, 
										  ((t_segmento *) seg)->seg_id,
										  ((t_segmento *) seg)->inicio,
										  ((t_segmento *) seg)->size,
										  ((t_segmento *) seg)->pos_mem_ppal
	);

	return;
}

bool comparador_segmento_direccion_fisica_asc(void *seg_a, void *seg_b)
{
	t_segmento *a = (t_segmento *) seg_a;
	t_segmento *b = (t_segmento *) seg_b;

	return ((a->pos_mem_ppal) < (b->pos_mem_ppal));
}

bool comparador_segmento_tamanio_asc(void *seg_a, void *seg_b)
{
	t_segmento *a = (t_segmento *) seg_a;
	t_segmento *b = (t_segmento *) seg_b;

	return ((a->size) < (b->size));
}

bool comparador_esp_libre_tamanio_asc(void *esp_a, void *esp_b)
{
	t_esp_libre *a = (t_esp_libre *) esp_a;
	t_esp_libre *b = (t_esp_libre *) esp_b;

	return ((a->size) < (b->size));
}

uint32_t getSegId(t_list *listaSegmentos)
{

	return 0;
}

/*
**	Útil sólo para first-fit, ya que para worst-fit la lista ya está ordenada de mayor a menor
**	y si el tamaño solicitado no entra en el primer espacio, no entrará en ninguno.
**	Se podría llamar para worst-fit, pero recorrería toda la lista y devolvería NULL al no encontrar
**	ningún espacio libre.
*/ 
t_esp_libre *buscar_primer_lugar_adecuado(t_list *espacios_libres, uint32_t size_requerido)
{
	uint32_t i;
	uint32_t tope = list_size(espacios_libres);
	
	t_esp_libre *encontrado = NULL;
	t_esp_libre *candidato = NULL;

	for(i = 0; i < tope; i++) {
		candidato = list_get(espacios_libres, i);
		if(candidato->size >= size_requerido){
			encontrado = candidato;
			return encontrado;
		}
	}

	return encontrado;
}

void dump_segmentos(t_list *listaSegmentos)
{
	printf("\n%-15s%-15s%-15s%-20s%-20s\n","ID Programa", "ID Segmento", "Dir. Inicio", "Tamaño (bytes)", "Posicion en mem_ppal");
	list_sort(listaSegmentos, comparador_segmento_direccion_fisica_asc);
	list_iterate(listaSegmentos, mostrarInfoSegmento);
	printf("\n\n");

	return;
}

void destruirSegmentos(t_list *listaSegmentos, uint32_t prog_id)
{
	int i, marcados = 0, sizeLista = 0;
	sizeLista = list_size(listaSegmentos);
	t_segmento *s = NULL;

	for(i = 0; i < sizeLista; i++){
		s = (t_segmento *) list_get(listaSegmentos, i);

		if(s->prog_id == prog_id){
			marcados++;
			s->marcadoParaBorrar = 1;
		}
	}

	while(marcados){
		list_remove_and_destroy_by_condition(listaSegmentos, marcado_para_borrar, eliminarSegmento);
		marcados--;
	}

	return;
}

void *marcar_para_borrar(void *seg)
{
	t_segmento *s = (t_segmento *) seg;
	s->marcadoParaBorrar = 1;

	return;
}

bool marcado_para_borrar(void *seg)
{
	return ((t_segmento *) seg)->marcadoParaBorrar;
}
