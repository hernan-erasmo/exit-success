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
	int statusRecepcion = 0;

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

	int puerto = config_get_int_value(config, "Puerto");
	char *ip_kernel = config_get_string_value(config, "IP");

	if ((script = fopen(argv[1],"r")) != NULL) {
		//Pudimos abrir el archivo correctamente
		//Entonces creamos la conexión
		log_info(logger, "Conectando a %s:%d ...", ip_kernel, puerto);

		errorConexion = crear_conexion_saliente(&unSocket, &socketInfo, ip_kernel, puerto, logger, "PROGRAMA");
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

		//acá se abre la guarda para el modo debug
		t_paquete_programa paq;
		while(1){
			statusRecepcion = recvAll(&paq, unSocket);
			if(statusRecepcion == 0){
				log_error(logger, "Hubo un error al recibir un mensaje del Kernel.");
			} else {
				if(ejecutarMensajeKernel(paq.mensaje)){		//Si es 0 era porque era un imprimir/imprimirTexto. Si es 1, hay que terminar.
					log_info(logger, "Finalizó la ejecución del programa.");
					free(paq.mensaje);
					goto liberarRecursos;
					break;
				}
			}
		}
		//acá se cierra la guarda para el modo debug

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
	paqueteSerializado = serializar_paquete(&paquete, logger);

	bEnv = paquete.tamanio_total;
	if(sendAll(unSocket, paqueteSerializado, &bEnv)){
		log_error(logger, "Error en la transmisión del script. Motivo: %s", strerror(errno));
		return 1;
	}

	log_info(logger, "Enviados %d bytes.", bEnv);

	free(contenidoScript);
	free(paqueteSerializado);

	return 0;
}

void finalizarEnvio(int *unSocket) {
	//Está comentado, porque hace que no se bloquee el programa al llegar a recvAll()
	//shutdown(*unSocket, SHUT_WR);
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
	//paquete->tamanio_total = 1 + sizeof(paquete->sizeMensaje) + sizeMensaje + sizeof(paquete->tamanio_total);

	return;
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

int ejecutarMensajeKernel(char *mensaje)
{
	char *orden = strtok(mensaje, "_");
	char *msj = strtok(NULL, "_");

	printf("El kernel dice: %s\n", msj);
	
	if(strcmp("FINALIZAR", orden) == 0){
		return 1;
	} else if (strcmp("INFORMAR", orden) == 0) {
		return 0;
	}

	printf("No entiendo la orden del Kernel (No es ni FINALIZAR, ni INFORMAR)\n");
	return 0;
}
