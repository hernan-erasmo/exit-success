#include <stdlib.h>
#include <stdio.h>

#include "segmento.h"

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
