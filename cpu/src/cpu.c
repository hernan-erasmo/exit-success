#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

#include <parser/metadata_program.h>
#include <parser/parser.h>

#include "cpu.h"

t_pcb pcb;

int main(int argc, char *argv[])
{
	int errorArgumentos = 0;
	int errorLogger = 0;
	int errorConfig = 0;
	int errorConexion = 0;

	//Variables para el logger
	t_log *logger = NULL;

	//Variables para la carga de la configuración
	t_config *config = NULL;

	//Variables para las conexiones
	int socket_pcp = -1;
	int puerto_pcp = -1;
	char *ip_pcp = NULL;
	int socket_umv = -1;
	int puerto_umv = -1;
	char *ip_umv = NULL;
	struct sockaddr_in socketInfo;
	int status = 0;

	//Variables para el funcionamiento del parser
	AnSISOP_funciones *funciones_comunes = malloc(sizeof(AnSISOP_funciones));
	AnSISOP_kernel *funciones_kernel = malloc(sizeof(AnSISOP_kernel));

	errorArgumentos = checkArgs(argc);
	errorLogger = crearLogger(&logger);
	errorConfig = cargarConfig(&config, argv[1]);

	if(errorArgumentos || errorLogger || errorConfig) {
		goto liberarRecursos;
		return EXIT_FAILURE;
	}

	ip_pcp = config_get_string_value(config,"IP_PCP");
	puerto_pcp = config_get_int_value(config,"PUERTO_PCP");
	ip_umv = config_get_string_value(config,"IP_UMV");
	puerto_umv = config_get_int_value(config, "PUERTO_UMV");

	log_info(logger, "Hola, soy la CPU %d", getpid());

	//Me conecto al hilo PCP del Kernel
	errorConexion = crear_conexion_saliente(&socket_pcp, &socketInfo, ip_pcp, puerto_pcp, logger, "CPU");
	if (errorConexion) {
		log_error(logger, "[CPU] Error al conectar con el Kernel (Hilo PCP).");
		goto liberarRecursos;
		return EXIT_FAILURE;
	}


	/*
	**	Acá habría que realizar la conexion con la UMV
	*/

	int q;
	char *proxima_instruccion = NULL;
	while(1){		//Asumo que todo lo que venga del PCP va a ser un PCB, asi que lo proceso como tal.
		status = recvPcb(&pcb, socket_pcp);
		
		if(status){
			//Comenzá a procesar el pcb
			log_info(logger,"[PCP] Me acaba de llegar un PCB correspondiente al programa con ID: %d.", pcb.id);
			
			for(q = pcb.quantum; q > 0; q--){
				
				analizadorLinea(proxima_instruccion, funciones_comunes, funciones_kernel);
			}

		} else {	//Falló la recepción del pcb.
			log_error(logger, "[PCP] Hubo una falla en la recepción del PCB.");
		}
	}
		
	goto liberarRecursos;
	return EXIT_SUCCESS;

liberarRecursos:
	if(logger)
		log_destroy(logger);

	if(config)
		config_destroy(config);

	if(socket_pcp != -1)
		close(socket_pcp);

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
