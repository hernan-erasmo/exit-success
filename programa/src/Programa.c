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
	char buffer[BUFF_SIZE];
	int bytesEnviados = 0;
	
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

		if ((unSocket = socket(AF_INET,SOCK_STREAM,0)) < 0) {
			log_error(logger, "Error al crear socket. Motivo: %s", strerror(errno));
			fclose(script);
			log_destroy(logger);
			return EXIT_FAILURE;
		}

		socketInfo.sin_family = AF_INET;
		socketInfo.sin_addr.s_addr = inet_addr(DIRECCION);
		socketInfo.sin_port = htons(PUERTO);

		//Conectamos el socket con la dirección de 'socketInfo'
		if (connect(unSocket, (struct sockaddr *) &socketInfo, sizeof(socketInfo)) != 0) {
			log_error(logger, "Error al conectar el socket. Motivo: %s", strerror(errno));
			fclose(script);
			close(unSocket);
			log_destroy(logger);
			return EXIT_FAILURE;
		}

		log_info(logger, "Conexión establecida.");
		log_info(logger, "Comenzando a enviar el script AnSISOP.");

		while (fread(buffer, sizeof(char), BUFF_SIZE, script) > 0) {
			if ((bytesEnviados += send(unSocket, buffer, strlen(buffer), 0)) < 0) {
				log_error(logger, "Error en la transmisión del script. Motivo: %s", strerror(errno));
			}
			memset(buffer, '\0', BUFF_SIZE);
		}

		shutdown(unSocket, SHUT_WR);
		log_info(logger, "Transmisión finalizada. Enviados %d bytes.", bytesEnviados);
		
		close(unSocket);
		fclose(script);
	
	}
	
	log_info(logger, "El programa finalizó.");
	log_destroy(logger);

	return EXIT_SUCCESS;
}