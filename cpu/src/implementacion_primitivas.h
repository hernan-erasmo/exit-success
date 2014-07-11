#include <parser/parser.h>
#include <pthread.h>

#include "cpu.h"

#ifndef IMPLEMENTACION_PRIMITIVAS_H
#define	IMPLEMENTACION_PRIMITIVAS_H

t_puntero definirVariable(t_nombre_variable identificador_variable);
t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable);
t_valor_variable dereferenciar(t_puntero direccion_variable);
void asignar(t_puntero direccion_variable, t_valor_variable valor);
t_valor_variable obtenerValorCompartida(t_nombre_compartida variable);
t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);
void irAlLabel(t_nombre_etiqueta t_nombre_etiqueta);
void llamarSinRetorno(t_nombre_etiqueta etiqueta);
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
void finalizar(void);
void retornar(t_valor_variable retorno);
void imprimir(t_valor_variable valor_mostrar);
void imprimirTexto(char* texto);
void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo);

void wait(t_nombre_semaforo identificador_semaforo);
void signal(t_nombre_semaforo identificador_semaforo);

//Funciones auxiliares
int _pushConRetorno(int puntero_cursor_aux, int cursor_viejo, int program_counter_viejo, t_puntero donde_retornar);
int _pushSinRetorno(int puntero_cursor_aux, int cursor_viejo, int program_counter_viejo);
int _popConRetorno();
int _popSinRetorno();
void _wait_bloqueante(t_nombre_semaforo identificador_semaforo);

#endif /* IMPLEMENTACION_PRIMITIVAS_H */
