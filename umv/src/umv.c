#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>

#include "segmento.h"

int crearLogger(t_log **logger);
int cargarConfig(t_config **config, char *path);

int main(int argc, char *argv[])
{
	int errorLogger = 0;
	int errorConfig = 0;

	//Variables para el logger
	t_log *logger = NULL;

	//Variables para la carga de la configuración
	t_config *config = NULL;

	//Variables para el manejo de la memoria principal
	int32_t tamanio_mem_ppal = 0;
	void *mem_ppal = NULL;

	//Variables para las listas de segmentos
	t_list *listaSegmentos = NULL;
	int32_t total_segmentos = 0;
	int i;

	//Variables para la administración de memoria
	t_list *lista_esp_libre = NULL;

	errorLogger = crearLogger(&logger);
	errorConfig = cargarConfig(&config, argv[1]);

	if(errorLogger || errorConfig){
		goto liberarRecursos;
		return EXIT_FAILURE;
	}

	//Cargo los valores desde la configuración
	tamanio_mem_ppal = config_get_int_value(config, "TAMANIO_MEM_PPAL_BYTES");
	log_info(logger, "TAMANIO_MEM_PPAL_BYTES = %d", tamanio_mem_ppal);
	log_info(logger, "ALGORITMO_COMPACTACION = %s", config_get_string_value(config, "ALGORITMO_COMPACTACION"));

	//Reservo memoria para la memoria principal
	mem_ppal = malloc(tamanio_mem_ppal);
	memset(mem_ppal, '\0', tamanio_mem_ppal);
	log_info(logger, "La memoria principal abarca desde la dirección %p hasta %p", (mem_ppal), (mem_ppal + tamanio_mem_ppal));

	//Inicializo una lista para los segmentos
	log_info(logger, "Creo la lista de segmentos usando list_create()");
	listaSegmentos = list_create();

	log_info(logger, "Veo si hay espacio libre en memoria");
	lista_esp_libre = buscarEspaciosLibres(listaSegmentos, mem_ppal, tamanio_mem_ppal);

	log_info(logger, "Encontré los siguientes espacios libres en memoria:");
	list_iterate(lista_esp_libre, mostrarInfoEspacioLibre);

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

	if(lista_esp_libre) {
		if (!list_is_empty(lista_esp_libre)) {
			list_clean_and_destroy_elements(lista_esp_libre, eliminarEspacioLibre);
		}

		list_destroy(lista_esp_libre);
	}

	if(listaSegmentos) {
		if (!list_is_empty(listaSegmentos)) {
			list_clean_and_destroy_elements(listaSegmentos, eliminarSegmento);
		}
	
		list_destroy(listaSegmentos);
	}

	if(config)
		config_destroy(config);

	if(mem_ppal) {
		free(mem_ppal);
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

int cargarConfig(t_config **config, char *path)
{
	if(path == NULL) {
		printf("No se pudo cargar el archivo de configuración.\n");
		return 1;
	}

	*config = config_create(path);
	return 0;	
}
