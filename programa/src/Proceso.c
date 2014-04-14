#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <commons/log.h>
#include <commons/config.h>

int main(int argc, char *argv[])
{
	int c;
	FILE *script;
	
	char *nombreArchivoLog = "programa_log";
	t_log *logger;

	const char *nombreArchivoConfig = getenv("ANSISOP_CONFIG");
	//t_config *configuracion;
	
	if(argc != 2) {
		printf("Debe indicar como parámetro la ruta a un archivo.\n");
		return 1;
	}

	if((logger = log_create(nombreArchivoLog,"Programa",true,LOG_LEVEL_INFO)) == NULL) {
		printf("No se pudo inicializar un logger.\n");
		return 1;
	}

	if(nombreArchivoConfig == NULL) {
		log_info(logger,"No se pudo abrir el archivo de configuración. ANSISOP_CONFIG apunta a: %s\n",getenv("ANSISOP_CONFIG"));
	} else {
		printf("Falta implementar =D\n");
	}
		
	script = fopen(argv[1],"r");

	if(script != NULL) {
		log_info(logger, "Comenzando a procesar el script AnSISOP.");
		
		while((c = getc(script)) != EOF)
			putchar(c);

		fclose(script);
		log_info(logger, "Se terminó de procesar el script AnSISOP.");
	} else {
		log_error(logger,"No se pudo abrir el script AnSISOP. Motivo: %s", strerror(errno));
	}

	if(logger != NULL)
		log_destroy(logger);

	return 0;
}