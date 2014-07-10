#include "implementacion_primitivas.h"

/*
uint32_t solicitar_crear_segmento(int socket_umv, uint32_t id_programa, uint32_t tamanio_segmento, char origen, t_log *logger);
uint32_t solicitar_destruir_segmentos(int socket_umv, uint32_t id_programa, char origen, t_log *logger);
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
		//solicitar_cambiar_proceso_activo(socket_umv, pcb.id, 'C', logger);
		
		respuesta = solicitar_enviar_bytes(socket_umv, pcb.seg_stack, offset, 1, nom_var, pcb.id, 'C', logger);
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
		//solicitar_cambiar_proceso_activo(socket_umv, pcb.id, 'C', logger);
		
		ret = (t_valor_variable *) solicitar_solicitar_bytes(socket_umv, pcb.seg_stack, direccion_variable, tamanio_lectura, pcb.id, 'C', logger);
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

		//solicitar_cambiar_proceso_activo(socket_umv, pcb.id, 'C', logger);
		
		respuesta = solicitar_enviar_bytes(socket_umv, pcb.seg_stack, direccion_variable, tamanio_escritura, v, pcb.id, 'C', logger);
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

void irAlLabel(t_nombre_etiqueta nombre_etiqueta)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _irAlLabel (nombre_etiqueta: %s)", nombre_etiqueta);
	int aux = 0;
	char *etiquetas_serializado = malloc(pcb.size_idx_etq);

	//Le saco el \n al final a 'etiqueta' si es que lo tiene. Ver: http://www.campusvirtual.frba.utn.edu.ar/especialidad/mod/forum/discuss.php?d=27715
	aux = strlen(nombre_etiqueta);
	if(nombre_etiqueta[aux - 1] == '\n')
		nombre_etiqueta[aux - 1] = '\0';

	etiquetas_serializado = (char *) solicitar_solicitar_bytes(socket_umv, pcb.seg_idx_etq, 0, pcb.size_idx_etq, pcb.id, 'C', logger);
	if(etiquetas_serializado == NULL){
		log_error(logger, "[CPU] Segmentation fault. La UMV dice que no se pudo leer. (base=%d, offset=%d, tamaño=%d).", pcb.seg_idx_etq, 0, pcb.size_idx_etq);
		salimosPorError = 1;
		return;
	}	

	//actualizamos el program counter
	log_info(logger, "[CPU] Antes de actualizarse por salto, el program counter del proceso %d es %d.", pcb.id, pcb.p_counter);
	pcb.p_counter = metadata_buscar_etiqueta(nombre_etiqueta,etiquetas_serializado,pcb.size_idx_etq);
	log_info(logger, "[CPU] Despues de actualizarse por salto, el program counter del proceso %d es %d.", pcb.id, pcb.p_counter);

	return;
}

void llamarSinRetorno(t_nombre_etiqueta etiqueta)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _llamarSinRetorno (etiqueta: %s)", etiqueta);
	uint32_t nuevoProgramCounter = pcb.p_counter + 1;
	uint32_t nuevoCursorStack = pcb.size_ctxt_actual * 5;
	int respuesta = 0;

	log_info(logger, "[PRIMITIVA] _llamarSinRetorno, el nuevo cursor de stack apunta a: %d", nuevoCursorStack);
	if((respuesta = _pushSinRetorno(nuevoCursorStack, pcb.cursor_stack, nuevoProgramCounter)) < 0){
		if(respuesta == -1){
			log_error(logger, "[CPU] No se encontró el segmento de stack. Muy bizarro. No sigo ni ahí. Olvidate.");	
		} else {
			log_error(logger, "[CPU] Stack overflow. Adiós para siempre.");
		}
				
		salimosPorError = 1;
		
		return;
	}

	pcb.cursor_stack = nuevoCursorStack + 8;

	irAlLabel(etiqueta);

	//vaciamos diccionario de variables
	dictionary_clean(diccionario_variables);
	pcb.size_ctxt_actual = 0;

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

	int respuesta = 0;

	//Si el cursor apunta al comienzo de la pila (offset == 0), e invocaron a esta primitiva, entonces finalizó el programa.
	if(pcb.cursor_stack == 0){
		salimosPorFin = 1;
	} else {

		if((respuesta = _popSinRetorno()) < 0){
			//_popSinRetorno ya brinda suficiente información acerca del error
			salimosPorError = 1;
		}
		
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
	log_info(logger, "[PRIMITIVA] Estoy dentro de _entradaSalida (dispositivo = %s, tiempo = %d)", dispositivo, tiempo);	
	pthread_mutex_t operacion = PTHREAD_MUTEX_INITIALIZER;
	
	pthread_mutex_lock(&operacion);
		//ejemplo: entradaSalida,Impresora,20|	(notar el pipe al final)
		char str_tiempo[10];
		char nombre_syscall[] = "entradaSalida\0";
		sprintf(str_tiempo, "%d", tiempo);
		int offset = 0;
		int nombre_syscall_len = strlen(nombre_syscall);
		int dispositivo_len = strlen(dispositivo);
		int tiempo_len = strlen(str_tiempo);
		char *comando = calloc(nombre_syscall_len + 1 + 1 + dispositivo_len + 1 + 1 + tiempo_len + 1 + 1 + 48, 1); //le agregamos los 48 que pesa el pcb

		memcpy(comando + offset, nombre_syscall, nombre_syscall_len);
		offset += nombre_syscall_len;

		memcpy(comando + offset, ",", 1);
		offset += 1;

		memcpy(comando + offset, dispositivo, dispositivo_len);
		offset += dispositivo_len;

		memcpy(comando + offset, ",", 1);
		offset += 1;

		memcpy(comando + offset, str_tiempo, tiempo_len);
		offset += tiempo_len;
		
		memcpy(comando + offset, "|", 1);

		mi_syscall = comando;
		log_info(logger, "[PRIMITIVA] entradaSalida tira el comando: %s", mi_syscall);

	pthread_mutex_unlock(&operacion);

	salimosPorSyscall = 1;

	return;
}

void wait(t_nombre_semaforo identificador_semaforo)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _wait");

	//envío como mensaje la palabra clave "wait?" que el pcp interpreta como no bloqueante, y me quedo esperando la respuesta

	//si el PCP me dice que hay que bloquear, entonces envío como bloqueante, "wait"

	//si no, entonces continúo la ejecución.

	return;
}

void signal(t_nombre_semaforo identificador_semaforo)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _signal");

	return;
}

int _pushSinRetorno(int puntero_cursor_aux, int cursor_viejo, int program_counter_viejo)
{
	int respuesta = 0;
	
	pthread_mutex_t operacion = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&operacion);
		log_info(logger, "[PRIMITIVA_aux] _pushSinRetorno apila el cursor de contexto viejo: %d", cursor_viejo);
		respuesta = solicitar_enviar_bytes(socket_umv, pcb.seg_stack, puntero_cursor_aux, 4, (void *) &cursor_viejo, pcb.id, 'C', logger);
		if(respuesta == -1){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que este proceso no tiene ningún segmento con base %d.", pcb.seg_stack);
			//acá hay que terminar la ejecución del programa
			return -1;
		} else if(respuesta == -2) {
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que intentar escribir %d bytes en la base %d, offset %d, quedaría fuera de rango.", 8, pcb.seg_stack, puntero_cursor_aux);
			//acá hay que terminar la ejecución del programa
			return -2;
		}

		log_info(logger, "[PRIMITIVA_aux] _pushSinRetorno apila el program counter viejo: %d", program_counter_viejo);
		respuesta = solicitar_enviar_bytes(socket_umv, pcb.seg_stack, puntero_cursor_aux + 4, 4, (void *) &program_counter_viejo, pcb.id, 'C', logger);
		if(respuesta == -1){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que este proceso no tiene ningún segmento con base %d.", pcb.seg_stack);
			//acá hay que terminar la ejecución del programa
			return -1;
		} else if(respuesta == -2) {
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que intentar escribir %d bytes en la base %d, offset %d, quedaría fuera de rango.", 8, pcb.seg_stack, puntero_cursor_aux);
			//acá hay que terminar la ejecución del programa
			return -2;
		}
	pthread_mutex_unlock(&operacion);

	return 0;
}

int _popSinRetorno()
{
	int off_p_counter_anterior = pcb.cursor_stack - 4;
	int off_cursor_stack_anterior = pcb.cursor_stack - 8;
	int valor_contexto_anterior = pcb.cursor_stack - 8;
	int tamanio_ctxt_actual_bytes = 0;
	int tamanio_ctxt_actual = 0;
	int *ret = NULL;

	pthread_mutex_t operacion = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&operacion);
		//Recupero el valor del program counter
		ret = (int *) solicitar_solicitar_bytes(socket_umv, pcb.seg_stack, off_p_counter_anterior, 4, pcb.id, 'C', logger);
		if(ret == NULL){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que no se pudo leer. (base=%d, offset=%d, tamaño=%d).", pcb.seg_stack, off_p_counter_anterior, 4);
			return -1;
		} else {
			pcb.p_counter = *ret;
		}

		//Recupero el valor del puntero al comienzo del contexto
		ret = (int *) solicitar_solicitar_bytes(socket_umv, pcb.seg_stack, off_cursor_stack_anterior, 4, pcb.id, 'C', logger);
		if(ret == NULL){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que no se pudo leer. (base=%d, offset=%d, tamaño=%d).", pcb.seg_stack, off_cursor_stack_anterior, 4);
			return -1;
		} else {
			pcb.cursor_stack = *ret;
		}

		//Calculo el tamaño del diccionario de variables (la cantidad de bytes debería ser múltiplo de 5)
		tamanio_ctxt_actual_bytes = valor_contexto_anterior - pcb.cursor_stack;
		pcb.size_ctxt_actual = (tamanio_ctxt_actual_bytes / 5);
		
		if(tamanio_ctxt_actual_bytes % 5)
			log_warning(logger, "[CPU] El tamaño del contexto actual en bytes es de %d, y no es múltiplo de 5", tamanio_ctxt_actual_bytes);

		generarDiccionarioVariables();

	pthread_mutex_unlock(&operacion);	

	return 0;
}
