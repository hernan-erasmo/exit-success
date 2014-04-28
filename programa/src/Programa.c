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

#define BUFF_SIZE 1024

int checkArgs(int args);
int crearLogger(t_log **logger);
int cargarConfig(t_config **config);
int crearConexion(int *unSocket, struct sockaddr_in *socketInfo, t_config *config, t_log *logger);
int crearSocket(struct sockaddr_in *socketInfo, t_config *config);
int enviarDatos(FILE *script, int unSocket, t_log *logger);
void finalizarEnvio(int *unSocket);

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
	char buffer[BUFF_SIZE];
	int id = 80; //P ascii, para indicar que soy un Programa
	int sz = getFileSize(script);
	int bEnv = 0;

	memset(buffer, '\0', BUFF_SIZE);

	if(send(unSocket, &id, sizeof(int), 0) < 0){
		log_error(logger, "Error en la transmisión del script (Handshake ID). Motivo: %s", strerror(errno));
		return 1;
	}

	if(send(unSocket, &sz, sizeof(int), 0) < 0){
		log_error(logger, "Error en la transmisión del script (Handshake SZ). Motivo: %s", strerror(errno));
		return 1;
	}

	while(fgets(buffer, BUFF_SIZE, script)) {
		if((bEnv += send(unSocket, buffer, strlen(buffer), 0)) < 0){
			log_error(logger, "Error en la transmisión del script (Contenido). Motivo: %s", strerror(errno));
			return 1;
		}
		memset(buffer, '\0', BUFF_SIZE);
	}

	log_info(logger, "Enviados %d bytes.", bEnv);	
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

	if ((*logger = log_create(nombreArchivoLog,"Programa",true,LOG_LEVEL_DEBUG)) == NULL) {
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
