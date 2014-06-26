#include "implementacion_primitivas.h"

/*
uint32_t solicitar_cambiar_proceso_activo(int socket_umv, uint32_t contador_id_programa, char origen, t_log *logger);
uint32_t solicitar_crear_segmento(int socket_umv, uint32_t id_programa, uint32_t tamanio_segmento, char origen, t_log *logger);
uint32_t solicitar_enviar_bytes(int socket_umv, uint32_t base, uint32_t offset, int tamanio, void *buffer, char origen, t_log *logger);
uint32_t solicitar_destruir_segmentos(int socket_umv, uint32_t id_programa, char origen, t_log *logger);
void *solicitar_solicitar_bytes(int socket_umv, uint32_t base, uint32_t offset, int tamanio, char origen, t_log *logger);
*/

t_puntero definirVariable(t_nombre_variable identificador_variable)
{
	//t_puntero == u_int32_t
	//t_nombre_variable = char
	log_info(logger, "[PRIMITIVA] Estoy dentro de _definirVariable (identificador_variable = %c)", identificador_variable);
	pthread_mutex_t operacion = PTHREAD_MUTEX_INITIALIZER;
	int respuesta = 0;
	int tamanio_escritura = 5;
	int offset = pcb.size_ctxt_actual*5;
	
	t_puntero *puntero_al_valor = malloc(sizeof(t_puntero));
	t_nombre_variable *nom_var = malloc(sizeof(t_nombre_variable) + 1);

	*puntero_al_valor = offset + 1;
	memset(nom_var, identificador_variable, 1);
	memset((nom_var + 1), '\0', 1);

	pthread_mutex_lock(&operacion);
		solicitar_cambiar_proceso_activo(socket_umv, pcb.id, 'C', logger);
		
		respuesta = solicitar_enviar_bytes(socket_umv, pcb.seg_stack, offset, 1, nom_var, 'C', logger);
		if(respuesta == -1){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que este proceso no tiene ningún segmento con base %d.", pcb.seg_stack);
			//acá hay que terminar la ejecución del programa
			return 0;
		} else if(respuesta == -2) {
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que intentar escribir %d bytes en la base %d, offset %d, quedaría fuera de rango.", tamanio_escritura, pcb.seg_stack, offset);
			//acá hay que terminar la ejecución del programa
			return 0;
		} else {
			dictionary_put(diccionario_variables, nom_var, puntero_al_valor);
			log_info(logger, "[PRIMITIVA] _definirVariable agregó al diccionario (k=%c, v=%d)", *nom_var, *puntero_al_valor);
			pcb.size_ctxt_actual = pcb.size_ctxt_actual + 1;
		}
	pthread_mutex_unlock(&operacion);

	log_info(logger, "[PRIMITIVA] _definirVariable retorna: %d.", identificador_variable);
	return *puntero_al_valor;
}

t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable)
{
	//t_puntero == u_int32_t
	log_info(logger, "[PRIMITIVA] Estoy dentro de _obtenerPosicionVariable (identificador_variable = %c)", identificador_variable);
	log_info(logger, "[PRIMITIVA] El tamaño del diccionario de variables es de: %d", dictionary_size(diccionario_variables));

	t_puntero *posicion = malloc(sizeof(t_puntero));
	t_nombre_variable *nom_var = malloc(sizeof(t_nombre_variable) + 1);

	//*nom_var = identificador_variable;
	memset(nom_var, identificador_variable, 1);
	memset((nom_var + 1), '\0', 1);

	posicion = (t_puntero *) dictionary_get(diccionario_variables, nom_var);

	log_info(logger, "[PRIMITIVA] _obtenerPosicionVariable retorna: %d.", *posicion);
	return *posicion;
}

t_valor_variable dereferenciar(t_puntero direccion_variable)
{
	//t_valor_variable == int
	log_info(logger, "[PRIMITIVA] Estoy dentro de _dereferenciar (direccion_variable = %d).", direccion_variable);
	
	pthread_mutex_t operacion = PTHREAD_MUTEX_INITIALIZER;
	t_valor_variable *ret = NULL;
	int tamanio_lectura = sizeof(t_valor_variable);
	
	pthread_mutex_lock(&operacion);
		solicitar_cambiar_proceso_activo(socket_umv, pcb.id, 'C', logger);
		
		ret = (t_valor_variable *) solicitar_solicitar_bytes(socket_umv, pcb.seg_stack, direccion_variable, tamanio_lectura, 'C', logger);
		if(ret == NULL){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que no se pudo leer. (base=%d, offset=%d, tamaño=%d).", pcb.seg_stack, direccion_variable, tamanio_lectura);
			//acá hay que terminar la ejecución del programa
			return 0;
		}
	
	pthread_mutex_unlock(&operacion);

	log_info(logger, "[PRIMITIVA] _dereferenciar retorna: %d.", *ret);
	return *ret;
}

void asignar(t_puntero direccion_variable, t_valor_variable valor)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _asignar (direccion_variable = %d, valor = %d)", direccion_variable, valor);
	pthread_mutex_t operacion = PTHREAD_MUTEX_INITIALIZER;
	int tamanio_escritura = sizeof(t_valor_variable);
	int respuesta = 0;
	t_valor_variable *v = malloc(tamanio_escritura);
	*v = valor;

	pthread_mutex_lock(&operacion);

		solicitar_cambiar_proceso_activo(socket_umv, pcb.id, 'C', logger);
		
		respuesta = solicitar_enviar_bytes(socket_umv, pcb.seg_stack, direccion_variable, tamanio_escritura, v, 'C', logger);
		if(respuesta == -1){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que este proceso no tiene ningún segmento con base %d.", pcb.seg_stack);
			//acá hay que terminar la ejecución del programa
			return;
		} else if(respuesta == -2) {
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que intentar escribir %d bytes en la base %d, offset %d, quedaría fuera de rango.", tamanio_escritura, pcb.seg_stack, direccion_variable);
			//acá hay que terminar la ejecución del programa
			return;
		} else {
			log_info(logger, "[PRIMITIVA] _asignar cambió correctamente el valor en la posición %d a %d.", direccion_variable, valor);
		}

	pthread_mutex_unlock(&operacion);

	return;
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable)
{
	//t_valor_variable == int
	log_info(logger, "[PRIMITIVA] Estoy dentro de _obtenerValorCompartida");

	return 0;
}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor)
{
	//t_valor_variable == int
	log_info(logger, "[PRIMITIVA] Estoy dentro de _asignarValorCompartida");

	return 0;
}

void irAlLabel(t_nombre_etiqueta t_nombre_etiqueta)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _irAlLabel");

	return;
}

void llamarSinRetorno(t_nombre_etiqueta etiqueta)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _llamarSinRetorno");

	return;
}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _llamarConRetorno");

	return;
}

void finalizar(void)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _finalizar");

	/*
	**	Cuando manejemos varios contextos, acá habría que volver al cursor stack y al program counter apilados previamente.
	**  Por ahora no manejamos este caso.
	*/

	//Pero si el cursor apunta al comienzo de la pila (offset == 0), e invocaron a esta primitiva, entonces finalizó el programa.
	if(pcb.cursor_stack == 0){
		salimosPorFin = 1;
	}

	return;
}

void retornar(t_valor_variable retorno)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _retornar");

	return;
}

void imprimir(t_valor_variable valor_mostrar)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _imprimir");

	return;
}

void imprimirTexto(char* texto)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _imprimirTexto");

	return;
}

void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _entradaSalida");

	return;
}

void wait(t_nombre_semaforo identificador_semaforo)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _wait");

	return;
}

void signal(t_nombre_semaforo identificador_semaforo)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _signal");

	return;
}
