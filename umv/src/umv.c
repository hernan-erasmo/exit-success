#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "umv.h"

static uint32_t PROCESO_ACTIVO = 0;
static pthread_mutex_t modificando_proceso_activo = PTHREAD_MUTEX_INITIALIZER;

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
	pid_t umv_pid;
	int noTerminar = 1;

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

	retardo_ms = 0;

	//Cargo los valores desde la configuración
	tamanio_mem_ppal = config_get_int_value(config, "TAMANIO_MEM_PPAL_BYTES");
	algoritmo_comp = config_get_string_value(config, "ALGORITMO_COMPACTACION");
	puerto = config_get_string_value(config, "PUERTO");
	retardo_ms = config_get_long_value(config, "RETARDO");
	inicializarTiempoRetardo(&retardo, retardo_ms);

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
	inicializarConfigConsola(&c_init, tamanio_mem_ppal, mem_ppal, listaSegmentos, algoritmo_comp, &listenningSocket, &noTerminar, config_get_int_value(config, "PUERTO"), retardo);

	// Arranca la consola
	if(pthread_create(&threadConsola, NULL, consola, (void *) c_init)) {
		log_error(logger, "[UMV] Error al crear el thread de la consola. Motivo: %s", strerror(errno));
		goto liberarRecursos;
		return EXIT_FAILURE;
	}	

	log_info(logger, "[UMV] Creando el socket que escucha conexiones entrantes.");
	if(crear_conexion_entrante(&listenningSocket, puerto, &serverInfo, logger, "UMV") != 0){
		goto liberarRecursos;
		pthread_exit(NULL);
	}

/*
**	Acá debería escuchar y delegar las conexiones a threads
*/

	while(noTerminar){
		log_info(logger, "[UMV] Esperando conexiones entrantes...");

		int *socketNuevo = malloc(sizeof(int));
		pthread_t *hiloAtencion = malloc(sizeof(pthread_t));
		
		t_param_memoria *param_memoria = malloc(sizeof(t_param_memoria));
			param_memoria->listaSegmentos = listaSegmentos;
			param_memoria->mem_ppal = mem_ppal;
			param_memoria->tamanio_mem_ppal = tamanio_mem_ppal;
			param_memoria->algoritmo_comp = algoritmo_comp;
			
		t_config_conexion *conf_con = malloc(sizeof(t_config_conexion));
			conf_con->socket = socketNuevo;
			conf_con->parametros_memoria = param_memoria;
			conf_con->logger = logger;
			conf_con->hilo = hiloAtencion;

		if((*socketNuevo = accept(listenningSocket, (struct sockaddr *) &addr, &addrlen)) != -1){
			if(noTerminar == 0){ //La conexión viene de la consola y tengo que salir.
				free(socketNuevo);
				free(hiloAtencion);
				free(param_memoria);
				free(conf_con);
				break;
			}	
			pthread_create(hiloAtencion, 0, atencionConexiones, (void *) conf_con);
			pthread_detach(*hiloAtencion);
		} else {
			log_error(logger, "[UMV] Error al aceptar una nueva conexión.");
			continue;
		}
	}

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

	if ((*logger = log_create(nombreArchivoLog,"UMV",false,LOG_LEVEL_DEBUG)) == NULL) {
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

void inicializarConfigConsola(t_consola_init **c_init, uint32_t sizeMem, void *mem, t_list *listaSegmentos, char *algoritmo_comp, int *listenningSocket, int *noTerminar, int puerto, struct timespec *ret)
{
	*c_init = malloc(sizeof(t_consola_init));
	(*c_init)->listaSegmentos = listaSegmentos;
	(*c_init)->tamanio_mem_ppal = sizeMem;
	(*c_init)->mem_ppal = mem;
	(*c_init)->algoritmo_comp = algoritmo_comp;
	(*c_init)->umv_pid = getpid();
	(*c_init)->listenningSocket = listenningSocket;
	(*c_init)->noTerminar = noTerminar;
	(*c_init)->puerto = puerto;
	(*c_init)->ret = ret;

	return;
}

uint32_t compactar(t_list *segmentos, void *mem_ppal, uint32_t size_mem_ppal)
{
	nanosleep(retardo, NULL);

	int i, cant_segmentos = list_size(segmentos);
	uint32_t espacio_liberado = 0;
	t_segmento *seg_actual = NULL;
	t_segmento *seg_siguiente = NULL;

	void *pos_seg_actual = NULL;
	void *fin_seg_actual = NULL;
	void *aux = NULL;

	if(cant_segmentos == 0){
		return espacio_liberado;
	}

	//Ordeno los segmentos de menor a mayor según dirección física
	list_sort(segmentos, comparador_segmento_direccion_fisica_asc);

	i = 0;
	seg_actual = list_get(segmentos, i);
	pos_seg_actual = seg_actual->pos_mem_ppal;
	fin_seg_actual = pos_seg_actual + seg_actual->size - 1;
	//actualizarInfoSegmento(&pos_seg_actual, &fin_seg_actual, seg_actual);
	
	//Si el primer segmento NO está pegado al comienzo de la memoria, lo mueve ahí
	if(seg_actual->pos_mem_ppal > mem_ppal){
		espacio_liberado += (seg_actual->pos_mem_ppal - mem_ppal);
		aux = memmove(mem_ppal, seg_actual->pos_mem_ppal, seg_actual->size);
		seg_actual->pos_mem_ppal = aux;

		pos_seg_actual = seg_actual->pos_mem_ppal;
		fin_seg_actual = pos_seg_actual + seg_actual->size - 1;		
		//actualizarInfoSegmento(&pos_seg_actual, &fin_seg_actual, seg_actual);
	}

	//Si existe más de un segmento, va moviéndolos juntos si es que no están pegados.
	while (i < (cant_segmentos - 1)) {
		seg_siguiente = list_get(segmentos, i+1);

		//No están pegados
		if((fin_seg_actual + 1) < seg_siguiente->pos_mem_ppal){
			espacio_liberado += (seg_siguiente->pos_mem_ppal - (fin_seg_actual + 1));
			aux = memmove(fin_seg_actual + 1, seg_siguiente->pos_mem_ppal, seg_siguiente->size);
			seg_siguiente->pos_mem_ppal = aux;
		}

		i++;
		seg_actual = seg_siguiente;
		
		pos_seg_actual = seg_actual->pos_mem_ppal;
		fin_seg_actual = pos_seg_actual + seg_actual->size - 1;		
		//actualizarInfoSegmento(&pos_seg_actual, &fin_seg_actual, seg_actual);
		
	}

	return espacio_liberado;
}

int enviar_bytes(t_list *listaSegmentos, uint32_t base, uint32_t offset, uint32_t tamanio, void *buffer)
{
	nanosleep(retardo, NULL);
	
	int cod_retorno = tamanio;
	int escritura_valida = 0;

	t_segmento *seg = buscar_segmento_solicitado(listaSegmentos, base);

	if(seg == NULL){
		cod_retorno = -1;	//Segmentation fault. El proceso activo no tiene un segmento con esa base.
		return cod_retorno;
	}
	
	if(escritura_valida = chequear_limites_lectoescritura(seg, offset, tamanio)){
		void *dest = seg->pos_mem_ppal + offset;
		if(buffer == NULL)
			//si buffer llega a ser null, porque strtok es una mierda que retorna null con dos token seguidos,
			//entonces este memset capaz nos salva las papas.
			//Confirmadísimo. Esta línea salva vidas.
			memset(dest, '\0', tamanio);
		else
			memcpy(dest, buffer, tamanio);	//este debería ser el curso normal de ejecución
	} else {
		cod_retorno = -2;	//Segmentation fault. Se quiso escribir por fuera de los límites de la memoria del segmento
	}	

	return cod_retorno;
}

t_segmento *buscar_segmento_solicitado(t_list *listaSegmentos, uint32_t base)
{
	int i, size_activos = 0;

	t_list *segmentosActivos = list_filter(listaSegmentos, buscar_por_proceso_activo);
	size_activos = list_size(segmentosActivos);
	if(size_activos == 0){
		//No se encontró un segmento para el proceso activo
		list_destroy(segmentosActivos);
		return NULL;
	}

	t_segmento *seg = NULL;
	int encontrado = 0;
	for(i = 0; i < size_activos; i++){
		seg = list_get(segmentosActivos, i);
		
		if(seg->inicio == base){
			list_destroy(segmentosActivos);
			return seg;
		}
		
	}

	//No se encontró un segmento con base [base] en la lista de segmentos del proceso activo
	list_destroy(segmentosActivos);
	return NULL;
}

bool buscar_por_proceso_activo(void *seg)
{
	t_segmento *segmento = (t_segmento *) seg;

	return(segmento->prog_id == get_proceso_activo());
}

int chequear_limites_lectoescritura(t_segmento *seg, uint32_t offset, uint32_t tamanio)
{
	uint32_t comienzo_segmento = seg->inicio;
	uint32_t fin_segmento = comienzo_segmento + seg->size - 1;
	uint32_t pos_comienzo_lectoescritura = comienzo_segmento + offset;
	uint32_t pos_fin_lectoescritura = pos_comienzo_lectoescritura + tamanio - 1;

	//Quiero leer/escribir fuera del fin de segmento
	if(pos_comienzo_lectoescritura > fin_segmento){
		return 0;
	}

	//Quiero leer/escribir un tamanio que va a terminar afuera del segmento
	if(pos_fin_lectoescritura > fin_segmento){
		return 0;
	}

	return 1;
}

uint32_t cambiar_proceso_activo(uint32_t proceso_activo)
{
	pthread_mutex_lock(&modificando_proceso_activo);
		PROCESO_ACTIVO = proceso_activo;
	pthread_mutex_unlock(&modificando_proceso_activo);

	return PROCESO_ACTIVO;
}

uint32_t get_proceso_activo()
{
	return PROCESO_ACTIVO;
}

void *solicitar_bytes(t_list *listaSegmentos, uint32_t base, uint32_t offset, uint32_t *tamanio)
{
	nanosleep(retardo, NULL);

	char *retorno = NULL;
	char *noHaySegmento = "-1";
	char *violacionLimite = "-2";

	
	int lectura_valida = 0;

	t_segmento *seg = buscar_segmento_solicitado(listaSegmentos, base);

	if(seg == NULL){
		printf("#ERROR#	El proceso activo %d no tiene un segmento con base %d\n", PROCESO_ACTIVO, base);
		return ((void *) noHaySegmento);
	}
	
	if(lectura_valida = chequear_limites_lectoescritura(seg, offset, *tamanio)){
		retorno = calloc(*tamanio, 1);
		memcpy(retorno, seg->pos_mem_ppal + offset, *tamanio);
	} else {
		printf("#ERROR#	Se quiso leer por fuera de los límites del segmento con base %d, proceso activo: %d\n", base, PROCESO_ACTIVO);
		return ((void *) violacionLimite);
	}	

	return (void *) retorno;
}

void inicializarTiempoRetardo(struct timespec **r, uint32_t retardo_ms)
{
	int segundos = 0;
	long nanosegundos = 0;

	*r = malloc(sizeof(struct timespec));

	if(retardo_ms > 999){
		segundos = (retardo_ms / 1000);
		nanosegundos = (retardo_ms % 1000) * 1000000;	//1 ms = 1.000.000 ns
	} else {
		nanosegundos = retardo_ms * 1000000;
	}

	(*r)->tv_sec = (time_t) segundos;
	(*r)->tv_nsec = nanosegundos;

	return;
}
