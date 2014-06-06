#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

#include <commons/log.h>
#include <commons/config.h>

#include <parser/metadata_program.h>
#include <parser/parser.h>

int checkArgs(int args);
int crearLogger(t_log **logger);
int cargarConfig(t_config **config, char *path);

void finalizar(void);

int main(int argc, char *argv[])
{
	int errorArgumentos = 0;
	int errorLogger = 0;
	int errorConfig = 0;

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

	log_info(logger, "Hola, soy la CPU %d", getpid());
/*
	char* const asd = "end";

	AnSISOP_funciones *funciones = malloc(sizeof(AnSISOP_funciones));
	funciones->AnSISOP_finalizar = finalizar;

	analizadorLinea(asd, funciones, NULL);

	free(funciones);

	return 0;
*/

	goto liberarRecursos;
	return EXIT_SUCCESS;

liberarRecursos:
	if(logger)
		log_destroy(logger);

	if(config)
		config_destroy(config);

}

int crearLogger(t_log **logger)
{
	char *nombreArchivoLog = "cpu_log";

	if ((*logger = log_create(nombreArchivoLog,"CPU",true,LOG_LEVEL_DEBUG)) == NULL) {
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
		printf("La CPU debe recibir como parámetro únicamente la ruta al archivo de configuración.\n");
		return 1;
	}
	
	return 0;	
}

void finalizar(void)
{
	printf("Estoy adentro de la función finalizar. Adiós para siempre.\n");

	return;
}