#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

#include "cpu.h"
#include "implementacion_primitivas.h"

void mostrarElementosDiccionario(char *k, void *v);

void sig_handler(int signo)
{
	if(signo == SIGUSR1)
		salimosPorSIGUSR1 = 1;
}

int main(int argc, char *argv[])
{
	salimosPorSIGUSR1 = 0;
	if(signal(SIGUSR1, sig_handler) == SIG_ERR){
		printf("No voy atrapar la señal SIGUSR1\n");
		return EXIT_FAILURE;		
	}

	int errorArgumentos = 0;
	int errorLogger = 0;
	int errorConfig = 0;
	int errorConexion = 0;

	salimosPorSyscallBloqueante = 0;
	salimosPorError = 0;
	salimosPorFin = 0;

	//Variables para la carga de la configuración
	t_config *config = NULL;

	//Variables para las conexiones
	int puerto_pcp = -1;
	char *ip_pcp = NULL;
	int puerto_umv = -1;
	char *ip_umv = NULL;
	struct sockaddr_in socketInfo;
	int status = 0;

	//Variables para el funcionamiento del parser
	AnSISOP_funciones *funciones_comunes = malloc(sizeof(AnSISOP_funciones));
		funciones_comunes->AnSISOP_definirVariable = definirVariable;
		funciones_comunes->AnSISOP_obtenerPosicionVariable = obtenerPosicionVariable;
		funciones_comunes->AnSISOP_dereferenciar = dereferenciar;
		funciones_comunes->AnSISOP_asignar = asignar;
		funciones_comunes->AnSISOP_obtenerValorCompartida = obtenerValorCompartida;
		funciones_comunes->AnSISOP_asignarValorCompartida = asignarValorCompartida;
		funciones_comunes->AnSISOP_irAlLabel = irAlLabel;
		funciones_comunes->AnSISOP_llamarSinRetorno = llamarSinRetorno;
		funciones_comunes->AnSISOP_llamarConRetorno = llamarConRetorno;
		funciones_comunes->AnSISOP_finalizar = finalizar;
		funciones_comunes->AnSISOP_retornar = retornar;
		funciones_comunes->AnSISOP_imprimir = imprimir;
		funciones_comunes->AnSISOP_imprimirTexto = imprimirTexto;
		funciones_comunes->AnSISOP_entradaSalida = entradaSalida;

	AnSISOP_kernel *funciones_kernel = malloc(sizeof(AnSISOP_kernel));
		funciones_kernel->AnSISOP_wait = wait;
		funciones_kernel->AnSISOP_signal = prim_signal;

	errorArgumentos = checkArgs(argc);
	errorLogger = crearLogger(&logger);
	errorConfig = cargarConfig(&config, argv[1]);

	if(errorArgumentos || errorLogger || errorConfig) {
		goto liberarRecursos;
		return EXIT_FAILURE;
	}

	ip_pcp = config_get_string_value(config,"IP_PCP");
	puerto_pcp = config_get_int_value(config,"PUERTO_PCP");
	ip_umv = config_get_string_value(config,"IP_UMV");
	puerto_umv = config_get_int_value(config, "PUERTO_UMV");

	log_info(logger, "Hola, soy la CPU %d", getpid());

	//Me conecto al hilo PCP del Kernel
	socket_pcp = -1;
	errorConexion = crear_conexion_saliente(&socket_pcp, &socketInfo, ip_pcp, puerto_pcp, logger, "CPU");
	if (errorConexion) {
		log_error(logger, "[CPU] Error al conectar con el Kernel (Hilo PCP).");
		goto liberarRecursos;
		return EXIT_FAILURE;
	}
	log_info(logger, "[CPU] Conecté correctamente con el PCP.");

	//Me conecto a la UMV
	socket_umv = -1;
	errorConexion = crear_conexion_saliente(&socket_umv, &socketInfo, ip_umv, puerto_umv, logger, "CPU");
	if (errorConexion) {
		log_error(logger, "[CPU] Error al conectar con la UMV.");
		goto liberarRecursos;
		return EXIT_FAILURE;
	}
	log_info(logger, "[CPU] Conecté correctamente con la UMV.");

	int q, bEnv = 0;
	char *proxima_instruccion = NULL;
	while(1){		//Asumo que todo lo que venga del PCP va a ser un PCB, asi que lo proceso como tal.
		t_paquete_programa paq;
			paq.id = 'O';
			paq.sizeMensaje = 0;
			paq.mensaje = NULL;

		//Le digo al PCP que estoy ociosa (porque soy una cpu, no el programador, que es bien hombre, y no está ociosa, a diferencia de la cpu)
		char *paqueteSerializado = serializar_paquete(&paq, logger);
		bEnv = paq.tamanio_total;
		if(sendAll(socket_pcp, paqueteSerializado, &bEnv)){
			log_error(logger, "[CPU] Error en la transmisión hacia el PCP. Motivo: %s", strerror(errno));
			goto liberarRecursos;
			return EXIT_FAILURE;		
		}

		free(paqueteSerializado);

		log_info(logger, "[CPU] Me bloqueo esperando algún PCB del PCP");
		status = recvPcb(&pcb, socket_pcp);
		
		if(status){		//Comenzá a procesar el pcb

			log_info(logger,"[CPU] Me acaba de llegar un PCB correspondiente al programa con ID: %d.", pcb.id);
			generarDiccionarioVariables();

			for(q = pcb.quantum; q > 0; q--){
				log_debug(logger, "[CPU] Comienza QUANTUM");
				debo_actualizar_manualmente_p_counter = 1;
				proxima_instruccion = obtener_proxima_instruccion(socket_umv, logger);
				analizadorLinea(proxima_instruccion, funciones_comunes, funciones_kernel);
				
				if(debo_actualizar_manualmente_p_counter){
					//log_info(logger, "[CPU] Incremento el program counter del proceso %d, antes valía %d, y ahora va a valer %d.", pcb.id, pcb.p_counter, (pcb.p_counter + 1));
					pcb.p_counter = pcb.p_counter + 1;
					debo_actualizar_manualmente_p_counter = 0;
				}
				
				if(salimosPorFin || salimosPorSyscallBloqueante || salimosPorError)
					break;
			}
			
			if(salimosPorSyscallBloqueante){
				log_debug(logger, "[CPU] Finaliza QUANTUM por syscall bloqueante");
				//log_info(logger, "[CPU] El proceso con ID = %d quiere ejecutar una syscall (%s).", pcb.id, mi_syscall);

				if(enviarPcbProcesado(socket_pcp, 'S', logger) > 0){
					log_error(logger, "[CPU] Error en la transmisión hacia el PCP. Motivo: %s", strerror(errno));
					goto liberarRecursos;
					return EXIT_FAILURE;
				}
				free(mi_syscall);
				salimosPorSyscallBloqueante = 0;

			} else if(salimosPorError) {
				log_debug(logger, "[CPU] Finaliza QUANTUM por error en runtime");
				salimosPorError = 0;

				log_error(logger, "[CPU] La ejecución del proceso con ID = %d finalizó de forma anormal.", pcb.id);
				
				if(enviarPcbProcesado(socket_pcp, 'E', logger) > 0){
					log_error(logger, "[CPU] Error en la transmisión hacia el PCP. Motivo: %s", strerror(errno));
					goto liberarRecursos;
					return EXIT_FAILURE;
				}
			} else {
				
				if(salimosPorFin){
					log_debug(logger, "[CPU] Finaliza QUANTUM por haber llegado al fin del proceso.");
					salimosPorFin = 0;
					//log_info(logger, "[CPU] Finalizó normalmente la ejecución del proceso con ID = %d.", pcb.id);
					
					if(enviarPcbProcesado(socket_pcp, 'F', logger) > 0){
						log_error(logger, "[CPU] Error en la transmisión hacia el PCP. Motivo: %s", strerror(errno));
						goto liberarRecursos;
						return EXIT_FAILURE;
					}
				} else {
					log_info(logger, "[CPU] Finalizó el quantum. Enviando PCB al Kernel.");

					if(salimosPorSIGUSR1){
						if(enviarPcbProcesado(socket_pcp, 'X', logger) > 0){
							log_error(logger, "[CPU] Error en la transmisión hacia el PCP. Motivo: %s", strerror(errno));
							goto liberarRecursos;
							return EXIT_FAILURE;
						}

						goto liberarRecursos;
						return EXIT_SUCCESS;			
					}
					
					if(enviarPcbProcesado(socket_pcp, 'Q', logger) > 0){
						log_error(logger, "[CPU] Error en la transmisión hacia el PCP. Motivo: %s", strerror(errno));
						goto liberarRecursos;
						return EXIT_FAILURE;
					}
				}
			}

			log_info(logger, "[CPU] Se envió al Kernel el PCB actualizado.");
			
		} else {	//Falló la recepción del pcb.
			log_error(logger, "[CPU] Hubo una falla en la recepción del PCB.");
		}

		//Vaciamos el diccionario de variables.
		dictionary_clean(diccionario_variables);
	}

	goto liberarRecursos;
	return EXIT_SUCCESS;

liberarRecursos:
	if(logger)
		log_destroy(logger);

	if(config)
		config_destroy(config);

	if(socket_pcp != -1)
		close(socket_pcp);

	if(socket_umv != -1)
		close(socket_umv);

	if(funciones_comunes)
		free(funciones_comunes);

	if(funciones_kernel)
		free(funciones_kernel);
}

int crearLogger(t_log **logger)
{
	char nombreArchivoLog[15];
	char *nombre = "cpu_log_";
	char str_pid[7];
	pid_t pid = getpid();

	sprintf(str_pid, "%d", pid);
	strcpy(nombreArchivoLog, nombre);
	strcpy(nombreArchivoLog + strlen(nombre), str_pid);

	if ((*logger = log_create(nombreArchivoLog,"CPU",true,LOG_LEVEL_DEBUG)) == NULL) {
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
		printf("La CPU debe recibir como parámetro únicamente la ruta al archivo de configuración.\n");
		return 1;
	}
	
	return 0;	
}

void generarDiccionarioVariables()
{
	diccionario_variables = NULL;
	diccionario_variables = dictionary_create();
	log_info(logger, "[CPU] Generando diccionario de variables.");
	
	int i;
	pthread_mutex_t operar = PTHREAD_MUTEX_INITIALIZER;
	char *respuesta_umv;
	uint32_t cursor = 0;
	uint32_t *valor;

	pthread_mutex_lock(&operar);
		
		//log_info(logger, "[CPU] El tamaño del contexto actual es de: %d.", pcb.size_ctxt_actual);
		for(i = 0; i < pcb.size_ctxt_actual; i++){
		
			cursor = pcb.cursor_stack + i*5;
			valor = malloc(sizeof(uint32_t));
			*valor = cursor + 1;
			respuesta_umv = (char *) solicitar_solicitar_bytes(socket_umv, pcb.seg_stack, cursor, 1, pcb.id, 'C', logger);
			dictionary_put(diccionario_variables, respuesta_umv, valor);

		//para debug - mostrar todo el diccionario de variables
		//dictionary_iterator(diccionario_variables, mostrarElementosDiccionario);
		}
	pthread_mutex_unlock(&operar);

	return;
}

char *obtener_proxima_instruccion(int socket_umv, t_log *logger)
{
	char *inst = NULL;
	uint32_t offset = pcb.p_counter * 8;
	uint32_t *offset_inst = 0;
	uint32_t *tamanio_inst = 0;
	pthread_mutex_t prox_instruccion = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&prox_instruccion);
		offset_inst = (uint32_t *) solicitar_solicitar_bytes(socket_umv, pcb.seg_idx_cod, offset, 4, pcb.id, 'C', logger);
		tamanio_inst = (uint32_t *) solicitar_solicitar_bytes(socket_umv, pcb.seg_idx_cod, offset + 4, 4, pcb.id, 'C', logger);
		//log_info(logger, "[CPU] El índice de código %d me manda a buscar la instrucción con offset %d:", pcb.seg_idx_cod, *offset_inst);
		//log_info(logger, "[CPU] La inst. a recuperar tiene un tamaño de %d bytes.", *tamanio_inst);
		inst = (char *) solicitar_solicitar_bytes(socket_umv, pcb.seg_cod, *offset_inst, *tamanio_inst, pcb.id, 'C', logger);
	pthread_mutex_unlock(&prox_instruccion);

	return inst;
}

int enviarPcbProcesado(int socket_pcp, char evento, t_log *logger)
{
	int bEnv = 0;
	int comando_syscall_len = 0;
	int i;
	int offset = 0;
	int offset_sys = 0;
	int offset_pcb = 0;
	char *pcb_serializado;

	t_paquete_programa paq;
			paq.id = evento; 	//'S' = Te mandó un pcb que salió por solicitud de syscall, 'P' = te mando un pcb que salió por quantum, 'F' = te mando un pcb que salió por fin de operación

		if(salimosPorSyscallBloqueante){
			comando_syscall_len = strlen(mi_syscall);
			paq.mensaje = calloc(comando_syscall_len + 48,1);
			
			//meto los dos strings en el mensaje, primero el comando...
			for(i = 0; i < comando_syscall_len; i++){
				memcpy(paq.mensaje + offset, mi_syscall + offset_sys, 1);
				offset += 1;
				offset_sys += 1;
			}

			//...y despues el pcb serializado.
			pcb_serializado = serializar_pcb(&pcb,logger);
			for(i = 0; i < 48; i++){
				memcpy(paq.mensaje + offset, pcb_serializado + offset_pcb, 1);
				offset += 1;
				offset_pcb += 1;
			}

			paq.sizeMensaje = comando_syscall_len + 48;
		} else {
			paq.mensaje = serializar_pcb(&pcb, logger);
			paq.sizeMensaje = 48;
		}

		//Le digo al PCP que estoy ociosa (porque soy una cpu, no el programador, que es bien hombre, y no está ociosa, a diferencia de la cpu)
		char *paqueteSerializado = serializar_paquete(&paq, logger);
		bEnv = paq.tamanio_total;
		if(sendAll(socket_pcp, paqueteSerializado, &bEnv)){
			free(paqueteSerializado);
			return 1;
		}

		free(paq.mensaje);
		return 0;
}

void mostrarElementosDiccionario(char *k, void *v)
{
	printf("\t\t KEY: %c -------- VALUE: %d\n", *k, *((uint32_t *) v));
	printf("\t\t KEY: %c -------- VALUE: %d\n", *k, *((uint32_t *) v));
	printf("\t\t KEY: %c -------- VALUE: %d\n", *k, *((uint32_t *) v));
	printf("\t\t KEY: %c -------- VALUE: %d\n", *k, *((uint32_t *) v));
	printf("\t\t KEY: %c -------- VALUE: %d\n", *k, *((uint32_t *) v));
	printf("\t\t KEY: %c -------- VALUE: %d\n", *k, *((uint32_t *) v));
	printf("\t\t KEY: %c -------- VALUE: %d\n", *k, *((uint32_t *) v));
	printf("\t\t KEY: %c -------- VALUE: %d\n", *k, *((uint32_t *) v));

	return;
}
