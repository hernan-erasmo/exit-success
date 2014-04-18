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

#define DIRECCION "127.0.0.1"
#define PUERTO 5000
#define BUFF_SIZE 1024

int crearSocket(struct sockaddr_in *socketInfo);
int enviarDatos(FILE *script, int unSocket);

int main(int argc, char *argv[])
{
	//Variables para el script
	FILE *script;
	
	//Variables para el logger
	char *nombreArchivoLog = "programa_log";
	t_log *logger;

	//Variables para el socket
	int unSocket;
	struct sockaddr_in socketInfo;
	int bytesEnviados;
	
	if (argc != 2) {
		printf("Debe indicar como parámetro la ruta a un archivo.\n");
		return EXIT_FAILURE;
	}

	if ((logger = log_create(nombreArchivoLog,"Programa",true,LOG_LEVEL_DEBUG)) == NULL) {
		printf("No se pudo inicializar un logger.\n");
		return EXIT_FAILURE;
	}

	if ((script = fopen(argv[1],"r")) == NULL) {
		log_error(logger,"No se pudo abrir el script AnSISOP. Motivo: %s", strerror(errno));
	} else {
		//Pudimos abrir el archivo correctamente
		//Entonces creamos la conexión
		log_info(logger, "Conectando a %s:%d ...", DIRECCION, PUERTO);

		if ((unSocket = crearSocket(&socketInfo)) < 0) {
			log_error(logger, "Error al crear socket. Motivo: %s", strerror(errno));
			goto liberarRecursos;
			return EXIT_FAILURE;
		}

		if (connect(unSocket, (struct sockaddr *) &socketInfo, sizeof(socketInfo)) != 0) {
			log_error(logger, "Error al conectar el socket. Motivo: %s", strerror(errno));
			goto liberarRecursos;
			return EXIT_FAILURE;
		}

		log_info(logger, "Conexión establecida.");
		log_info(logger, "Comenzando a enviar el script AnSISOP.");

		if((bytesEnviados = enviarDatos(script, unSocket)) < 0) {
			log_error(logger, "Error en la transmisión del script. Motivo: %s", strerror(errno));
			goto liberarRecursos;
			return EXIT_FAILURE;
		} else {
			log_info(logger, "Transmisión finalizada. Enviados %d bytes.", bytesEnviados);	
		}	
		
		shutdown(unSocket, SHUT_WR);
		goto liberarRecursos;
	}

	return EXIT_SUCCESS;

liberarRecursos:
	if(unSocket > -1) 
		close(unSocket);

	if(script != NULL)
		fclose(script);
	
	log_destroy(logger);
}


int crearSocket(struct sockaddr_in *socketInfo)
{
	int sock = -1;
		
	if((sock = socket(AF_INET,SOCK_STREAM,0)) >= 0) {
		socketInfo->sin_family = AF_INET;
		socketInfo->sin_addr.s_addr = inet_addr(DIRECCION);
		socketInfo->sin_port = htons(PUERTO);
	}

	return sock;		
}

int enviarDatos(FILE *script, int unSocket)
{
	char buffer[BUFF_SIZE];
	int bEnv = 0;

	while(fread(buffer, sizeof(char), BUFF_SIZE, script) > 0) {
		if((bEnv += send(unSocket, buffer, strlen(buffer), 0)) < 0){
			return -1;
		}
		memset(buffer, '\0', BUFF_SIZE);
	}

	return bEnv;
}