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
			printf("\tTamaño memoria principal: %d bytes.\n", tamanio_mem_ppal);
			printf("\t\tDesde %p hasta %p.\n", mem_ppal, (mem_ppal + tamanio_mem_ppal));
		} else if(strcmp(comando,"dump-segmentos") == 0){
			dump_segmentos(listaSegmentos);
		} else if(strcmp(comando,"crear-segmento") == 0){
			crear_segmento(listaSegmentos, mem_ppal, tamanio_mem_ppal);
		} else if(strcmp(comando,"dump-all") == 0){
			printf("UMV> Falta implementar el comando \"dump-all\".\n");
		} else if (strcmp(comando,"escribir") == 0) {
			printf("UMV> Falta implementar el comando \"escribir\".\n");
		} else if (strcmp(comando,"h") == 0) {
			printf("UMV> Falta implementar la ayuda. Perdón :(\n");
		} else {
			printf("UMV> Comando no reconocido. Ingrese 'h' para ver la lista de comandos disponibles.\n");
		}

		free(comando);
	}

	printf("UMV> Adiós!\n");
	return 0;
	pthread_exit(NULL);
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

void dump_segmentos(t_list *listaSegmentos)
{
	printf("\n%-15s%-15s%-15s%-20s%-20s\n","ID Programa", "ID Segmento", "Dir. Inicio", "Tamaño (bytes)", "Posicion en mem_ppal");
	list_iterate(listaSegmentos, mostrarInfoSegmento);
	printf("\n\n");

	return;
}

void crear_segmento(t_list *listaSegmentos, void *mem_ppal, uint32_t tamanio_mem_ppal)
{
	t_list *espacios_libres = buscarEspaciosLibres(listaSegmentos, mem_ppal, tamanio_mem_ppal);
	t_segmento *seg = crearSegmento(99,10,espacios_libres,listaSegmentos,"first-fit");
	
	list_add(listaSegmentos, seg);

	if (!list_is_empty(espacios_libres)) {
		list_clean_and_destroy_elements(espacios_libres, eliminarEspacioLibre);
	}

	list_destroy(espacios_libres);

	return;
}