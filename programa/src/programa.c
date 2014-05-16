#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <commons/log.h>
#include <commons/config.h>

#include "programa.h"

int main(int argc, char *argv[])
{
	int errorArgumentos = 0;
	int errorLogger = 0;
	int errorConfig = 0;
	int errorConexion = 0;
	int errorEnvio = 0;

	//Variables para el script
	FILE *script = NULL;
	
	//Variables para el logger
	t_log *logger = NULL;
	
	//Variables para el socket
	int unSocket = -1;
	struct sockaddr_in socketInfo;

	//Variables para la carga de la configuración
	t_config *config = NULL;

	errorArgumentos = checkArgs(argc);
	errorLogger = crearLogger(&logger);
	errorConfig = cargarConfig(&config);

	if(errorArgumentos || errorLogger || errorConfig) {
		goto liberarRecursos;
		return EXIT_FAILURE;
	}

	if ((script = fopen(argv[1],"r")) != NULL) {
		//Pudimos abrir el archivo correctamente
		//Entonces creamos la conexión
		log_info(logger, "Conectando a %s:%d ...", config_get_string_value(config, "IP"), config_get_int_value(config, "Puerto"));

		errorConexion = crearConexion(&unSocket, &socketInfo, config, logger);
		if (errorConexion) {
			log_error(logger, "Error al conectar con el Kernel.");
			goto liberarRecursos;
			return EXIT_FAILURE;
		}

		log_info(logger, "Conexión establecida.");
		log_info(logger, "Comenzando a enviar el script AnSISOP.");

		errorEnvio = enviarDatos(script, unSocket, logger);
		if (errorEnvio) {
			goto liberarRecursos;
			return EXIT_FAILURE;
		} else {
			finalizarEnvio(&unSocket);
			log_info(logger, "Transmisión finalizada.");	
		}			
	
	} else {
		log_error(logger,"No se pudo abrir el script AnSISOP. Motivo: %s", strerror(errno));
		goto liberarRecursos;
		return EXIT_FAILURE;
	}

	goto liberarRecursos;
	return EXIT_SUCCESS;

liberarRecursos:
	if(unSocket != -1) 
		close(unSocket);

	if(script)
		fclose(script);
	
	if(logger)
		log_destroy(logger);

	if(config)
		config_destroy(config);
}

int crearConexion(int *unSocket, struct sockaddr_in *socketInfo, t_config *config, t_log *logger)
{
	if ((*unSocket = crearSocket(socketInfo, config)) < 0) {
		log_error(logger, "Error al crear socket. Motivo: %s", strerror(errno));
		return 1;
	}

	if (connect(*unSocket, (struct sockaddr *) socketInfo, sizeof(*socketInfo)) != 0) {
		log_error(logger, "Error al conectar socket. Motivo: %s", strerror(errno));
		return 1;
	}

	return 0;
}

int crearSocket(struct sockaddr_in *socketInfo, t_config *config)
{
	int sock = -1;
		
	if((sock = socket(AF_INET,SOCK_STREAM,0)) >= 0) {
		socketInfo->sin_family = AF_INET;
		socketInfo->sin_addr.s_addr = inet_addr(config_get_string_value(config, "IP"));
		socketInfo->sin_port = htons(config_get_int_value(config, "Puerto"));
	}

	return sock;		
}

int enviarDatos(FILE *script, int unSocket, t_log *logger)
{
	uint32_t bEnv = 0;
	char *contenidoScript = NULL;
	char *paqueteSerializado = NULL;
	uint32_t sizeMensaje = 0;

	t_paquete_programa paquete;
		sizeMensaje = getFileSize(script);
		contenidoScript = calloc(sizeMensaje + 1, sizeof(char)); //sizeMensaje + 1, el contenido del script mas el '\0'

	cargarScript(script, &contenidoScript);
	inicializarPaquete(&paquete, sizeMensaje, &contenidoScript);
	paqueteSerializado = serializarPaquete(&paquete, logger);

	if((bEnv += send(unSocket, paqueteSerializado, paquete.tamanio_total, 0)) < 0){
		log_error(logger, "Error en la transmisión del script (Handshake ID). Motivo: %s", strerror(errno));
		return 1;
	}

	log_info(logger, "Enviados %d bytes.", bEnv);

	free(contenidoScript);
	free(paqueteSerializado);

	return 0;
}

void finalizarEnvio(int *unSocket) {
	shutdown(*unSocket, SHUT_WR);
}

int checkArgs(int args)
{
	if (args != 2) {
		printf("Debe indicar como parámetro la ruta a un archivo.\n");
		return 1;
	}
	
	return 0;	
}

int crearLogger(t_log **logger)
{
	char *nombreArchivoLog = "programa_log";

	if ((*logger = log_create(nombreArchivoLog,"programa",true,LOG_LEVEL_DEBUG)) == NULL) {
		printf("No se pudo inicializar un logger.\n");
		return 1;
	}

	return 0;
}

int cargarConfig(t_config **config)
{
	char *path = getenv("ANSISOP_CONFIG");

	if(path == NULL) {
		printf("No se pudo encontrar la variable de entorno ANSISOP_CONFIG.\n");
		return 1;
	}

	*config = config_create(path);
	return 0;
}

int getFileSize(FILE *script){
	int sz = 0;

	fseek(script, 0L, SEEK_END);
	sz = ftell(script);
	fseek(script, 0L, SEEK_SET);

	return sz;
}

void inicializarPaquete(t_paquete_programa *paquete, uint32_t sizeMensaje, char **contenidoScript)
{
	paquete->id = 'P';
	paquete->sizeMensaje = sizeMensaje;
	paquete->mensaje = *contenidoScript;
	paquete->tamanio_total = 1 + sizeof(paquete->sizeMensaje) + sizeMensaje + sizeof(paquete->tamanio_total);

	return;
}

char *serializarPaquete(t_paquete_programa *paquete, t_log *logger)
{
	char *serializedPackage = calloc(1 + sizeof(paquete->sizeMensaje) + paquete->sizeMensaje + sizeof(paquete->tamanio_total), sizeof(char)); //El tamaño del char id, el tamaño de la variable sizeMensaje, el tamaño del script y el tamaño de la variable tamaño_total
	uint32_t offset = 0;
	uint32_t sizeToSend;

	sizeToSend = sizeof(paquete->id);
	log_info(logger, "sizeof(paquete->id) = %d", sizeof(paquete->id));
	memcpy(serializedPackage + offset, &(paquete->id), sizeToSend);
	offset += sizeToSend;

	sizeToSend = sizeof(paquete->sizeMensaje);
	log_info(logger, "sizeof(paquete->sizeMensaje) = %d", sizeof(paquete->sizeMensaje));
	memcpy(serializedPackage + offset, &(paquete->sizeMensaje), sizeToSend);
	offset += sizeToSend;

	sizeToSend = paquete->sizeMensaje;
	log_info(logger, "paquete->size = %d", paquete->sizeMensaje);
	memcpy(serializedPackage + offset, paquete->mensaje, sizeToSend);

	return serializedPackage;
}

char *cargarScript(FILE *script, char **contenidoScript)
{
	char *aux = NULL;
	char c = '\0';
	
	aux = *contenidoScript;
	while((c = fgetc(script)) != EOF){
		memcpy(aux, &c, 1);
		aux++;
	}
	memset(aux, '\0', 1);		// ¿Es realmente necesario este memset? Para preguntar. Pero anda igual.

}