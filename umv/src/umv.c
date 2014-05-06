#include <stdio.h>
#include <stdlib.h>

#include <commons/log.h>
#include <commons/collections/list.h>

#include "segmento.h"

int crearLogger(t_log **logger);

int main(int argc, char *argv[])
{
	int errorLogger = 0;

	//Variables para el logger
	t_log *logger = NULL;

	//Variables para las listas de segmentos
	t_list *listaSegmentos = NULL;
	int32_t total_segmentos = 0;
	int i;

	errorLogger = crearLogger(&logger);

	if(errorLogger){
		goto liberarRecursos;
		return EXIT_FAILURE;
	}

	//Inicializo una lista para los segmentos
	log_info(logger, "Creo la lista usando list_create()");
	listaSegmentos = list_create();

	//Creo segmentos y los agrego a la lista
	log_info(logger, "Comienzo a crear los segmentos");
	for(i = 0; i < 3; i++) {
		total_segmentos += list_add(listaSegmentos, (void *) crearSegmento(1,i,i + 10));
		log_info(logger, "Creé un segmento!");
	}
	
	//Imprimo los datos de los segmentos por pantalla
	log_info(logger, "Se crearon %d segmentos:", total_segmentos);
	list_iterate(listaSegmentos, mostrarInfoSegmento);

	log_info(logger, "Terminé de iterar usando list_iterate()");
	log_info(logger, "Chau!");

	goto liberarRecursos;
	return EXIT_SUCCESS;

liberarRecursos:
	if(logger)
		log_destroy(logger);

	if(listaSegmentos) {
		if (!list_is_empty(listaSegmentos)) {
			list_clean_and_destroy_elements(listaSegmentos, eliminarSegmento);
		}
	
		list_destroy(listaSegmentos);
	}
		

}

int crearLogger(t_log **logger)
{
	char *nombreArchivoLog = "umv_log";

	if ((*logger = log_create(nombreArchivoLog,"UMV",true,LOG_LEVEL_DEBUG)) == NULL) {
		printf("No se pudo inicializar un logger.\n");
		return 1;
	}

	return 0;
}