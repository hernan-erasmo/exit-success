#include <errno.h>

#include "kernel.h"
#include "plp.h"

int main(int argc, char *argv[])
{
	int errorArgumentos = 0;
	int errorLogger = 0;
	int errorConfig = 0;

	//Variables de threads
	pthread_t threadPlp;
	pthread_t threadPcp;
	t_datos_plp *d_plp;
	t_datos_pcp *d_pcp;

	void *retorno = NULL;

	//Variables para el logger
	t_log *logger = NULL;

	//Variables para la carga de la configuración
	t_config *config = NULL;

	errorArgumentos = checkArgs(argc);
	errorLogger = crearLogger(&logger);
	errorConfig = cargarConfig(&config, argv[1]);

	if(errorArgumentos || errorLogger || errorConfig) {
		goto liberarRecursos;
		return EXIT_FAILURE;
	}

	cola_new = list_create();
	cola_ready = list_create();
	cola_exec = list_create();
	cola_block = list_create();
	cola_exit = list_create();
	
	multiprogramacion = config_get_int_value(config, "MULTIPROGRAMACION");
	log_info(logger, "[KERNEL] El nivel de multiprogramacion del sistema es: %d", multiprogramacion);

	tamanio_quantum = config_get_int_value(config, "QUANTUM");
	log_info(logger, "[KERNEL] El tamaño de quantum es: %d", tamanio_quantum);

	cargarInfoCompartidas(&listaCompartidas, config, logger);

	cargarInfoSemaforosAnsisop(&semaforos_ansisop, config, logger);
	if(crearHilosSemaforosAnsisop(semaforos_ansisop, logger)){
		goto liberarRecursos;
		return EXIT_FAILURE;
	}

	cargarInfoIO(&cabeceras_io, config, logger);
	if(crearHilosIO(cabeceras_io, logger)){
		goto liberarRecursos;
		return EXIT_FAILURE;
	}

	d_pcp = crearConfiguracionPcp(config, logger);
	d_plp = crearConfiguracionPlp(config, logger);

	log_info(logger, "[PLP] Inicializando el hilo PLP");
	if(pthread_create(&threadPlp, NULL, plp, (void *) d_plp)) {
		log_error(logger, "Error al crear el thread del PLP. Motivo: %s", strerror(errno));
		goto liberarRecursos;
		return EXIT_FAILURE;
	}

	log_info(logger, "[PCP] Inicializando el hilo PCP");
	if(pthread_create(&threadPcp, NULL, pcp, (void *) d_pcp)) {
		log_error(logger, "Error al crear el thread del PCP. Motivo: %s", strerror(errno));
		goto liberarRecursos;
		return EXIT_FAILURE;
	}

	pthread_detach(threadPcp);

	pthread_join(threadPlp, NULL);
	log_info(logger, "El thread PLP finalizó y retornó status: (falta implementar!)");

	goto liberarRecursos;
	return EXIT_SUCCESS;

liberarRecursos:
	if(logger)
		log_destroy(logger);

	if(config)
		config_destroy(config);

	if(list_is_empty(cola_new))
		list_destroy(cola_new);

	if(list_is_empty(cola_ready))
		list_destroy(cola_ready);

	if(list_is_empty(cola_exec))
		list_destroy(cola_exec);

	if(list_is_empty(cola_block))
		list_destroy(cola_block);

	if(list_is_empty(cola_exit))
		list_destroy(cola_exit);

	free(d_pcp);
	free(d_plp);
}

int crearLogger(t_log **logger)
{
	char *nombreArchivoLog = "kernel_log";

	if ((*logger = log_create(nombreArchivoLog,"Kernel",true,LOG_LEVEL_DEBUG)) == NULL) {
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

int checkArgs(int args)
{
	if (args != 2) {
		printf("El kernel debe recibir como parámetro únicamente la ruta al archivo de configuración.\n");
		return 1;
	}
	
	return 0;	
}

t_datos_plp *crearConfiguracionPlp(t_config *config, t_log *logger)
{
	t_datos_plp *d_plp = malloc(sizeof(t_datos_plp));
	d_plp->puerto_escucha = config_get_string_value(config, "PUERTO_PROG");
	d_plp->ip_umv = config_get_string_value(config, "IP_UMV");
	d_plp->puerto_umv = config_get_int_value(config, "PUERTO_UMV");
	d_plp->tamanio_stack = config_get_int_value(config, "TAMANIO_STACK");
	d_plp->logger = logger;

	return d_plp;
}

t_datos_pcp *crearConfiguracionPcp(t_config *config, t_log *logger)
{
	t_datos_pcp *d_pcp = malloc(sizeof(t_datos_pcp));
	d_pcp->puerto_escucha_cpu = config_get_string_value(config, "PUERTO_CPU");
	d_pcp->logger = logger;

	return d_pcp;
}

//motivo = sólo deberia ser "INFORMAR" o "FINALIZAR"
int enviarMensajePrograma(int *socket, char *motivo, char *mensaje)
{
	t_paquete_programa paq;
	int respuesta = 0;
	int bytesEnviados = 0;

	char *mensaje_completo = NULL;
	int motivo_len = strlen(motivo);
	int mensaje_len = strlen(mensaje);
	int offset = 0;

	mensaje_completo = calloc(motivo_len + 1 + 1 + mensaje_len + 1, 1);

	memcpy(mensaje_completo + offset, motivo, motivo_len);
	offset += motivo_len;

	memcpy(mensaje_completo + offset, "_", 1);
	offset += 1;

	memcpy(mensaje_completo + offset, mensaje, mensaje_len);
	offset += mensaje_len;

	paq.id = 'K';
	paq.mensaje = mensaje_completo;
	paq.sizeMensaje = motivo_len + 1 + 1 + mensaje_len + 1;	//motivo + el nulo + el _ + mensaje + el nulo

	char *paqueteSaliente = serializar_paquete(&paq, NULL);
	bytesEnviados = paq.tamanio_total;

	sendAll(*socket, paqueteSaliente, &bytesEnviados);
	free(paqueteSaliente);
	free(mensaje_completo);

	return respuesta;
}


/*
** Asumo que siempre hay al menos un dispositivo, si no hay dispositivos, entonces no pongas la opcion ID_HIO
** en la configuración.
*/
void cargarInfoIO(t_list **cabeceras, t_config *config, t_log *logger)
{

	int i, cant_disp = contarOcurrenciasElementos(config_get_string_value(config, "ID_HIO"));
	*cabeceras = list_create();
	t_cola_io *dispositivo_io = NULL;
	char **IO_ids = config_get_array_value(config, "ID_HIO");
	char **IO_tiempos = config_get_array_value(config, "HIO");

	printf("Hay %d dispositivos configurados. Sus IDs son:\n", cant_disp);

	//cargo los dispositivos y al final muestro el nombre.
	for(i = 0; i < cant_disp; i++){
		dispositivo_io = malloc(sizeof(t_cola_io));
		dispositivo_io->nombre_dispositivo = IO_ids[i];
		dispositivo_io->tiempo_espera = atoi(IO_tiempos[i]);
		dispositivo_io->cola_dispositivo = list_create();
		dispositivo_io->logger = logger;

		list_add(*cabeceras, dispositivo_io);
		
		//debug!
		printf("\t%s (Espera: %d, cola en: %p)\n", dispositivo_io->nombre_dispositivo, dispositivo_io->tiempo_espera, dispositivo_io->cola_dispositivo);
	}

	return;
}

void cargarInfoSemaforosAnsisop(t_list **semaforos, t_config *config, t_log *logger)
{
	int i, cant_semaforos = contarOcurrenciasElementos(config_get_string_value(config, "SEMAFOROS"));
	*semaforos = list_create();
	t_semaforo_ansisop *semaforo = NULL;
	char **sem_nombres = config_get_array_value(config, "SEMAFOROS");
	char **sem_valores = config_get_array_value(config, "VALOR_SEMAFORO");

	printf("Hay %d semáforos en el sistema. Sus nombres son:\n", cant_semaforos);
	
	//cargo los semáforos y al final muestro el nombres
	int aux = 0;
	for(i = 0; i < cant_semaforos; i++){
		semaforo = malloc(sizeof(t_semaforo_ansisop));
		semaforo->nombre = sem_nombres[i];
		semaforo->valor = malloc(sizeof(int));
			aux = atoi(sem_valores[i]);
			memcpy(semaforo->valor, &aux, sizeof(int));
		semaforo->pcbs_en_wait = list_create();
		semaforo->logger = logger;

		list_add(*semaforos, semaforo);

		//debug!
		int *aux = semaforo->valor;
		printf("\t%s (Valor: %d, cola de PCBs: %p)\n", semaforo->nombre, *aux, semaforo->pcbs_en_wait);
	}

	return;
}

int contarOcurrenciasElementos(char *cadena)
{
	int i, len = strlen(cadena);
	int disp = 1;	

	for(i = 0; i < len; i++){
		if(cadena[i] == ',') disp++;
	}

	return disp;
}

/*
** Asumo que siempre hay al menos una variable compartida, si no hay entonces no pongas la opcion COMPARTIDAS
** en la configuración.
*/
void cargarInfoCompartidas(t_list **listaCompartidas, t_config *config, t_log *logger)
{
	int i, cantVariablesCompartidas = contarOcurrenciasElementos(config_get_string_value(config, "COMPARTIDAS"));
	*listaCompartidas = list_create();
	char **variables_compartidas = config_get_array_value(config, "COMPARTIDAS");
	t_compartida *comp = NULL;

	log_info(logger, "Hay %d variables compartidas. Sus nombres son:\n", cantVariablesCompartidas);

	//cargo las variables y al final muestro el nombre.
	for(i = 0; i < cantVariablesCompartidas; i++){
		comp = malloc(sizeof(t_cola_io));
		comp->nombre = variables_compartidas[i];
		comp->valor = 98;

		list_add(*listaCompartidas, comp);
		
		//debug!
		printf("\t%s (Valor inicial: %d)\n", comp->nombre, comp->valor);
	}

	return;
}

int crearHilosIO(t_list *cabeceras, t_log *logger)
{
	int i, cant_disp = list_size(cabeceras);
	int huboError = 0;
	sem_t *s_cola_io;

	for(i = 0; i < cant_disp; i++){
		t_cola_io *config_disp = list_get(cabeceras, i);
		s_cola_io = malloc(sizeof(sem_t));

		if(sem_init(s_cola_io, 0, 0) != 0){
			log_error(logger, "[DISPOSITIVO-%s] Hubo un error al inicializar el semáforo de mi cola. Adiós para siempre!.", config_disp->nombre_dispositivo);
			huboError = 1;
			break;
		}

		config_disp->s_cola = s_cola_io;
		pthread_t *hilo_io = malloc(sizeof(pthread_t));

		if(pthread_create(hilo_io, NULL, hilo_entrada_salida, (void *) config_disp)) {
			log_error(logger, "Error al crear el hilo para el dispositivo %s. Motivo: %s", config_disp->nombre_dispositivo, strerror(errno));
			huboError = 1;
			break;
		}

		pthread_detach(*hilo_io);
	}

	return huboError;
}

int crearHilosSemaforosAnsisop(t_list *lista_semaforos, t_log *logger)
{
	int i, cant_sem = list_size(lista_semaforos);
	int huboError = 0;
	sem_t *liberar;

	for(i = 0; i < cant_sem; i++){
		t_semaforo_ansisop *config_sem = list_get(lista_semaforos, i);
		liberar = malloc(sizeof(sem_t));

		if(sem_init(liberar, 0, 0) != 0){
			log_error(logger, "[SEMAFORO_ANSISOP-%s] Hubo un error al inicializar el semáforo que libera PCBs. Adiós para siempre!", config_sem->nombre);
			huboError = 1;
			break;
		}

		config_sem->liberar = liberar;
		pthread_t *hilo_sem = malloc(sizeof(pthread_t));

		if(pthread_create(hilo_sem, NULL, hilo_semaforo, (void *) config_sem)) {
			log_error(logger, "Error al crear el hilo para el semáforo ansisop \'%s\'. Motivo: %s", config_sem->nombre, strerror(errno));
			huboError = 1;
			break;
		}

		pthread_detach(*hilo_sem);
	}

	return huboError;
}

/*
**	Busca en la cola de dispositivos al dispositivo solicitado y pone en su cola al pcb correspondiente.
*/
int syscall_entradaSalida(char *nombre_dispositivo, t_pcb *pcb_en_espera, uint32_t tiempoEnUnidades, t_log *logger){

	int i, sizeDispositivos = list_size(cabeceras_io);
	int encontrado = 0;
	t_cola_io *disp_buscado = malloc(sizeof(t_cola_io));

	for(i = 0; i < sizeDispositivos; i++){
		disp_buscado = list_get(cabeceras_io, i);

		if(strcmp(disp_buscado->nombre_dispositivo, nombre_dispositivo) == 0){
			encontrado = 1;
			break;
		}
	}

	if(!encontrado){
		log_error(logger, "[SYS_entradaSalida] No se encontró el dispositivo buscado.");
		return -1;
	}

	t_pcb_en_io *pcb_en_io = malloc(sizeof(t_pcb_en_io));
		pcb_en_io->pcb_en_espera = pcb_en_espera;
		pcb_en_io->unidades_de_tiempo = tiempoEnUnidades;

	//agrego al pcb a la cola del dispositivo
	list_add(disp_buscado->cola_dispositivo, pcb_en_io);

	//le aviso al dispositivo
	sem_post(disp_buscado->s_cola);

	return 0;
}

int syscall_obtenerValorCompartida(char *nombre_compartida, int socket_respuesta, t_log *logger)
{
	int i, tamanio_lista = list_size(listaCompartidas);
	t_compartida *variable = NULL;
	int encontrada = 0;

	for(i = 0; i < tamanio_lista; i++){
		variable = list_get(listaCompartidas, i);

		if(strcmp(variable->nombre,nombre_compartida) == 0){
			encontrada = 1;
			break;
		}
	}

	if(!encontrada){
		log_error(logger, "[SYS_obtenerValorCompartida] No se encontró la variable buscada.");
		return -1;
	}

	char str_valor[10];
	t_paquete_programa paq;
		paq.id = 'K';
		sprintf(str_valor, "%d", variable->valor);
		paq.mensaje = str_valor;
		paq.sizeMensaje = strlen(str_valor);

	char *paq_serializado = serializar_paquete(&paq, logger);
	int bEnv = paq.tamanio_total;
	if(sendAll(socket_respuesta, paq_serializado, &bEnv)){
		log_error(logger, "[SYS_obtenerValorCompartida] Hubo un error al tratar de enviar la respuesta a la CPU");
	}

	return 0;
}

int syscall_asignarValorCompartida(char *nombre_compartida, int socket_respuesta, int nuevo_valor, t_log *logger)
{
	int i, tamanio_lista = list_size(listaCompartidas);
	t_compartida *variable = NULL;
	int encontrada = 0;

	for(i = 0; i < tamanio_lista; i++){
		variable = list_get(listaCompartidas, i);

		if(strcmp(variable->nombre,nombre_compartida) == 0){
			encontrada = 1;
			break;
		}
	}

	if(!encontrada){
		log_error(logger, "[SYS_asignarValorCompartida] No se encontró la variable buscada.");
		return -1;
	}

	variable->valor = nuevo_valor;

	char str_valor[10];
	t_paquete_programa paq;
		paq.id = 'K';
		sprintf(str_valor, "%d", variable->valor);
		paq.mensaje = str_valor;
		paq.sizeMensaje = strlen(str_valor);

	char *paq_serializado = serializar_paquete(&paq, logger);
	int bEnv = paq.tamanio_total;
	if(sendAll(socket_respuesta, paq_serializado, &bEnv)){
		log_error(logger, "[SYS_asignarValorCompartida] Hubo un error al tratar de enviar la respuesta a la CPU");
	}

	return 0;
}

int syscall_wait(char *nombre_semaforo, t_pcb *pcb_a_wait, int socket_respuesta, t_log *logger)
{
	//busco el semaforo en la lista de semaforos
	int i, cant_semaforos = list_size(semaforos_ansisop);
	int encontrado = 0;
	t_semaforo_ansisop *sem_buscado = NULL;
	int *valor_semaforo = NULL;

	for(i = 0; i < cant_semaforos; i++){
		sem_buscado = list_get(semaforos_ansisop, i);
		if(strcmp(nombre_semaforo, sem_buscado->nombre) == 0){
			valor_semaforo = sem_buscado->valor;
			encontrado = 1;
			break;
		}
	}

	if(!encontrado){
		_responderWait(socket_respuesta, -1, logger);
		return -1;
	}

	if(pcb_a_wait == NULL){	//esto es una consulta para ver si bloquearía o no
		if(*valor_semaforo > 0){
			*valor_semaforo = *valor_semaforo - 1;
			log_info(logger, "[SYS_wait] Respondo al PCB que siga operando, y mi valor ahora reducido es: %d", *valor_semaforo);
			_responderWait(socket_respuesta, 1, logger);
			return 1;
		} else {
			_responderWait(socket_respuesta, 0, logger);
			log_info(logger, "[SYS_wait] Respondo al PCB que se va a bloquear, mi valor actual (sin reducir) es: %d", *valor_semaforo);
			return 0;
		}
	} else {	// la cpu ya había preguntado, 
		if(*valor_semaforo > 0){	// si pasa esto, es porque entre la consulta del proceso y el envío del pcb se liberó una instancia
									// el famoso caso "Te comiste un desalojo al pedo"
			*valor_semaforo = *valor_semaforo - 1;
			list_add(sem_buscado->pcbs_en_wait, pcb_a_wait);
			sem_post(sem_buscado->liberar);
			log_info(logger, "[SYS_wait] Caso \"TCUDAP\".");
			return 1;
		} else {
			*valor_semaforo = *valor_semaforo - 1;
			list_add(sem_buscado->pcbs_en_wait, pcb_a_wait);
			log_info(logger, "[SYS_wait] Voy a meter en la cola del semáforo \'%s\' al proceso con ID: %d.", sem_buscado->nombre, pcb_a_wait->id);
			_mostrar_cola_semaforo(sem_buscado, logger);
			return 1;
		}
	}

	return 678;
}

void _responderWait(int socket_respuesta, int valor_resp, t_log *logger)
{
	char str_valor[10];
	t_paquete_programa paq;
		paq.id = 'K';
		sprintf(str_valor, "%d", valor_resp);
		paq.mensaje = str_valor;
		paq.sizeMensaje = strlen(str_valor);

	char *paq_serializado = serializar_paquete(&paq, logger);
	int bEnv = paq.tamanio_total;
	if(sendAll(socket_respuesta, paq_serializado, &bEnv)){
		log_error(logger, "[SYS_wait?] Hubo un error al tratar de enviar la respuesta a la CPU");
	}

	return;
}

int syscall_signal(char *nombre_semaforo, t_log *logger)
{
	//busco el semaforo en la lista de semaforos
	int i, cant_semaforos = list_size(semaforos_ansisop);
	int encontrado = 0;
	t_semaforo_ansisop *sem_buscado = NULL;

	for(i = 0; i < cant_semaforos; i++){
		sem_buscado = list_get(semaforos_ansisop, i);
		if(strcmp(nombre_semaforo, sem_buscado->nombre) == 0){
			encontrado = 1;
			break;
		}
	}

	if(!encontrado){
		log_error(logger, "[SYS_signal] No encontré el semáforo con nombre: %s. Posiblemente vuele todo al choto.", nombre_semaforo);
		return -1;
	}

	sem_post(sem_buscado->liberar);
	//no incremento el valor, porque ya lo hace el hilo del semáforo. Creo.
	return 1;
}

void _mostrar_cola_semaforo(t_semaforo_ansisop *sem_buscado, t_log *logger)
{
	int i, cant_bloq = list_size(sem_buscado->pcbs_en_wait);
	t_pcb *pcb_bloqueado = NULL;
	log_info(logger, "[COLA_BLOQUEADO] El semáforo %s tiene en su cola a los procesos:", sem_buscado->nombre);
	
	pthread_mutex_t mostrarTextoConsecutivo = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&mostrarTextoConsecutivo);
		for(i = 0; i < cant_bloq; i++){
			pcb_bloqueado = list_get(sem_buscado->pcbs_en_wait, i);
			log_info(logger, "\tID Proceso: %d.", pcb_bloqueado->id);
		}
	pthread_mutex_unlock(&mostrarTextoConsecutivo);
	
	return;
}
