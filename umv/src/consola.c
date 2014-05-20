#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "consola.h"

void *consola(void *consola_init)
{
	t_consola_init *c_init = (t_consola_init *) consola_init;
	uint32_t tamanio_mem_ppal = c_init->tamanio_mem_ppal;
	void *mem_ppal = c_init->mem_ppal;

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
		} else if(strcmp(comando,"dump-all") == 0){
			printf("UMV> Falta implementar el comando \"dump-all\".\n");
		} else if (strcmp(comando,"escribir") == 0) {
			printf("UMV> Falta implementar el comando \"escribir\".\n");
		} else {
			printf("UMV> Comando inválido.\n");
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

	//*line = '\0';
	return linep;
}