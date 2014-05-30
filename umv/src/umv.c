#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "umv.h"

int main(int argc, char *argv[])
{
	int errorLogger = 0;
	int errorConfig = 0;

	//Variables para el logger
	t_log *logger = NULL;

	//Variables para la carga de la configuración
	t_config *config = NULL;

	//Variables para el manejo de la memoria principal
	uint32_t tamanio_mem_ppal = 0;
	void *mem_ppal = NULL;
	char *algoritmo_comp = NULL;

	//Variables para el manejo de los hilos
	pthread_t threadConsola;
	t_consola_init *c_init;

	//Variables para las listas de segmentos
		/*
		** OJO CON LA SINCRONIZACIÓN DE ESTAS VARIABLES, ya
		** que a ellas acceden simultáneamente varios hilos,
		** incluído el que maneja la consola.
		*/
	t_list *listaSegmentos = NULL;
	uint32_t total_segmentos = 0;

	//Variables para la administración de memoria
	t_list *lista_esp_libre = NULL;

	//Variables para las conexiones por sockets
	char *puerto;
	int listenningSocket = -1;
	int socketCliente = -1;
	struct addrinfo *serverInfo = NULL;
	struct sockaddr_in addr; // Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	socklen_t addrlen = sizeof(addr);
	int status = 1;

	errorLogger = crearLogger(&logger);
	errorConfig = cargarConfig(&config, argv[1]);

	if(errorLogger || errorConfig){
		goto liberarRecursos;
		return EXIT_FAILURE;
	}

	//Cargo los valores desde la configuración
	tamanio_mem_ppal = config_get_int_value(config, "TAMANIO_MEM_PPAL_BYTES");
	algoritmo_comp = config_get_string_value(config, "ALGORITMO_COMPACTACION");
	puerto = config_get_string_value(config, "PUERTO");
	log_info(logger, "TAMANIO_MEM_PPAL_BYTES = %d", tamanio_mem_ppal);
	log_info(logger, "ALGORITMO_COMPACTACION = %s", algoritmo_comp);
	log_info(logger, "PUERTO = %s", puerto);

	//Reservo memoria para la memoria principal
	if((mem_ppal = inicializarMemoria(tamanio_mem_ppal)) == NULL){
		log_error(logger, "[UMV] No se pudo crear la memoria principal.");
		goto liberarRecursos;
		return EXIT_FAILURE;
	} else {
		log_info(logger, "[UMV] La memoria principal abarca desde la dirección %p hasta %p", (mem_ppal), (mem_ppal + tamanio_mem_ppal - 1));
	}
	
	//Inicializo una lista para los segmentos
	log_info(logger, "[UMV] Creo la lista de segmentos usando list_create()");
	listaSegmentos = list_create();

	//Inicializo la configuración de la consola
	inicializarConfigConsola(&c_init, tamanio_mem_ppal, mem_ppal, listaSegmentos, algoritmo_comp);

	// Arranca la consola
	if(pthread_create(&threadConsola, NULL, consola, (void *) c_init)) {
		log_error(logger, "[UMV] Error al crear el thread de la consola. Motivo: %s", strerror(errno));
		goto liberarRecursos;
		return EXIT_FAILURE;
	}	

	log_info(logger, "[UMV] Creando el socket que escucha conexiones entrantes.");
	if(init_sock_escucha(&listenningSocket, puerto, &serverInfo, logger) != 0){
		goto liberarRecursos;
		pthread_exit(NULL);
	}


	log_info(logger, "[UMV] Esperando conexión del PLP (Kernel)");
	while(1){
		socketCliente = accept(listenningSocket, (struct sockaddr *) &addr, &addrlen);
		log_info(logger, "[UMV] Recibí una conexión!");

		t_paquete_programa paquete;

		status = recvAll(&paquete, socketCliente);
		if(status){
			switch(paquete.id)
			{
				case 'K':
					log_info(logger, "[UMV] Se conectó el Kernel (hilo PLP)");
					//log_info(logger, "[UMV] Un programa se conectó. Va a enviar %d bytes de datos.", paquete.tamanio_total);
					//printf("%s", paquete.mensaje);

					/*
					**	TIRÁ UN HILO PARA ATENDER EL PLP, SALI DE ESTE WHILE Y PONETE A ESCUCHAR CPUs
					*/

					break;
				default:
					log_info(logger, "[UMV] No es una conexión del kernel.");
			}
		}

		if(paquete.mensaje)
			free(paquete.mensaje);

		//Salimos de este bucle (con la conexión al plp o con un intento de conexión fallido)
		break;
	}	

/*
**	Acá debería empezar a escuchar por socket las conexiones entrantes de las CPUs
*/

	log_info(logger, "Terminé de iterar usando list_iterate()");
	log_info(logger, "Chau!");

	pthread_join(threadConsola, NULL);
	log_info(logger, "El thread de la consola finalizó y retornó status: (falta implementar!)");
	
	goto liberarRecursos;
	return EXIT_SUCCESS;

liberarRecursos:
	if(logger)
		log_destroy(logger);

	if(c_init)
		free(c_init);

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

	if(serverInfo != NULL)
		freeaddrinfo(serverInfo);

	if(listenningSocket != -1)
		close(listenningSocket);

	if(socketCliente != -1)
		close(socketCliente);
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

void *inicializarMemoria(uint32_t size)
{
	void *mem = malloc(size);
	memset(mem, '\0', size);

	return mem;
}

void inicializarConfigConsola(t_consola_init **c_init, uint32_t sizeMem, void *mem, t_list *listaSegmentos, char *algoritmo_comp)
{
	*c_init = malloc(sizeof(t_consola_init));
	(*c_init)->listaSegmentos = listaSegmentos;
	(*c_init)->tamanio_mem_ppal = sizeMem;
	(*c_init)->mem_ppal = mem;
	(*c_init)->algoritmo_comp = algoritmo_comp;

	return;
}

int init_sock_escucha(int *listenningSocket, char *puerto, struct addrinfo **serverInfo, t_log *logger)
{
	struct addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// No importa si uso IPv4 o IPv6
	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	// Notar que le pasamos NULL como IP, ya que le indicamos que use localhost en AI_PASSIVE
	if (getaddrinfo(NULL, puerto, &hints, serverInfo) != 0) {
		log_error(logger,"[UMV] No se pudo crear la estructura addrinfo. Motivo: %s", strerror(errno));
		return 1;
	}
	
	if ((*listenningSocket = socket((*serverInfo)->ai_family, (*serverInfo)->ai_socktype, (*serverInfo)->ai_protocol)) < 0) {
		log_error(logger, "[UMV] Error al crear socket. Motivo: %s", strerror(errno));
		return 1;
	}
	log_info(logger, "[UMV] Se creó el socket a la escucha del puerto: %s", puerto);

	if(bind(*listenningSocket,(*serverInfo)->ai_addr, (*serverInfo)->ai_addrlen)) {
		log_error(logger, "[UMV] No se pudo bindear el socket a la dirección. Motivo: %s", strerror(errno));
		return 1;
	}

	listen(*listenningSocket, 10);

	return 0;
}
