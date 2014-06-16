#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

#include <parser/metadata_program.h>
#include <parser/parser.h>

#include "cpu.h"

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
	log_info(logger, "Puedo encontrar la UMV en %s:%s", config_get_string_value(config, "IP_UMV"), config_get_string_value(config, "PUERTO_UMV"));
	log_info(logger, "Puedo encontrar al Kernel en %s:%s", config_get_string_value(config, "IP_KERNEL"), config_get_string_value(config, "PUERTO_KERNEL"));

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
	char nombreArchivoLog[15];
	char *nombre = "cpu_log_";
	char str_pid[7];
	pid_t pid = getpid();

	sprintf(str_pid, "%d", pid);
	strcpy(nombreArchivoLog, nombre);
	strcpy(nombreArchivoLog + strlen(nombre), str_pid);

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

char *codificar_enviar_bytes(uint32_t base, uint32_t offset, int tamanio, void *buffer, uint32_t *tamanio_de_la_orden_completa)
{
	int offset_codificacion = 0;

	char str_base[10];
	sprintf(str_base, "%d", base);
	char str_offset[10];
	sprintf(str_offset, "%d", offset);
	char str_tamanio[10];
	sprintf(str_tamanio, "%d", tamanio);
	char *comando = "enviar_bytes";

	int base_len = strlen(str_base);
	int offset_len = strlen(str_offset);
	int tamanio_len = strlen(str_tamanio);
	int comando_len = strlen(comando);
	int buffer_len = tamanio;

	char *orden_completa = calloc(comando_len + 1 + 1 + base_len + 1 + offset_len + 1 + tamanio_len + 1 + buffer_len, 1);
	memcpy(orden_completa + offset_codificacion, comando, comando_len);
	offset_codificacion += comando_len;
	memcpy(orden_completa + offset_codificacion, ",", 1);
	offset_codificacion += 1;

	memcpy(orden_completa + offset_codificacion, str_base, base_len);
	offset_codificacion += base_len;
	memcpy(orden_completa + offset_codificacion, ",", 1);
	offset_codificacion += 1;

	memcpy(orden_completa + offset_codificacion, str_offset, offset_len);
	offset_codificacion += offset_len;
	memcpy(orden_completa + offset_codificacion, ",", 1);
	offset_codificacion += 1;

	memcpy(orden_completa + offset_codificacion, str_tamanio, tamanio_len);
	offset_codificacion += tamanio_len;
	memcpy(orden_completa + offset_codificacion, ",", 1);
	offset_codificacion += 1;

	//hasta acá tengo el control de lo que escribo en orden_completa, después ya no sé si hay comas, nulls, etc. 
	//y sólo me guío por el tamaño del buffer.
	*tamanio_de_la_orden_completa = strlen(orden_completa) + tamanio;

	memcpy(orden_completa + offset_codificacion, buffer, buffer_len);
	offset_codificacion += buffer_len;
	
	return orden_completa;
}
