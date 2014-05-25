#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "consola.h"

void *consola(void *consola_init)
{
	t_consola_init *c_init = (t_consola_init *) consola_init;
	uint32_t tamanio_mem_ppal = c_init->tamanio_mem_ppal;
	void *mem_ppal = c_init->mem_ppal;
	t_list *listaSegmentos = c_init->listaSegmentos;

	char *comando = NULL;
	int noSalir = 1;

	/*
	** Bucle principal de la consola
	*/
	while(noSalir){
		printf("UMV> Ingrese un comando: ");
		comando = getLinea();

		if(strcmp(comando,"q") == 0){
			noSalir = 0;
		
		} else if(strcmp(comando,"info") == 0){
			comando_info_memoria(mem_ppal, listaSegmentos, tamanio_mem_ppal);
		
		} else if(strcmp(comando,"crear-segmento") == 0){
			comando_crear_segmento(listaSegmentos, mem_ppal, tamanio_mem_ppal);
		
		} else if(strcmp(comando,"destruir-segmentos") == 0){
			comando_destruir_segmentos(listaSegmentos);
		
		} else if(strcmp(comando,"dump-all") == 0){
			printf("UMV> Falta implementar el comando \"dump-all\".\n");
		
		} else if(strcmp(comando,"dump-segmentos") == 0){
			comando_dump_segmentos(listaSegmentos);
		
		} else if (strcmp(comando,"escribir") == 0) {
			printf("UMV> Falta implementar el comando \"escribir\".\n");
		
		} else if (strcmp(comando,"h") == 0) {
			printf("UMV> Falta implementar la ayuda. Perdón :(\n");
		
		} else if (strlen(comando) == 0) {
			//para cuando se presiona 'enter' sin ningún caracter, repite el prompt indefinidamente.
		
		} else {
			printf("UMV> No entiendo \"%s\". Ingrese 'h' para ver la lista de comandos disponibles.\n", comando);
		}

		free(comando);
	}

	printf("UMV> Adiós!\n");
	return 0;
	pthread_exit(NULL);
}

void comando_crear_segmento(t_list *listaSegmentos, void *mem_ppal, uint32_t tamanio_mem_ppal)
{
	uint32_t id_prog = 0;
	uint32_t tamanio_segmento = 0;

	printf("\tID de programa (uint32_t): ");
	scanf("%d", &id_prog);
	printf("\tTamaño (uint32_t): ");
	scanf("%d", &tamanio_segmento);

	if(!tamanio_segmento){
		printf("UMV> No se puede crear un segmento de tamaño 0.\n");
		fgetc(stdin);
		return;
	}

	t_list *espacios_libres = buscarEspaciosLibres(listaSegmentos, mem_ppal, tamanio_mem_ppal);
	t_segmento *seg = crearSegmento(id_prog,tamanio_segmento,espacios_libres,listaSegmentos,"first-fit");
	
	if(seg != NULL){
		list_add(listaSegmentos, seg);
		printf("UMV> Se creó el segmento.\n");
	} else {
		printf("UMV> No se pudo crear el segmento.\n");
	}

	if (!list_is_empty(espacios_libres)) {
		list_clean_and_destroy_elements(espacios_libres, eliminarEspacioLibre);
	}

	//Si no hago este fgetc() entonces se repite dos veces el prompt de UMV> cuando retorna al bucle principal.
	fgetc(stdin);

	list_destroy(espacios_libres);

	return;
}

void comando_destruir_segmentos(t_list *listaSegmentos)
{
	uint32_t progId = 0;
	
	printf("\tIngrese el ID del programa: ");
	scanf("%d", &progId);
	
	//Si no hago este fgetc() entonces se repite dos veces el prompt de UMV> cuando retorna al bucle principal.
	fgetc(stdin);
	
	destruirSegmentos(listaSegmentos, progId);
	
	return;
}

void comando_dump_segmentos(t_list *listaSegmentos)
{
	int id_prog, opcion = 0;
	t_list *segmentos = NULL;

	printf("\t1) Por ID de programa.\n");
	printf("\t2) Todos.\n");
	printf("\tOpción: ");
	scanf("%d", &opcion);
	
	if(opcion == 1){
		printf("\t\tIngrese ID de programa: ");
		scanf("%d", &id_prog);
		segmentos = buscarSegmentosConId(listaSegmentos, id_prog);

		if(segmentos != NULL){
			dump_segmentos(segmentos);
			list_destroy(segmentos);
		} else {
			printf("UMV> No hay segmentos con id de programa: %d\n", id_prog);
		}
	
	} else if (opcion == 2) {
		dump_segmentos(listaSegmentos);
	}

	//Si no hago este fgetc() entonces se repite dos veces el prompt de UMV> cuando retorna al bucle principal.
	fgetc(stdin);

	return;
}

void comando_info_memoria(void *mem_ppal, t_list *listaSegmentos, uint32_t tamanio_mem_ppal)
{
	t_list *esp_libre = buscarEspaciosLibres(listaSegmentos, mem_ppal, tamanio_mem_ppal);

	if(list_is_empty(esp_libre)){
		printf("\tNo hay espacio libre en memoria.\n");
	} else {
		list_iterate(esp_libre, mostrarInfoEspacioLibre);
	}

	list_destroy_and_destroy_elements(esp_libre, eliminarEspacioLibre);
}

char *getLinea(void)
{
	char *line = NULL;
	char *linep = NULL;
	size_t lenmax = 40;
	size_t len = 0;
	int c = 0;

	line = calloc(40,1);
	linep = line;
	len = lenmax;

	if(line == NULL)
		return NULL;

	for(;;) {
		c = fgetc(stdin);
		if(c == EOF)
			break;

		if(--len == 0) {
			len = lenmax;
			char *linen = realloc(linep, lenmax *= 2);

			if(linen == NULL){
				free(linep);
				return NULL;
			}

			line = linen + (line - linep);
			linep = linen;
		}

		if((*line++ = c) == '\n') {
			len = strlen(line) - 1;
			
			if(line[len] == '\n')
				line[len] = '\0';
			
			break;
		}
	}
	return linep;
}

t_list *buscarSegmentosConId(t_list *listaSegmentos, uint32_t id)
{
	t_list *seg = list_create();
	t_segmento *s = NULL;
	int i, size = list_size(listaSegmentos);

	for(i = 0; i < size; i++){
		s = (t_segmento *) list_get(listaSegmentos, i);

		if(s->prog_id == id)
			list_add(seg, s);
	
	}

	if(list_size(seg) == 0){
		list_destroy(seg);
		return NULL;
	}
		
	return seg;
}
