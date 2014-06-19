#include "interfaz_umv.h"

uint32_t solicitar_cambiar_proceso_activo(int socket_umv, uint32_t contador_id_programa, char origen, t_log *logger)
{
	uint32_t bEnv = 0;
	char *orden = codificar_cambiar_proceso_activo(contador_id_programa);
	uint32_t valorRetorno = 0;	

	t_paquete_programa paq_saliente;
		paq_saliente.id = origen;
		paq_saliente.mensaje = orden;
		paq_saliente.sizeMensaje = strlen(orden);

	char *paqueteSaliente = serializar_paquete(&paq_saliente, logger);
	
	bEnv = paq_saliente.tamanio_total;
	if(sendAll(socket_umv, paqueteSaliente, &bEnv)){
		log_error(logger, "[PLP] Error en la solicitud de cambio de proceso activo. Motivo: %s", strerror(errno));
		free(paqueteSaliente);
		free(orden);
		return 0;
	}

	free(paqueteSaliente);
	free(orden);

	uint32_t bRec = 0;
	t_paquete_programa respuesta;
	log_info(logger, "[PLP] Esperando la respuesta de la UMV.");
	bRec = recvAll(&respuesta, socket_umv);
	log_info(logger, "[PLP] La UMV respondió %s", respuesta.mensaje);

	valorRetorno = atoi(respuesta.mensaje);
	free(respuesta.mensaje);

	return valorRetorno;	
}

uint32_t solicitar_crear_segmento(int socket_umv, uint32_t id_programa, uint32_t tamanio_segmento, char origen, t_log *logger)
{
	uint32_t bEnv = 0;
	char *orden = codificar_crear_segmento(id_programa, tamanio_segmento);
	uint32_t valorRetorno = 0;

	t_paquete_programa paq_saliente;
		paq_saliente.id = origen;
		paq_saliente.mensaje = orden;
		paq_saliente.sizeMensaje = strlen(orden);
	
	char *paqueteSaliente = serializar_paquete(&paq_saliente, logger);
	
	bEnv = paq_saliente.tamanio_total;
	if(sendAll(socket_umv, paqueteSaliente, &bEnv)){
		log_error(logger, "[PLP] Error en la solicitud de creación de segmento. Motivo: %s", strerror(errno));
		free(paqueteSaliente);
		free(orden);
		return 0;
	}

	free(paqueteSaliente);
	free(orden);

	uint32_t bRec = 0;
	t_paquete_programa respuesta;
	log_info(logger, "[PLP] Esperando la respuesta de la UMV.");
	bRec = recvAll(&respuesta, socket_umv);
	log_info(logger, "[PLP] La UMV respondió %s", respuesta.mensaje);

	valorRetorno = atoi(respuesta.mensaje);
	free(respuesta.mensaje);

	return valorRetorno;
}

uint32_t solicitar_enviar_bytes(int socket_umv, uint32_t base, uint32_t offset, int tamanio, void *buffer, char origen, t_log *logger)
{
	uint32_t bEnv = 0;
	uint32_t tamanio_de_la_orden_completa = 0;
	char *orden = codificar_enviar_bytes(base,offset,tamanio,buffer,&tamanio_de_la_orden_completa);
	uint32_t valorRetorno = 0;

	t_paquete_programa paq_saliente;
		paq_saliente.id = origen;
		paq_saliente.mensaje = orden;
		printf("############## ORDEN ############## : %s", orden);
		paq_saliente.sizeMensaje = tamanio_de_la_orden_completa;
	
	char *paqueteSaliente = serializar_paquete(&paq_saliente, logger);

	bEnv = paq_saliente.tamanio_total;
	if(sendAll(socket_umv, paqueteSaliente, &bEnv)){
		log_error(logger, "[PLP] Error en la solicitud de enviar bytes para el segmento de código. Motivo: %s", strerror(errno));
		free(paqueteSaliente);
		free(orden);
		return 0;
	}

	free(paqueteSaliente);
	free(orden);

	uint32_t bRec = 0;
	t_paquete_programa respuesta;
	log_info(logger, "[PLP] Esperando la respuesta de la UMV.");
	bRec = recvAll(&respuesta, socket_umv);
	log_info(logger, "[PLP] La UMV respondió %s", respuesta.mensaje);

	valorRetorno = atoi(respuesta.mensaje);
	free(respuesta.mensaje);

	return valorRetorno;
}

uint32_t solicitar_destruir_segmentos(int socket_umv, uint32_t id_programa, char origen, t_log *logger)
{
	uint32_t bEnv = 0;
	char *orden = codificar_destruir_segmentos(id_programa);
	uint32_t valorRetorno = 0;	

	t_paquete_programa paq_saliente;
		paq_saliente.id = origen;
		paq_saliente.mensaje = orden;
		paq_saliente.sizeMensaje = strlen(orden);

	char *paqueteSaliente = serializar_paquete(&paq_saliente, logger);
	
	bEnv = paq_saliente.tamanio_total;
	if(sendAll(socket_umv, paqueteSaliente, &bEnv)){
		log_error(logger, "[PLP] Error en la solicitud de destrucción de segmentos. Motivo: %s", strerror(errno));
		free(paqueteSaliente);
		free(orden);
		return 0;
	}

	free(paqueteSaliente);
	free(orden);

	uint32_t bRec = 0;
	t_paquete_programa respuesta;
	log_info(logger, "[PLP] Esperando la respuesta de la UMV.");
	bRec = recvAll(&respuesta, socket_umv);
	log_info(logger, "[PLP] La UMV respondió %s", respuesta.mensaje);

	valorRetorno = atoi(respuesta.mensaje);
	free(respuesta.mensaje);

	return valorRetorno;	
}

void *solicitar_solicitar_bytes(int socket_umv, uint32_t base, uint32_t offset, int tamanio, char origen, t_log *logger)
{
	uint32_t bEnv = 0;
	char *orden = codificar_solicitar_bytes(base,offset,tamanio);
	void *valorRetorno = NULL;

	t_paquete_programa paq_saliente;
		paq_saliente.id = origen;
		paq_saliente.mensaje = orden;
		printf("############## ORDEN ############## : %s", orden);
		paq_saliente.sizeMensaje = strlen(orden);
	
	char *paqueteSaliente = serializar_paquete(&paq_saliente, logger);

	bEnv = paq_saliente.tamanio_total;
	if(sendAll(socket_umv, paqueteSaliente, &bEnv)){
		log_error(logger, "[PLP] Error en la solicitud de solicitar_bytes. Motivo: %s", strerror(errno));
		free(paqueteSaliente);
		free(orden);
		return 0;
	}

	free(paqueteSaliente);
	free(orden);

	uint32_t bRec = 0;
	t_paquete_programa respuesta;
	log_info(logger, "[PLP] Esperando la respuesta de la UMV.");
	bRec = recvAll(&respuesta, socket_umv);
	log_info(logger, "[PLP] La UMV respondió %s", respuesta.mensaje);

	valorRetorno = (void *) respuesta.mensaje;

	return valorRetorno;
}

char *codificar_cambiar_proceso_activo(uint32_t contador_id_programa)
{
	int offset = 0;
	
	char proc_activo[5];
	sprintf(proc_activo, "%d", contador_id_programa);
	char *comando = "cambiar_proceso_activo";

	int proc_activo_len = strlen(proc_activo);
	int comando_len = strlen(comando);

	//longitud del comando + el nulo de fin de cadena + la coma que separa + longitud de proc_activo
	char *orden_completa = calloc(comando_len + 1 + 1 + proc_activo_len, 1);
	memcpy(orden_completa + offset, comando, comando_len);
	offset += comando_len;

	memcpy(orden_completa + offset, ",", 1);
	offset += 1;	

	memcpy(orden_completa + offset, proc_activo, proc_activo_len);
	
	return orden_completa;	
}

char *codificar_crear_segmento(uint32_t id_programa, uint32_t tamanio)
{
	int offset = 0;
	
	char id[5];
	sprintf(id, "%d", id_programa);
	char tam[10];
	sprintf(tam, "%d", tamanio);
	char *comando = "crear_segmento";

	int id_len = strlen(id);
	int tam_len = strlen(tam);
	int comando_len = strlen(comando);

	//longitud del comando + el nulo de fin de cadena + la coma que separa + longitud de tamanio + la coma + longitud de id
	char *orden_completa = calloc(comando_len + 1 + 1 + tam_len + 1 + id_len, 1);
	memcpy(orden_completa + offset, comando, comando_len);
	offset += comando_len;

	memcpy(orden_completa + offset, ",", 1);
	offset += 1;	

	memcpy(orden_completa + offset, id, id_len);
	offset += id_len;

	memcpy(orden_completa + offset, ",", 1);
	offset += 1;

	memcpy(orden_completa + offset, tam, tam_len);

	return orden_completa;
}

char *codificar_enviar_bytes(uint32_t base, uint32_t offset, int tamanio, void *buffer, uint32_t *tamanio_de_la_orden_completa)
{
	int offset_codificacion = 0;

	char str_base[10];
	sprintf(str_base, "%d", base);
	char str_offset[10];
	sprintf(str_offset, "%d", offset);
	char str_tamanio[10];
	sprintf(str_tamanio, "%d", tamanio);
	char *comando = "enviar_bytes";

	int base_len = strlen(str_base);
	int offset_len = strlen(str_offset);
	int tamanio_len = strlen(str_tamanio);
	int comando_len = strlen(comando);
	int buffer_len = tamanio;

	char *orden_completa = calloc(comando_len + 1 + 1 + base_len + 1 + offset_len + 1 + tamanio_len + 1 + buffer_len, 1);
	memcpy(orden_completa + offset_codificacion, comando, comando_len);
	offset_codificacion += comando_len;
	memcpy(orden_completa + offset_codificacion, ",", 1);
	offset_codificacion += 1;

	memcpy(orden_completa + offset_codificacion, str_base, base_len);
	offset_codificacion += base_len;
	memcpy(orden_completa + offset_codificacion, ",", 1);
	offset_codificacion += 1;

	memcpy(orden_completa + offset_codificacion, str_offset, offset_len);
	offset_codificacion += offset_len;
	memcpy(orden_completa + offset_codificacion, ",", 1);
	offset_codificacion += 1;

	memcpy(orden_completa + offset_codificacion, str_tamanio, tamanio_len);
	offset_codificacion += tamanio_len;
	memcpy(orden_completa + offset_codificacion, ",", 1);
	offset_codificacion += 1;

	//hasta acá tengo el control de lo que escribo en orden_completa, después ya no sé si hay comas, nulls, etc. 
	//y sólo me guío por el tamaño del buffer.
	*tamanio_de_la_orden_completa = strlen(orden_completa) + tamanio;

	memcpy(orden_completa + offset_codificacion, buffer, buffer_len);
	offset_codificacion += buffer_len;
	
	return orden_completa;
}

char *codificar_solicitar_bytes(uint32_t base, uint32_t offset, int tamanio)
{
	int offset_codificacion = 0;

	char str_base[10];
	sprintf(str_base, "%d", base);
	char str_offset[10];
	sprintf(str_offset, "%d", offset);
	char str_tamanio[10];
	sprintf(str_tamanio, "%d", tamanio);
	char *comando = "solicitar_bytes";

	int base_len = strlen(str_base);
	int offset_len = strlen(str_offset);
	int tamanio_len = strlen(str_tamanio);
	int comando_len = strlen(comando);
	
	char *orden_completa = calloc(comando_len + 1 + 1 + base_len + 1 + offset_len + 1 + tamanio_len, 1);
	memcpy(orden_completa + offset_codificacion, comando, comando_len);
	offset_codificacion += comando_len;
	memcpy(orden_completa + offset_codificacion, ",", 1);
	offset_codificacion += 1;

	memcpy(orden_completa + offset_codificacion, str_base, base_len);
	offset_codificacion += base_len;
	memcpy(orden_completa + offset_codificacion, ",", 1);
	offset_codificacion += 1;

	memcpy(orden_completa + offset_codificacion, str_offset, offset_len);
	offset_codificacion += offset_len;
	memcpy(orden_completa + offset_codificacion, ",", 1);
	offset_codificacion += 1;

	memcpy(orden_completa + offset_codificacion, str_tamanio, tamanio_len);
	offset_codificacion += tamanio_len;
	
	return orden_completa;	
}

char *codificar_destruir_segmentos(uint32_t id_programa)
{
	int offset_codificacion = 0;

	char str_id_programa[10];
	sprintf(str_id_programa, "%d", id_programa);
	char *comando = "destruir_segmentos";

	int id_programa_len = strlen(str_id_programa);
	int comando_len = strlen(comando);
	
	char *orden_completa = calloc(comando_len + 1 + 1 + id_programa_len, 1);
	memcpy(orden_completa + offset_codificacion, comando, comando_len);
	offset_codificacion += comando_len;
	memcpy(orden_completa + offset_codificacion, ",", 1);
	offset_codificacion += 1;

	memcpy(orden_completa + offset_codificacion, str_id_programa, id_programa_len);
	offset_codificacion += id_programa_len;
	memcpy(orden_completa + offset_codificacion, ",", 1);
	offset_codificacion += 1;
	
	return orden_completa;	
}
