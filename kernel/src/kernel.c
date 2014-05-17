#include <errno.h>

#include <commons/log.h>
#include <commons/config.h>

#include "plp.h"

#define PACKAGESIZE 1024

int checkArgs(int args);
int crearLogger(t_log **logger);
int cargarConfig(t_config **config, char *path);

int main(int argc, char *argv[])
{
	int errorArgumentos = 0;
	int errorLogger = 0;
	int errorConfig = 0;

	//Variables de threads
	pthread_t threadPlp;
	t_datos_plp *d_plp;
	void *retorno = NULL;

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

	d_plp = malloc(sizeof(t_datos_plp));
	d_plp->puerto = config_get_string_value(config, "PUERTO_PROG");
	d_plp->logger = logger;

	log_info(logger, "[PLP] Inicializando el hilo PLP");
	if(pthread_create(&threadPlp, NULL, plp, (void *) d_plp)) {
		log_error(logger, "Error al crear el thread del PLP. Motivo: %s", strerror(errno));
		goto liberarRecursos;
		return EXIT_FAILURE;
	}

	pthread_join(threadPlp, NULL);
	log_info(logger, "El thread PLP finalizó y retornó status: (falta implementar!)");

	goto liberarRecursos;
	return EXIT_SUCCESS;

liberarRecursos:
	if(logger)
		log_destroy(logger);

	if(config)
		config_destroy(config);

	free(d_plp);
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
