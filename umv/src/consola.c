#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "consola.h"

void *consola(void *consola_init)
{
	t_consola_init *c_init = (t_consola_init *) consola_init;
	uint32_t tamanio_mem_ppal = c_init->tamanio_mem_ppal;
	void *mem_ppal = c_init->mem_ppal;
	t_list *listaSegmentos = c_init->listaSegmentos;
	char *algoritmo_comp = c_init->algoritmo_comp;
	pid_t pid = c_init->umv_pid;	

	char *comando = NULL;
	int noSalir = 1;

	/*
	** Bucle principal de la consola
	*/
	while(noSalir){
		printf("UMV> Ingrese un comando: ");
		comando = getLinea();

		if(strcmp(comando,"q") == 0){
			noSalir = 0;
			comando_matar_umv(pid, c_init);
		
		} else if(strcmp(comando,"info") == 0){
			comando_info_memoria(mem_ppal, listaSegmentos, tamanio_mem_ppal, algoritmo_comp);
		
		} else if(strcmp(comando,"cambiar-algoritmo") == 0){
			comando_cambiar_algoritmo(&algoritmo_comp);
		
		} else if(strcmp(comando,"cambiar-proceso-activo") == 0){
			comando_cambiar_proceso_activo();
		
		} else if(strcmp(comando,"compactar") == 0){
			comando_compactar(listaSegmentos, mem_ppal, tamanio_mem_ppal);
		
		} else if(strcmp(comando,"crear-segmento") == 0){
			comando_crear_segmento(listaSegmentos, mem_ppal, tamanio_mem_ppal, algoritmo_comp);
		
		} else if(strcmp(comando,"destruir-segmentos") == 0){
			comando_destruir_segmentos(listaSegmentos);
		
		} else if(strcmp(comando,"dump-all") == 0){
			comando_dump_all(mem_ppal, tamanio_mem_ppal);

		} else if(strcmp(comando,"dump-segmentos") == 0){
			comando_dump_segmentos(listaSegmentos);
		
		} else if (strcmp(comando,"enviar-bytes") == 0) {
			printf("UMV> Falta implementar el comando \"enviar-bytes\".\n");
		
		} else if (strcmp(comando,"h") == 0) {
			printf("UMV> Falta implementar la ayuda. Perdón :(\n");
		
		} else if (strlen(comando) == 0) {
			//para cuando se presiona 'enter' sin ningún caracter, repite el prompt indefinidamente.
		
		} else {
			printf("UMV> No entiendo \"%s\". Ingrese 'h' para ver la lista de comandos disponibles.\n", comando);
		}

		free(comando);
	}

	printf("UMV> Adiós!\n");
	pthread_exit(NULL);
}

void comando_cambiar_algoritmo(char **algoritmo)
{
	uint32_t opcion = 0;
	
	printf("\t1) FIRST_FIT\n\t2) WORST_FIT\n");
	printf("\tElija una opción: ");
	scanf("%d", &opcion);

	if(opcion == 1) {
		*algoritmo = "FIRST_FIT";
	} else if (opcion == 2) {
		*algoritmo = "WORST_FIT";
	}
		
	//Si no hago este fgetc() entonces se repite dos veces el prompt de UMV> cuando retorna al bucle principal.
	fgetc(stdin);

	return;
}

void comando_cambiar_proceso_activo()
{
	uint32_t id_nuevo = 0;

	printf("\tIngrese el ID del nuevo proceso activo: ");
	scanf("%d", &id_nuevo);

	cambiar_proceso_activo(id_nuevo);
	
	//Si no hago este fgetc() entonces se repite dos veces el prompt de UMV> cuando retorna al bucle principal.
	fgetc(stdin);

	return;
}

void comando_compactar(t_list *listaSegmentos, void *mem_ppal, uint32_t tamanio_mem_ppal)
{
	uint32_t espacioLiberado = 0;

	espacioLiberado = compactar(listaSegmentos, mem_ppal, tamanio_mem_ppal);

	printf("\tSe pudieron liberar %d bytes.\n", espacioLiberado);
}

void comando_crear_segmento(t_list *listaSegmentos, void *mem_ppal, uint32_t tamanio_mem_ppal, char *algoritmo)
{
	uint32_t id_prog = 0;
	uint32_t tamanio_segmento = 0;

	printf("\tID de programa (uint32_t): ");
	scanf("%d", &id_prog);
	printf("\tTamaño (uint32_t): ");
	scanf("%d", &tamanio_segmento);

	if(!tamanio_segmento){
		printf("UMV> No se puede crear un segmento de tamaño 0.\n");
		fgetc(stdin);
		return;
	}

	t_list *espacios_libres = buscarEspaciosLibres(listaSegmentos, mem_ppal, tamanio_mem_ppal);
	uint32_t seg = crearSegmento(id_prog,tamanio_segmento,espacios_libres,listaSegmentos,algoritmo);
	
	if(seg != 0){
		printf("UMV> Se creó el segmento (dir. virtual = %d).\n", seg);
	} else {
		printf("UMV> No se pudo crear el segmento en el estado actual de la memoria. Voy a compactar.\n");
		comando_compactar(listaSegmentos, mem_ppal, tamanio_mem_ppal);
		
		if (!list_is_empty(espacios_libres)) {
			list_clean_and_destroy_elements(espacios_libres, eliminarEspacioLibre);
		}

		list_destroy(espacios_libres);

		espacios_libres = buscarEspaciosLibres(listaSegmentos, mem_ppal, tamanio_mem_ppal);
		seg = crearSegmento(id_prog,tamanio_segmento,espacios_libres,listaSegmentos,algoritmo);

		if(seg != 0){
			printf("UMV> Se creó el segmento (dir. virtual = %d).\n", seg);
		} else {
			printf("UMV> No se pudo crear el segmento porque no hay espacio en memoria.\n");
		}	
	}

	//Si no hago este fgetc() entonces se repite dos veces el prompt de UMV> cuando retorna al bucle principal.
	fgetc(stdin);

	if (!list_is_empty(espacios_libres)) {
		list_clean_and_destroy_elements(espacios_libres, eliminarEspacioLibre);
	}

	list_destroy(espacios_libres);

	return;
}

void comando_destruir_segmentos(t_list *listaSegmentos)
{
	uint32_t progId = 0;
	
	printf("\tIngrese el ID del programa: ");
	scanf("%d", &progId);
	
	//Si no hago este fgetc() entonces se repite dos veces el prompt de UMV> cuando retorna al bucle principal.
	fgetc(stdin);
	
	destruirSegmentos(listaSegmentos, progId);
	
	return;
}

void comando_dump_all(void *mem_ppal, uint32_t tamanio_mem_ppal)
{
	void *posicion = mem_ppal;
	void *offset = mem_ppal - 1;
	void *fin_memoria = mem_ppal + tamanio_mem_ppal - 1;
	uint32_t salto = 16;
	int cant = 0;
	int bytesAImprimir = 0;

	printf("\tIngrese offset y cantidad de bytes (separados por un espacio): ");
	scanf("%p %d", &offset, &cant);
	
	if((offset < mem_ppal) || (offset > fin_memoria)){
		printf("\tLa base no cae dentro de la memoria reservada por la UMV.\n");

		//Si no hago este fgetc() entonces se repite dos veces el prompt de UMV> cuando retorna al bucle principal.
		fgetc(stdin);
		return;		
	}

	if(cant <= 0){
		printf("\tLa cantidad de bytes debe ser mayor que cero.\n");

		//Si no hago este fgetc() entonces se repite dos veces el prompt de UMV> cuando retorna al bucle principal.
		fgetc(stdin);
		return;		
	}

	posicion = offset;

	while(cant > 0){
		bytesAImprimir = (cant < salto) ? cant : salto;
		cant -= salto;

		printf("\t%p\t", posicion);
		imprimirArrayDeBytes(posicion, bytesAImprimir);
		printf("\t");
		imprimirArrayDeChars(posicion, bytesAImprimir);
		printf("\n");

		posicion += salto;
		if(posicion > fin_memoria){
			printf("\n\t\t ######## FIN DE LA MEMORIA RESERVADA ########\n");
			break;
		}
	}

	//Si no hago este fgetc() entonces se repite dos veces el prompt de UMV> cuando retorna al bucle principal.
	fgetc(stdin);
	
	return;
}

void comando_dump_segmentos(t_list *listaSegmentos)
{
	int id_prog, opcion = 0;
	t_list *segmentos = NULL;

	printf("\t1) Por ID de programa.\n");
	printf("\t2) Todos.\n");
	printf("\tOpción: ");
	scanf("%d", &opcion);
	
	if(opcion == 1){
		printf("\t\tIngrese ID de programa: ");
		scanf("%d", &id_prog);
		segmentos = buscarSegmentosConId(listaSegmentos, id_prog);

		if(segmentos != NULL){
			dump_segmentos(segmentos);
			list_destroy(segmentos);
		} else {
			printf("UMV> No hay segmentos con id de programa: %d\n", id_prog);
		}
	
	} else if (opcion == 2) {
		dump_segmentos(listaSegmentos);
	}

	//Si no hago este fgetc() entonces se repite dos veces el prompt de UMV> cuando retorna al bucle principal.
	fgetc(stdin);

	return;
}

void comando_info_memoria(void *mem_ppal, t_list *listaSegmentos, uint32_t tamanio_mem_ppal, char *algoritmo)
{
	t_list *esp_libre = buscarEspaciosLibres(listaSegmentos, mem_ppal, tamanio_mem_ppal);
	printf("\tEl algoritmo de compactación actual es: %s\n", algoritmo);
	printf("\tEl proceso activo tiene ID: %d\n", get_proceso_activo());

	if(list_is_empty(esp_libre)){
		printf("\tNo hay espacio libre en memoria.\n");
	} else {
		list_iterate(esp_libre, mostrarInfoEspacioLibre);
	}

	list_destroy_and_destroy_elements(esp_libre, eliminarEspacioLibre);
}

char *getLinea(void)
{
	char *line = NULL;
	char *linep = NULL;
	size_t lenmax = 40;
	size_t len = 0;
	int c = 0;

	line = calloc(40,1);
	linep = line;
	len = lenmax;

	if(line == NULL)
		return NULL;

	for(;;) {
		c = fgetc(stdin);
		if(c == EOF)
			break;

		if(--len == 0) {
			len = lenmax;
			char *linen = realloc(linep, lenmax *= 2);

			if(linen == NULL){
				free(linep);
				return NULL;
			}

			line = linen + (line - linep);
			linep = linen;
		}

		if((*line++ = c) == '\n') {
			len = strlen(line) - 1;
			
			if(line[len] == '\n')
				line[len] = '\0';
			
			break;
		}
	}
	return linep;
}

t_list *buscarSegmentosConId(t_list *listaSegmentos, uint32_t id)
{
	t_list *seg = list_create();
	t_segmento *s = NULL;
	int i, size = list_size(listaSegmentos);

	for(i = 0; i < size; i++){
		s = (t_segmento *) list_get(listaSegmentos, i);

		if(s->prog_id == id)
			list_add(seg, s);
	
	}

	if(list_size(seg) == 0){
		list_destroy(seg);
		return NULL;
	}
		
	return seg;
}

void imprimirArrayDeBytes(void *offset, uint32_t cantidad)
{
	int i;
	char *c;

	for(i = 0; i < cantidad; i++){
		c = (char *) (offset + i);
		printf("%02X ", *c);
	}

	return;
}

void imprimirArrayDeChars(void *offset, uint32_t cantidad)
{
	int i;
	char *c;

	for(i = 0; i < cantidad; i++){
		c = (char *) (offset + i);

		if(*c > 32)
			printf("%c", *c);
		else
			printf(".");
	}

	return;
}

void comando_matar_umv(pid_t pid, t_consola_init *c_init)
{
//	list_destroy_and_destroy_elements(c_init->listaSegmentos, eliminarSegmento);
//	free(c_init->mem_ppal);
//	free(c_init->algoritmo_comp);

	struct addrinfo hints;
	struct addrinfo *serverInfo;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	char prto[6];
	sprintf(prto, "%d", c_init->puerto);
	getaddrinfo("127.0.0.1", prto, &hints, &serverInfo);

	int sock = -1;

	sock = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
		
	connect(sock, serverInfo->ai_addr, serverInfo->ai_addrlen);
	freeaddrinfo(serverInfo);
	*(c_init->noTerminar) = 0;
	close(sock);

//	kill(pid, SIGINT);
}
