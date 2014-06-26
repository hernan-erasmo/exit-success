#include <netdb.h>
#include <string.h>

#include "atencionConexiones.h"

void *atencionConexiones(void *config)
{
	t_log *logger = (t_log *) (((t_config_conexion *) config)->logger);
	int *sock = (int *) (((t_config_conexion *) config)->socket);
	t_param_memoria *parametros_memoria = (t_param_memoria *) (((t_config_conexion *) config)->parametros_memoria);
	pthread_t *punteroAEsteHilo = (pthread_t *) (((t_config_conexion *) config)->hilo);
	t_paquete_programa paq;
	int bytesRecibidos = 0;
	int atendiendoSolicitud = 1;

	
	while(atendiendoSolicitud){
		bytesRecibidos = recvAll(&paq, *sock);

		switch(paq.id){
			case 'P':
				;
				uint32_t resp = -1;
				handler_plp(*sock, &resp, paq.mensaje, parametros_memoria, logger);

				break;
			case 'C':
				;
				void *resp_buf = NULL;
				handler_cpu(*sock, resp_buf, paq.mensaje, parametros_memoria, logger);

				break;
			case 'H':
				log_info(logger, "[UMV] Recibí un handshake del Kernel");
				log_info(logger, "[UMV] El mensaje del handshake es: %s\n", paq.mensaje);
				break;
			case 'F':
			default:
				log_info(logger, "[UMV] No puedo identificar el tipo de conexión");
				close(*sock);
				atendiendoSolicitud = 0;
		}

		free(paq.mensaje);
		bytesRecibidos = 0;
	}

	free(sock);
	free(parametros_memoria);
	free(punteroAEsteHilo);
	free(config);
	pthread_exit(NULL);
}

void handler_plp(int sock, uint32_t *respuesta, char *orden, t_param_memoria *parametros_memoria, t_log *logger)
{
	char *savePtr1 = NULL;
	char *comando = strtok_r(orden, ",", &savePtr1);

	log_info(logger, "[UMV] Estoy atendiendo una solicitud de %s del PLP.", comando);

	if(strcmp(comando, "cambiar_proceso_activo") == 0){
		handler_cambiar_proceso_activo(respuesta, orden, parametros_memoria, &savePtr1, logger);
		enviar_respuesta_numerica(&sock, *respuesta, logger);
	}

	if(strcmp(comando,"crear_segmento") == 0){
		handler_crear_segmento(respuesta, orden, parametros_memoria, &savePtr1, logger);
		enviar_respuesta_numerica(&sock, *respuesta, logger);
	}
	
	if(strcmp(comando,"enviar_bytes") == 0){
		handler_enviar_bytes(respuesta, orden, parametros_memoria, &savePtr1, logger);
		enviar_respuesta_numerica(&sock, *respuesta, logger);
	}

	if(strcmp(comando,"destruir_segmentos") == 0){
		handler_destruir_segmentos(respuesta, parametros_memoria, &savePtr1, logger);
		enviar_respuesta_numerica(&sock, *respuesta, logger);
	}
	
	if(strcmp(comando,"solicitar_bytes") == 0){
		uint32_t tamanio_buffer_respuesta = 0;
		void *resp = NULL;
		handler_solicitar_bytes(&resp, parametros_memoria, &tamanio_buffer_respuesta, &savePtr1, logger);

		enviar_respuesta_buffer(&sock, resp, &tamanio_buffer_respuesta, logger);
	}

	return;
}

void handler_cpu(int sock, void *respuesta, char *orden, t_param_memoria *parametros_memoria, t_log *logger)
{
	char *savePtr1 = NULL;
	char *comando = strtok_r(orden, ",", &savePtr1);
	uint32_t resp_num = 0;

	log_info(logger, "[UMV] Estoy atendiendo una solicitud de %s de una CPU.", comando);

	uint32_t tamanio_buffer_respuesta = 0;

	if(strcmp(comando, "cambiar_proceso_activo") == 0){
		handler_cambiar_proceso_activo(&resp_num, orden, parametros_memoria, &savePtr1, logger);
		enviar_respuesta_numerica(&sock, resp_num, logger);
	} else if(strcmp(comando,"solicitar_bytes") == 0){
		handler_solicitar_bytes(&respuesta, parametros_memoria, &tamanio_buffer_respuesta, &savePtr1, logger);
		enviar_respuesta_buffer(&sock, respuesta, &tamanio_buffer_respuesta, logger);
	} else if(strcmp(comando,"enviar_bytes") == 0){
		handler_enviar_bytes(&resp_num, orden, parametros_memoria, &savePtr1, logger);
		enviar_respuesta_numerica(&sock, resp_num, logger);
	}

	return;	
}

void handler_cambiar_proceso_activo(uint32_t *respuesta, char *orden, t_param_memoria *parametros_memoria, char **savePtr1, t_log *logger)
{
	char *proceso_activo = strtok_r(NULL, ",", savePtr1);

	*respuesta = cambiar_proceso_activo(atoi(proceso_activo));

	return;
}

void handler_crear_segmento(uint32_t *respuesta, char *orden, t_param_memoria *parametros_memoria, char **savePtr1, t_log *logger)
{
	char *id_prog = strtok_r(NULL, ",", savePtr1);
	char *tamanio = strtok_r(NULL, ",", savePtr1);
	
	void *mem_ppal = parametros_memoria->mem_ppal;
	uint32_t tamanio_mem_ppal = parametros_memoria->tamanio_mem_ppal;
	t_list *lista_segmentos = parametros_memoria->listaSegmentos;
	char *algoritmo_compactacion = parametros_memoria->algoritmo_comp;

	t_list *espacios_libres = buscarEspaciosLibres(lista_segmentos, mem_ppal, tamanio_mem_ppal);
	*respuesta = crearSegmento(atoi(id_prog), atoi(tamanio), espacios_libres, lista_segmentos, algoritmo_compactacion);

	if(*respuesta == 0){
		log_info(logger, "[UMV] No hay espacio para este segmento. Voy a intentar compactar.");
		compactar(lista_segmentos, mem_ppal, tamanio_mem_ppal);

		if (!list_is_empty(espacios_libres)) {
			list_clean_and_destroy_elements(espacios_libres, eliminarEspacioLibre);
		}

		list_destroy(espacios_libres);

		espacios_libres = buscarEspaciosLibres(lista_segmentos, mem_ppal, tamanio_mem_ppal);
		*respuesta = crearSegmento(atoi(id_prog), atoi(tamanio), espacios_libres, lista_segmentos, algoritmo_compactacion);

		if(*respuesta == 0)
			log_info(logger, "[UMV] No hay lugar para crear un segmento, incluso después de haber compactado la memoria.");
	}

	list_destroy_and_destroy_elements(espacios_libres, eliminarEspacioLibre);

	return;
}

void handler_enviar_bytes(uint32_t *respuesta, char *orden, t_param_memoria *parametros_memoria, char **savePtr1, t_log *logger)
{
	char *base = strtok_r(NULL, ",", savePtr1);
	char *offset = strtok_r(NULL, ",", savePtr1);
	char *id_prog = strtok_r(NULL, ",", savePtr1);
	char *tamanio = strtok_r(NULL, ",", savePtr1);
	void *buffer = strtok_r(NULL, "\0", savePtr1);

	pthread_mutex_t op_atomica = PTHREAD_MUTEX_INITIALIZER;
	
	pthread_mutex_lock(&op_atomica);
		cambiar_proceso_activo(atoi(id_prog));
		*respuesta = enviar_bytes(parametros_memoria->listaSegmentos, atoi(base), atoi(offset), atoi(tamanio), buffer);
	pthread_mutex_unlock(&op_atomica);

	return;
}

void handler_solicitar_bytes(void **respuesta, t_param_memoria *parametros_memoria, uint32_t *tam, char **savePtr1, t_log *logger)
{
	char *base = strtok_r(NULL, ",", savePtr1);
	char *offset = strtok_r(NULL, ",", savePtr1);
	char *tamanio = strtok_r(NULL, ",", savePtr1);
	*tam = atoi(tamanio);

	*respuesta = solicitar_bytes(parametros_memoria->listaSegmentos, atoi(base), atoi(offset), tam);

	return;
}

void handler_destruir_segmentos(void *respuesta, t_param_memoria *parametros_memoria, char **savePtr1, t_log *logger){
	char *id_proceso = strtok_r(NULL, ",", savePtr1);

	destruirSegmentos(parametros_memoria->listaSegmentos, atoi(id_proceso));

	return;
}

void enviar_respuesta_numerica(int *socket, uint32_t respuesta, t_log *logger)
{
	t_paquete_programa respuestaAlComando;
	char r[5];
	sprintf(r, "%d", respuesta);
	respuestaAlComando.id = 'U';
	respuestaAlComando.mensaje = r;
	respuestaAlComando.sizeMensaje = strlen(r);

	char *respuesta_serializada = serializar_paquete(&respuestaAlComando, logger);
	int bEnv = respuestaAlComando.tamanio_total;
	
	if(sendAll(*socket, respuesta_serializada, &bEnv)){
		log_error(logger, "[UMV] Error en el envío de la respuesta al comando. Motivo: %s", strerror(errno));
	}
	log_info(logger, "[UMV] Ya envié la respuesta al comando.");

	free(respuesta_serializada);

	return;
}

void enviar_respuesta_buffer(int *socket, void *respuesta, uint32_t *tam_buffer, t_log *logger)
{
	t_paquete_programa respuestaAlComando;
		respuestaAlComando.id = 'U';
		respuestaAlComando.mensaje = (char *) respuesta;
		respuestaAlComando.sizeMensaje = *tam_buffer;

	char *respuesta_serializada = serializar_paquete(&respuestaAlComando, logger);
	int bEnv = respuestaAlComando.tamanio_total;

	if(sendAll(*socket, respuesta_serializada, &bEnv)){
		log_error(logger, "[UMV] Error en el envío de la respuesta al comando. Motivo: %s", strerror(errno));		
	}
	log_info(logger, "[UMV] Ya envié la respuesta al comando.");

	free(respuesta_serializada);

	return;	
}
