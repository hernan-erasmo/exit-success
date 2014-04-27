#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>

#include <commons/log.h>
#include <commons/config.h>

#define PACKAGESIZE 1024

int checkArgs(int args);
int crearLogger(t_log **logger);
int cargarConfig(t_config **config, char *path);

int main(int argc, char *argv[])
{
	int errorArgumentos = 0;
	int errorLogger = 0;
	int errorConfig = 0;

	//Variables de la conexion
	struct addrinfo hints;
	struct addrinfo *serverInfo = NULL;
	int listenningSocket = -1;
	int socketCliente = -1;
	struct sockaddr_in addr; // Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	socklen_t addrlen = sizeof(addr);
	char package[PACKAGESIZE];
	int status = 1;		// Estructura que manjea el status de los recieve.

	//Variables para el logger
	t_log *logger = NULL;

	//Variables para la carga de la configuración
	t_config *config = NULL;

	errorArgumentos = checkArgs(argc);
	errorLogger = crearLogger(&logger);
	errorConfig = cargarConfig(&config, argv[1]);

	if(errorArgumentos || errorLogger || errorConfig) {
		goto liberarRecursos;
		return EXIT_FAILURE;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// No importa si uso IPv4 o IPv6
	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	// Notar que le pasamos NULL como IP, ya que le indicamos que use localhost en AI_PASSIVE
	if (getaddrinfo(NULL, config_get_string_value(config, "PUERTO_PROG"), &hints, &serverInfo) != 0) {
		log_error(logger,"No se pudo crear la estructura addrinfo. Motivo: %s", strerror(errno));
		goto liberarRecursos;
		return EXIT_FAILURE;
	}
	
	if ((listenningSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol)) < 0) {
		log_error(logger, "Error al crear socket. Motivo: %s", strerror(errno));
	}
	log_info(logger, "Se creó el socket a la escucha del puerto: %s", config_get_string_value(config, "PUERTO_PROG"));

	if(bind(listenningSocket,serverInfo->ai_addr, serverInfo->ai_addrlen)) {
		log_error(logger, "No se pudo bindear el socket a la dirección. Motivo: %s", strerror(errno));
		goto liberarRecursos;
		return EXIT_FAILURE;
	}

	listen(listenningSocket, 10);
	
	log_info(logger, "Esperando conexiones...");
	socketCliente = accept(listenningSocket, (struct sockaddr *) &addr, &addrlen);
	log_info(logger, "Recibí una conexión!");

	while (status != 0){
		memset(package, 0, PACKAGESIZE);
		status = recv(socketCliente, (void*) package, PACKAGESIZE, 0);
		if (status > 0) 
			printf("%s", package);
		if (status < 0) {
			log_error(logger, "Error en la transmisión. Motivo: %s", strerror(errno));
			goto liberarRecursos;
			return EXIT_FAILURE;
		}
			
	}

	goto liberarRecursos;
	return EXIT_SUCCESS;

liberarRecursos:
	if(logger)
		log_destroy(logger);

	if(config)
		config_destroy(config);		

	if(listenningSocket != -1) 
		close(listenningSocket);

	if(socketCliente != -1)
		close(socketCliente);

	if(serverInfo != NULL)
		freeaddrinfo(serverInfo);
}

int crearLogger(t_log **logger)
{
	char *nombreArchivoLog = "kernel_log";

	if ((*logger = log_create(nombreArchivoLog,"Kernel",true,LOG_LEVEL_DEBUG)) == NULL) {
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
		printf("El kernel debe recibir como parámetro únicamente la ruta al archivo de configuración.\n");
		return 1;
	}
	
	return 0;	
}