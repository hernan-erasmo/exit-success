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
				log_info(logger, "[UMV] Estoy atendiendo una solicitud del PLP");
				uint32_t resp = -1;
				handler_plp(&resp, paq.mensaje, parametros_memoria, logger);
				
				enviar_respuesta_plp(sock, resp, logger);

				break;
			case 'C':
				log_info(logger, "[UMV] Estoy atendiendo una solicitud de una cpu.");
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

void handler_plp(uint32_t *respuesta, char *orden, t_param_memoria *parametros_memoria, t_log *logger){
	char *savePtr1 = NULL;
	char *comando = strtok_r(orden, ",", &savePtr1);

	if(strcmp(comando, "cambiar_proceso_activo") == 0)
		handler_cambiar_proceso_activo(respuesta, orden, parametros_memoria, &savePtr1, logger);
	if(strcmp(comando,"crear_segmento") == 0)
		handler_crear_segmento(respuesta, orden, parametros_memoria, &savePtr1, logger);
	if(strcmp(comando,"enviar_bytes") == 0)
		handler_enviar_bytes(respuesta, orden, parametros_memoria, &savePtr1, logger);

	return;
}

void handler_cambiar_proceso_activo(uint32_t *respuesta, char *orden, t_param_memoria *parametros_memoria, char **savePtr1, t_log *logger)
{
	char *proceso_activo = strtok_r(NULL, ",", savePtr1);

	cambiar_proceso_activo(atoi(proceso_activo));

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
	*respuesta = crearSegmento(
					atoi(id_prog),
					atoi(tamanio),
					espacios_libres,
					lista_segmentos,
					algoritmo_compactacion
				);

	list_destroy_and_destroy_elements(espacios_libres, eliminarEspacioLibre);

	return;
}

void enviar_respuesta_plp(int *socket, uint32_t respuesta, t_log *logger)
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
		log_error(logger, "[UMV] Error en el envío de la respuesta del comando al PLP. Motivo: %s", strerror(errno));
	}
	log_info(logger, "[UMV] Ya mandé la respuesta al PLP");

	free(respuesta_serializada);

	return;
}
