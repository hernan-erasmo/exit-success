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
	//int offset = pcb.size_ctxt_actual*5;	//este era el viejo, que andaba bien cuando sólo existía 'llamarSinRetorno'
	int offset = pcb.cursor_stack + pcb.size_ctxt_actual*5;
	
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
			salimosPorError = 1;
			return 0;
		} else if(respuesta == -2) {
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que intentar escribir %d bytes en la base %d, offset %d, quedaría fuera de rango.", tamanio_escritura, pcb.seg_stack, offset);
			salimosPorError = 1;
			return 0;
		} else {
			dictionary_put(diccionario_variables, nom_var, puntero_al_valor);
			//log_info(logger, "[PRIMITIVA] _definirVariable agregó al diccionario (k=%c, v=%d)", *nom_var, *puntero_al_valor);
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
	//log_info(logger, "[PRIMITIVA] El tamaño del diccionario de variables es de: %d", dictionary_size(diccionario_variables));

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
		if(*ret == -1){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que no se pudo leer. (base=%d, offset=%d, tamaño=%d).", pcb.seg_stack, direccion_variable, tamanio_lectura);
			salimosPorError = 1;
			return 0;
		} else if(*ret == -2){
			log_error(logger, "[CPU] Se quiso leer por fuera de los límites del segmento con base %d, offset %d y tamaño de lectura %d", pcb.seg_stack, direccion_variable, tamanio_lectura);
			salimosPorError = 1;
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
			salimosPorError = 1;
			return;
		} else if(respuesta == -2) {
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que intentar escribir %d bytes en la base %d, offset %d, quedaría fuera de rango.", tamanio_escritura, pcb.seg_stack, direccion_variable);
			salimosPorError = 1;
			return;
		} else {
			log_info(logger, "[PRIMITIVA] _asignar cambió correctamente el valor en la posición %d a %d.", direccion_variable, valor);
		}

	pthread_mutex_unlock(&operacion);

	return;
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable)
{
	pthread_mutex_t obtener_mutex = PTHREAD_MUTEX_INITIALIZER;
	//t_valor_variable == int
	
	pthread_mutex_lock(&obtener_mutex);
		log_info(logger, "[PRIMITIVA] Estoy dentro de _obtenerValorCompartida (variable: %s)", variable);

		char nombre_syscall[] = "obtenerValorCompartida\0";
		int nombre_syscall_len = strlen(nombre_syscall);
		int variable_len = strlen(variable);
		char *paqueteSerializado, *comando = calloc(nombre_syscall_len + 1 + 1 + variable_len + 1 + 1, 1);
		int offset = 0;
		int bEnv = 0;
		int aux = 0;

		aux = strlen(variable);
		if(variable[aux - 1] == '\n')
			variable[aux - 1] = '\0';

		memcpy(comando + offset, nombre_syscall, nombre_syscall_len);
		offset += nombre_syscall_len;

		memcpy(comando + offset, ",", 1);
		offset += 1;

		memcpy(comando + offset, variable, variable_len);
		offset += variable_len;

		t_paquete_programa paq;
			paq.id = 'S';	//porque el pcp reconoce que es una syscall si le mandás 'S'
			paq.mensaje = comando;
			paq.sizeMensaje = strlen(comando);

		paqueteSerializado = serializar_paquete(&paq, logger);
		bEnv = paq.tamanio_total;
		if(sendAll(socket_pcp, paqueteSerializado, &bEnv)){
			log_error(logger, "[PRIMITIVA_obtenerValorCompartida] Hubo un error al tratar de enviar el syscall al PCP");
		}

		free(paq.mensaje);

		/*
		**	Ahora espero la respuesta del Kernel
		*/
		//log_info(logger, "[PRIMITIVA_obtenerValorCompartida] Estoy esperando el valor de \'%s\'", variable);
		int status = 0;
		t_paquete_programa respuesta;
		while(1)
		{
			status = recvAll(&respuesta, socket_pcp);
			if(status){
				log_info(logger, "[PRIMITIVA_obtenerValorCompartida] La variable global \'%s\' vale %s", variable, respuesta.mensaje);
				return atoi(respuesta.mensaje);
			} else {
				log_error(logger, "[PRIMITIVA_obtenerValorCompartida] El socket PCP cerró su conexión de manera inesperada.");
				break;
			}
		}
	pthread_mutex_unlock(&obtener_mutex);
	
	return 0;
}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor)
{
	//t_valor_variable == int
	pthread_mutex_t asignar_mutex = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&asignar_mutex);
		log_info(logger, "[PRIMITIVA] Estoy dentro de _asignarValorCompartida (variable: %s, valor: %d)", variable, valor);

		char nombre_syscall[] = "asignarValorCompartida\0";
		char valor_serializado[10];
		sprintf(valor_serializado, "%d", valor);
		int valor_serializado_len = strlen(valor_serializado);
		int nombre_syscall_len = strlen(nombre_syscall);
		int variable_len = strlen(variable);
		char *paqueteSerializado, *comando = calloc(nombre_syscall_len + 1 + 1 + variable_len + 1 + 1 + valor_serializado_len + 1 + 1, 1);
		int offset = 0;
		int bEnv = 0;
		int aux = 0;

		aux = strlen(variable);
		if(variable[aux - 1] == '\n')
			variable[aux - 1] = '\0';

		memcpy(comando + offset, nombre_syscall, nombre_syscall_len);
		offset += nombre_syscall_len;

		memcpy(comando + offset, ",", 1);
		offset += 1;

		memcpy(comando + offset, variable, variable_len);
		offset += variable_len;

		memcpy(comando + offset, ",", 1);
		offset += 1;

		memcpy(comando + offset, valor_serializado, valor_serializado_len);
		offset += valor_serializado_len;

		t_paquete_programa paq;
			paq.id = 'S';	//porque el pcp reconoce que es una syscall si le mandás 'S'
			paq.mensaje = comando;
			paq.sizeMensaje = strlen(comando);

		paqueteSerializado = serializar_paquete(&paq, logger);
		bEnv = paq.tamanio_total;
		if(sendAll(socket_pcp, paqueteSerializado, &bEnv)){
			log_error(logger, "[PRIMITIVA_asignarValorCompartida] Hubo un error al tratar de enviar la syscall al PCP");
		}

		free(paq.mensaje);

		/*
		**	Ahora espero la respuesta del Kernel
		*/
		//log_info(logger, "[PRIMITIVA_asignarValorCompartida] Estoy esperando el nuevo valor de \'%s\'", variable);
		int status = 0;
		t_paquete_programa respuesta;
		while(1)
		{
			status = recvAll(&respuesta, socket_pcp);
			if(status){
				log_info(logger, "[PRIMITIVA_asignarValorCompartida] La variable global \'%s\' ahora vale %s", variable, respuesta.mensaje);
				return atoi(respuesta.mensaje);
			} else {
				log_error(logger, "[PRIMITIVA_asignarValorCompartida] El socket PCP cerró su conexión de manera inesperada.");
				break;
			}
		}
	pthread_mutex_unlock(&asignar_mutex);

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
	if(etiquetas_serializado == "-1"){
		log_error(logger, "[CPU] La UMV dice que no existe el segmento %d)", pcb.seg_idx_etq);
		salimosPorError = 1;
		return;
	} else if(etiquetas_serializado == "-2"){
		log_error(logger, "[CPU] La UMV dice que se quiso leer por fuera de los límites del segmento %d, con base %d y offset %d)", pcb.seg_idx_etq, 0, pcb.size_idx_etq);
		salimosPorError = 1;
		return;
	}

	//actualizamos el program counter
	//log_info(logger, "[PRIMITIVA_irAlLabel] Antes de actualizarse por salto, el program counter del proceso %d es %d.", pcb.id, pcb.p_counter);
	pcb.p_counter = metadata_buscar_etiqueta(nombre_etiqueta,etiquetas_serializado,pcb.size_idx_etq);
	debo_actualizar_manualmente_p_counter = 0;
	//log_info(logger, "[PRIMITIVA_irAlLabel] Despues de actualizarse por salto, el program counter del proceso %d es %d.", pcb.id, pcb.p_counter);

	return;
}

void llamarSinRetorno(t_nombre_etiqueta etiqueta)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _llamarSinRetorno (etiqueta: %s)", etiqueta);
	uint32_t nuevoProgramCounter = pcb.p_counter + 1;
	uint32_t nuevoCursorStack = pcb.cursor_stack + pcb.size_ctxt_actual * 5;
	int respuesta = 0;

	//log_info(logger, "[PRIMITIVA] _llamarSinRetorno, el nuevo cursor de stack apunta a: %d", nuevoCursorStack);
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
	log_info(logger, "[PRIMITIVA] Estoy dentro de _llamarConRetorno (etiqueta: %s, donde_retornar: %d)", etiqueta, donde_retornar);
	uint32_t nuevoProgramCounter = pcb.p_counter + 1;
	uint32_t nuevoCursorStack = pcb.cursor_stack + pcb.size_ctxt_actual * 5;
	int respuesta = 0;

	//log_info(logger, "[PRIMITIVA] _llamarConRetorno, el nuevo cursor de stack apunta a: %d", nuevoCursorStack);
	if((respuesta = _pushConRetorno(nuevoCursorStack, pcb.cursor_stack, nuevoProgramCounter, donde_retornar)) < 0){
		if(respuesta == -1){
			log_error(logger, "[CPU] No se encontró el segmento de stack. Muy bizarro. No sigo ni ahí. Olvidate.");	
		} else {
			log_error(logger, "[CPU] Stack overflow. Adiós para siempre.");
		}
				
		salimosPorError = 1;
		
		return;
	}

	pcb.cursor_stack = nuevoCursorStack + 12;

	irAlLabel(etiqueta);

	//vaciamos diccionario de variables
	dictionary_clean(diccionario_variables);
	pcb.size_ctxt_actual = 0;

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
	log_info(logger, "[PRIMITIVA] Estoy dentro de _retornar (retorno: %d)", retorno);

	int respuesta_pop = 0;
	int respuesta_escritura;

	if((respuesta_pop = _popConRetorno()) < 0){
		//_popConRetorno ya brinda suficiente información acerca del error
		salimosPorError = 1;
	} else {
		//'respuesta_pop' contiene el offset respecto al comienzo del contexto actual
		// donde se debe escribir el valor de 'retorno' que viene como parámetro de esta función
		respuesta_escritura = solicitar_enviar_bytes(socket_umv, pcb.seg_stack, respuesta_pop, 4, (void *) &retorno, pcb.id, 'C', logger);
		if(respuesta_escritura == -1){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que este proceso no tiene ningún segmento con base %d.", respuesta_pop);
			salimosPorError = 1;
			return;
		} else if(respuesta_escritura == -2) {
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que intentar escribir %d bytes en la base %d, offset %d, quedaría fuera de rango.", 4, respuesta_pop, 0);
			salimosPorError = 1;
			return;
		}

		/*
		** A diferencia del funcionamiento de 'llamarSinRetorno' y 'popSinRetorno' la generación del diccionario de variables
		** se realiza acá, después de 'llamarConRetorno' pero más importante aún, después de actualizar el valor de la variable
		** que recibe el valor de retorno. En 'popSinRetorno', no se modifica ningún valor de variable una vez desapilados los datos. Aquí si.
		*/
		generarDiccionarioVariables();
	}

	return;
}

void imprimir(t_valor_variable valor_mostrar)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _imprimir (valor_mostrar: %d)", valor_mostrar);
	pthread_mutex_t operacion = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&operacion);
		char nombre_syscall[] = "imprimir\0";
		char str_valor_mostrar[10];
			sprintf(str_valor_mostrar, "%d", valor_mostrar);
		char str_socket_programa[10];
			sprintf(str_socket_programa, "%d", pcb.socket);
		int socket_programa_len = strlen(str_socket_programa);
		int valor_mostrar_len = strlen(str_valor_mostrar);
		int nombre_syscall_len = strlen(nombre_syscall);
		char *paqueteSerializado, *comando = calloc(nombre_syscall_len + 1 + 1 + valor_mostrar_len + 1 + 1 + socket_programa_len + 1 + 1, 1);
		int offset = 0;
		int bEnv = 0;
		int aux = 0;

		memcpy(comando + offset, nombre_syscall, nombre_syscall_len);
		offset += nombre_syscall_len;

		memcpy(comando + offset, ",", 1);
		offset += 1;

		memcpy(comando + offset, str_socket_programa, socket_programa_len);
		offset += socket_programa_len;

		memcpy(comando + offset, ",", 1);
		offset += 1;

		memcpy(comando + offset, str_valor_mostrar, valor_mostrar_len);
		offset += valor_mostrar_len;

		t_paquete_programa paq;
			paq.id = 'S';	//porque el pcp reconoce que es una syscall si le mandás 'S'
			paq.mensaje = comando;
			paq.sizeMensaje = strlen(comando);

		paqueteSerializado = serializar_paquete(&paq, logger);
		bEnv = paq.tamanio_total;
		if(sendAll(socket_pcp, paqueteSerializado, &bEnv)){
			log_error(logger, "[PRIMITIVA_imprimir] Hubo un error al tratar de enviar la syscall al PCP");
		}

		free(paq.mensaje);
		
	pthread_mutex_unlock(&operacion);

	return;
}

void imprimirTexto(char* texto)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _imprimirTexto (texto: %s)", texto);

	pthread_mutex_t operacion = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&operacion);
		char nombre_syscall[] = "imprimirTexto\0";
		char str_socket_programa[10];
			sprintf(str_socket_programa, "%d", pcb.socket);
		int socket_programa_len = strlen(str_socket_programa);
		int texto_len = strlen(texto);
		int nombre_syscall_len = strlen(nombre_syscall);
		char *paqueteSerializado, *comando = calloc(nombre_syscall_len + 1 + 1 + texto_len + 1 + 1 + socket_programa_len + 1 + 1, 1);
		int offset = 0;
		int bEnv = 0;
		int aux = 0;

		memcpy(comando + offset, nombre_syscall, nombre_syscall_len);
		offset += nombre_syscall_len;

		memcpy(comando + offset, ",", 1);
		offset += 1;

		memcpy(comando + offset, str_socket_programa, socket_programa_len);
		offset += socket_programa_len;

		memcpy(comando + offset, ",", 1);
		offset += 1;

		memcpy(comando + offset, texto, texto_len);
		offset += texto_len;

		t_paquete_programa paq;
			paq.id = 'S';	//porque el pcp reconoce que es una syscall si le mandás 'S'
			paq.mensaje = comando;
			paq.sizeMensaje = strlen(comando);

		paqueteSerializado = serializar_paquete(&paq, logger);
		bEnv = paq.tamanio_total;
		if(sendAll(socket_pcp, paqueteSerializado, &bEnv)){
			log_error(logger, "[PRIMITIVA_imprimirTexto] Hubo un error al tratar de enviar la syscall al PCP");
		}

		free(paq.mensaje);
		
	pthread_mutex_unlock(&operacion);

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
		//log_info(logger, "[PRIMITIVA] entradaSalida tira el comando: %s", mi_syscall);

	pthread_mutex_unlock(&operacion);

	salimosPorSyscallBloqueante = 1;

	return;
}


/*
** Envío como mensaje la palabra clave "wait?" que el pcp interpreta como no bloqueante, y me quedo esperando la respuesta
** si el PCP me dice que hay que bloquear, entonces envío como bloqueante, "wait"
** si no, entonces continúo la ejecución.
*/
void wait(t_nombre_semaforo identificador_semaforo)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _wait (identificador_semaforo: %s)", identificador_semaforo);

	char nombre_syscall[] = "wait?\0";
	int nombre_syscall_len = strlen(nombre_syscall);
	int identificador_semaforo_len = strlen(identificador_semaforo);
	char *paqueteSerializado, *comando = calloc(nombre_syscall_len + 1 + 1 + identificador_semaforo_len + 1 + 1, 1);
	int offset = 0;
	int bEnv = 0;
	int aux = 0;

	aux = strlen(identificador_semaforo);
	if(identificador_semaforo[aux - 1] == '\n')
		identificador_semaforo[aux - 1] = '\0';

	memcpy(comando + offset, nombre_syscall, nombre_syscall_len);
	offset += nombre_syscall_len;

	memcpy(comando + offset, ",", 1);
	offset += 1;

	memcpy(comando + offset, identificador_semaforo, identificador_semaforo_len);
	offset += identificador_semaforo_len;

	t_paquete_programa paq;
		paq.id = 'S';	//porque el pcp reconoce que es una syscall si le mandás 'S'
		paq.mensaje = comando;
		paq.sizeMensaje = strlen(comando);

	paqueteSerializado = serializar_paquete(&paq, logger);
	bEnv = paq.tamanio_total;
	if(sendAll(socket_pcp, paqueteSerializado, &bEnv)){
		log_error(logger, "[PRIMITIVA_wait?] Hubo un error al tratar de enviar la syscall al PCP");
	}

	free(paq.mensaje);

	/*
	**	Ahora espero la respuesta del Kernel
	*/
	//log_info(logger, "[PRIMITIVA_wait?] Estoy esperando la respuesta para ver si me bloqueo o no.");
	int status = 0;
	t_paquete_programa respuesta;
	while(1)
	{
		int respuesta_pcp = -1;
		status = recvAll(&respuesta, socket_pcp);
		if(status){
			
			respuesta_pcp = atoi(respuesta.mensaje);
			if(respuesta_pcp > 0){
				log_info(logger, "[PRIMITIVA_wait?] Obtuve el semáforo, sigo trabajando sin bloquearme.");
				break;
			} else if(respuesta_pcp < 0){
				log_error(logger, "[PRIMITIVA_wait?] ¿El semáforo no existe? La respuesta fue -1.");
				break;
			} else {
				log_info(logger, "[PRIMITIVA_wait?] El semáforo está bloqueado, tengo que mandar el PCB y salirme de la CPU");
				_wait_bloqueante(identificador_semaforo);
				break;
			}
		} else {
			log_error(logger, "[PRIMITIVA_wait?] El socket PCP cerró su conexión de manera inesperada.");
			break;
		}
	}

	return;
}

void prim_signal(t_nombre_semaforo identificador_semaforo)
{
	log_info(logger, "[PRIMITIVA] Estoy dentro de _signal (identificador_semaforo: %s)", identificador_semaforo);

	pthread_mutex_t operacion = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&operacion);
		char nombre_syscall[] = "signal\0";
		int nombre_syscall_len = strlen(nombre_syscall);
		int identificador_semaforo_len = strlen(identificador_semaforo);
		char *paqueteSerializado, *comando = calloc(nombre_syscall_len + 1 + 1 + identificador_semaforo_len + 1 + 1, 1);
		int offset = 0;
		int bEnv = 0;
		int aux = 0;

		aux = strlen(identificador_semaforo);
		if(identificador_semaforo[aux - 1] == '\n')
			identificador_semaforo[aux - 1] = '\0';

		memcpy(comando + offset, nombre_syscall, nombre_syscall_len);
		offset += nombre_syscall_len;

		memcpy(comando + offset, ",", 1);
		offset += 1;

		memcpy(comando + offset, identificador_semaforo, identificador_semaforo_len);
		offset += identificador_semaforo_len;

		t_paquete_programa paq;
			paq.id = 'S';	//porque el pcp reconoce que es una syscall si le mandás 'S'
			paq.mensaje = comando;
			paq.sizeMensaje = strlen(comando);

		paqueteSerializado = serializar_paquete(&paq, logger);
		bEnv = paq.tamanio_total;
		if(sendAll(socket_pcp, paqueteSerializado, &bEnv)){
			log_error(logger, "[PRIMITIVA_signal] Hubo un error al tratar de enviar la syscall al PCP");
		}

		free(paq.mensaje);
		
	pthread_mutex_unlock(&operacion);

	return;
}

int _pushConRetorno(int puntero_cursor_aux, int cursor_viejo, int program_counter_viejo, t_puntero donde_retornar){
	int respuesta = 0;

	pthread_mutex_t operacion = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&operacion);
		//log_info(logger, "[PRIMITIVA_aux] _pushConRetorno apila el cursor de contexto viejo: %d", cursor_viejo);
		respuesta = solicitar_enviar_bytes(socket_umv, pcb.seg_stack, puntero_cursor_aux, 4, (void *) &cursor_viejo, pcb.id, 'C', logger);
		if(respuesta == -1){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que este proceso no tiene ningún segmento con base %d.", pcb.seg_stack);
			salimosPorError = 1;
			return -1;
		} else if(respuesta == -2) {
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que intentar escribir %d bytes en la base %d, offset %d, quedaría fuera de rango.", 8, pcb.seg_stack, puntero_cursor_aux);
			salimosPorError = 1;
			return -2;
		}

		//log_info(logger, "[PRIMITIVA_aux] _pushConRetorno apila el program counter viejo: %d", program_counter_viejo);
		respuesta = solicitar_enviar_bytes(socket_umv, pcb.seg_stack, puntero_cursor_aux + 4, 4, (void *) &program_counter_viejo, pcb.id, 'C', logger);
		if(respuesta == -1){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que este proceso no tiene ningún segmento con base %d.", pcb.seg_stack);
			salimosPorError = 1;
			return -1;
		} else if(respuesta == -2) {
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que intentar escribir %d bytes en la base %d, offset %d, quedaría fuera de rango.", 8, pcb.seg_stack, puntero_cursor_aux);
			salimosPorError = 1;
			return -2;
		}

		//log_info(logger, "[PRIMITIVA_aux] _pushConRetorno apila la dirección donde retornar el valor: %d", donde_retornar);
		respuesta = solicitar_enviar_bytes(socket_umv, pcb.seg_stack, puntero_cursor_aux + 8, 4, (void *) &donde_retornar, pcb.id, 'C', logger);
		if(respuesta == -1){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que este proceso no tiene ningún segmento con base %d.", pcb.seg_stack);
			salimosPorError = 1;
			return -1;
		} else if(respuesta == -2) {
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que intentar escribir %d bytes en la base %d, offset %d, quedaría fuera de rango.", 8, pcb.seg_stack, puntero_cursor_aux);
			salimosPorError = 1;
			return -2;
		}
	pthread_mutex_unlock(&operacion);

	return 0;
}

int _pushSinRetorno(int puntero_cursor_aux, int cursor_viejo, int program_counter_viejo)
{
	int respuesta = 0;
	
	pthread_mutex_t operacion = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&operacion);
		//log_info(logger, "[PRIMITIVA_aux] _pushSinRetorno apila el cursor de contexto viejo: %d", cursor_viejo);
		respuesta = solicitar_enviar_bytes(socket_umv, pcb.seg_stack, puntero_cursor_aux, 4, (void *) &cursor_viejo, pcb.id, 'C', logger);
		if(respuesta == -1){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que este proceso no tiene ningún segmento con base %d.", pcb.seg_stack);
			salimosPorError = 1;
			return -1;
		} else if(respuesta == -2) {
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que intentar escribir %d bytes en la base %d, offset %d, quedaría fuera de rango.", 8, pcb.seg_stack, puntero_cursor_aux);
			salimosPorError = 1;
			return -2;
		}

		//log_info(logger, "[PRIMITIVA_aux] _pushSinRetorno apila el program counter viejo: %d", program_counter_viejo);
		respuesta = solicitar_enviar_bytes(socket_umv, pcb.seg_stack, puntero_cursor_aux + 4, 4, (void *) &program_counter_viejo, pcb.id, 'C', logger);
		if(respuesta == -1){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que este proceso no tiene ningún segmento con base %d.", pcb.seg_stack);
			salimosPorError = 1;
			return -1;
		} else if(respuesta == -2) {
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que intentar escribir %d bytes en la base %d, offset %d, quedaría fuera de rango.", 8, pcb.seg_stack, puntero_cursor_aux);
			salimosPorError = 1;
			return -2;
		}
	pthread_mutex_unlock(&operacion);

	return 0;
}

int _popConRetorno()
{
	int off_dir_guardar_retorno = pcb.cursor_stack - 4;
	int off_p_counter_anterior = pcb.cursor_stack - 8;
	int off_cursor_stack_anterior = pcb.cursor_stack - 12;
	int fin_contexto_anterior = pcb.cursor_stack - 12;
	int tamanio_ctxt_actual_bytes = 0;
	int tamanio_ctxt_actual = 0;
	int *ret = NULL;
	int valorRetorno = 0;

	pthread_mutex_t operacion = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&operacion);
		//Recupero el valor de la posición donde tengo que almacenar el retorno.
		ret = (int *) solicitar_solicitar_bytes(socket_umv, pcb.seg_stack, off_dir_guardar_retorno, 4, pcb.id, 'C', logger);
		if(*ret == -1){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que no se pudo leer. (base=%d, offset=%d, tamaño=%d).", pcb.seg_stack, off_dir_guardar_retorno, 4);
			return -1;
		} else if(*ret == -2){
			log_error(logger, "[CPU] Se quiso leer por fuera de los límites del segmento con base %d, offset %d y tamaño de lectura %d", pcb.seg_stack, off_dir_guardar_retorno, 4);
			return -1;
		} else {
			valorRetorno = *ret;
		}

		//Recupero el valor del program counter
		ret = (int *) solicitar_solicitar_bytes(socket_umv, pcb.seg_stack, off_p_counter_anterior, 4, pcb.id, 'C', logger);
		if(*ret == -1){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que no se pudo leer. (base=%d, offset=%d, tamaño=%d).", pcb.seg_stack, off_p_counter_anterior, 4);
			return -1;
		} else if(*ret == -2){
			log_error(logger, "[CPU] Se quiso leer por fuera de los límites del segmento con base %d, offset %d y tamaño de lectura %d", pcb.seg_stack, off_p_counter_anterior, 4);
			return -1;
		} else {
			//log_info(logger, "[PRIMITIVA_aux] _popConRetorno: El program counter del proceso %d valía %d y lo voy a actualizar.", pcb.id, pcb.p_counter);
			pcb.p_counter = *ret;
			debo_actualizar_manualmente_p_counter = 0;
			//log_info(logger, "[PRIMITIVA_aux] _popConRetorno: El program counter del proceso %d ahora vale %d.", pcb.id, pcb.p_counter);
		}

		//Recupero el valor del puntero al comienzo del contexto
		ret = (int *) solicitar_solicitar_bytes(socket_umv, pcb.seg_stack, off_cursor_stack_anterior, 4, pcb.id, 'C', logger);
		if(*ret == -1){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que no se pudo leer. (base=%d, offset=%d, tamaño=%d).", pcb.seg_stack, off_cursor_stack_anterior, 4);
			return -1;
		} else if(*ret == -2){
			log_error(logger, "[CPU] Se quiso leer por fuera de los límites del segmento con base %d, offset %d y tamaño de lectura %d", pcb.seg_stack, off_cursor_stack_anterior, 4);
			return -1;
		} else {
			pcb.cursor_stack = *ret;
		}

		//Calculo el tamaño del diccionario de variables (la cantidad de bytes debería ser múltiplo de 5)
		tamanio_ctxt_actual_bytes = fin_contexto_anterior - pcb.cursor_stack;
		pcb.size_ctxt_actual = (tamanio_ctxt_actual_bytes / 5);
		
		if(tamanio_ctxt_actual_bytes % 5)
			log_warning(logger, "[CPU] El tamaño del contexto actual en bytes es de %d, y no es múltiplo de 5", tamanio_ctxt_actual_bytes);

	pthread_mutex_unlock(&operacion);

	return valorRetorno;
}

int _popSinRetorno()
{
	int off_p_counter_anterior = pcb.cursor_stack - 4;
	int off_cursor_stack_anterior = pcb.cursor_stack - 8;
	int fin_contexto_anterior = pcb.cursor_stack - 8;
	int tamanio_ctxt_actual_bytes = 0;
	int tamanio_ctxt_actual = 0;
	int *ret = NULL;

	pthread_mutex_t operacion = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&operacion);
		//Recupero el valor del program counter
		ret = (int *) solicitar_solicitar_bytes(socket_umv, pcb.seg_stack, off_p_counter_anterior, 4, pcb.id, 'C', logger);
		if(*ret == -1){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que no se pudo leer. (base=%d, offset=%d, tamaño=%d).", pcb.seg_stack, off_p_counter_anterior, 4);
			return -1;
		} else if(*ret == -2){
			log_error(logger, "[CPU] Se quiso leer por fuera de los límites del segmento con base %d, offset %d y tamaño de lectura %d", pcb.seg_stack, off_p_counter_anterior, 4);
			return -1;
		} else {
			//log_info(logger, "[PRIMITIVA_aux] _popSinRetorno: El program counter del proceso %d valía %d y lo voy a actualizar.", pcb.id, pcb.p_counter);
			pcb.p_counter = *ret;
			debo_actualizar_manualmente_p_counter = 0;
			//log_info(logger, "[PRIMITIVA_aux] _popSinRetorno: El program counter del proceso %d ahora vale %d.", pcb.id, pcb.p_counter);
		}

		//Recupero el valor del puntero al comienzo del contexto
		ret = (int *) solicitar_solicitar_bytes(socket_umv, pcb.seg_stack, off_cursor_stack_anterior, 4, pcb.id, 'C', logger);
		if(*ret == -1){
			log_error(logger, "[CPU] Segmentation fault. La UMV dice que no se pudo leer. (base=%d, offset=%d, tamaño=%d).", pcb.seg_stack, off_cursor_stack_anterior, 4);
			return -1;
		} else if(*ret == -2){
			log_error(logger, "[CPU] Se quiso leer por fuera de los límites del segmento con base %d, offset %d y tamaño de lectura %d", pcb.seg_stack, off_cursor_stack_anterior, 4);
			return -1;
		} else {
			//log_info(logger, "[PRIMITIVA_aux] _popSinRetorno, cursor al comienzo del contexto apunta al offset: %d", pcb.cursor_stack);
			pcb.cursor_stack = *ret;
		}

		//Calculo el tamaño del diccionario de variables (la cantidad de bytes debería ser múltiplo de 5)
		tamanio_ctxt_actual_bytes = fin_contexto_anterior - pcb.cursor_stack;
		pcb.size_ctxt_actual = (tamanio_ctxt_actual_bytes / 5);
		
		if(tamanio_ctxt_actual_bytes % 5)
			log_warning(logger, "[CPU] El tamaño del contexto actual en bytes es de %d, y no es múltiplo de 5", tamanio_ctxt_actual_bytes);

		generarDiccionarioVariables();

	pthread_mutex_unlock(&operacion);	

	return 0;
}

void _wait_bloqueante(t_nombre_semaforo identificador_semaforo)
{
	char nombre_syscall[] = "wait\0";
	int nombre_syscall_len = strlen(nombre_syscall);
	int identificador_semaforo_len = strlen(identificador_semaforo);
	char *comando = calloc(nombre_syscall_len + 1 + 1 + identificador_semaforo_len + 1 + 1 + 48, 1);
	int offset = 0;
	int bEnv = 0;
	int aux = 0;

	aux = strlen(identificador_semaforo);
	if(identificador_semaforo[aux - 1] == '\n')
		identificador_semaforo[aux - 1] = '\0';

	memcpy(comando + offset, nombre_syscall, nombre_syscall_len);
	offset += nombre_syscall_len;

	memcpy(comando + offset, ",", 1);
	offset += 1;

	memcpy(comando + offset, identificador_semaforo, identificador_semaforo_len);
	offset += identificador_semaforo_len;

	memcpy(comando + offset, "|", 1);

	mi_syscall = comando;
	//log_info(logger, "[PRIMITIVA_wait] wait bloqueante tira el comando: %s", mi_syscall);

	salimosPorSyscallBloqueante = 1;
	//debo_actualizar_manualmente_p_counter = 0;

	return;
}
