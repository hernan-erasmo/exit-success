#include <netdb.h>

#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/dictionary.h>

#include <parser/metadata_program.h>
#include <parser/parser.h>

#include "../../utils/comunicacion.h"

#ifndef CPU_H
#define CPU_H

t_pcb pcb;
t_dictionary *diccionario_variables;

int checkArgs(int args);
int crearLogger(t_log **logger);
int cargarConfig(t_config **config, char *path);
void generarDiccionarioVariables(t_pcb *pcb);
void destructor_elementos_diccionario(void *elemento);

#endif /* CPU_H */
