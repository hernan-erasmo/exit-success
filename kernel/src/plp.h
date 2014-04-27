#include <pthread.h>

#include <commons/log.h>
#include <parser/metadata_program.h>

typedef struct datos_plp {
	char *puerto;
	t_log *logger;
} t_datos_plp;

typedef struct con_prog {
	char *codigo;
	t_medatada_program medatada;
} t_con_prog;

void *plp(void *puerto_prog);
void *atenderConexionPrograma(void *con_prog);
