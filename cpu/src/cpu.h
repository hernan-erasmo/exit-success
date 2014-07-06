#include <pthread.h>
#include <netdb.h>

#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/dictionary.h>

#include <parser/metadata_program.h>
#include <parser/parser.h>

#include "../../utils/comunicacion.h"
#include "../../utils/interfaz_umv.h"

#ifndef CPU_H
#define CPU_H

t_pcb pcb;
t_dictionary *diccionario_variables;
int socket_umv;
t_log *logger;

uint32_t salimosPorSyscall;
char *mi_syscall;

uint32_t salimosPorError;
uint32_t salimosPorFin;

int checkArgs(int args);
int crearLogger(t_log **logger);
int cargarConfig(t_config **config, char *path);
void generarDiccionarioVariables();
char *obtener_proxima_instruccion(int socket_umv, t_log *logger);
int enviarPcbProcesado(int socket_pcp, char evento, t_log *logger);

#endif /* CPU_H */
