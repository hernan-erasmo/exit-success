#include <semaphore.h>
#include <commons/log.h>
#include <commons/config.h>

#ifndef KERNEL_H
#define KERNEL_H

#include "hilo_entrada_salida.h"
#include "plp.h"
#include "pcp.h"

t_list *cola_new;
t_list *cola_ready;
t_list *cola_exec;
t_list *cola_block;
t_list *cola_exit;

uint32_t multiprogramacion;
uint32_t tamanio_quantum;

sem_t s_ready_max;
sem_t s_ready_nuevo;
sem_t s_exit;
sem_t s_hay_cpus;

t_list *cabeceras_io;	//Una lista de estructuras de tipo t_cola_io

typedef struct cola_io {
	char *nombre_dispositivo;
	uint32_t tiempo_espera;
	t_list *cola_dispositivo;	//Una lista de estructuras de tipo t_pcb_en_io
	t_log *logger;
	sem_t *s_cola;
} t_cola_io;

typedef struct pcb_en_io {
	t_pcb *pcb_en_espera;
	uint32_t unidades_de_tiempo;
} t_pcb_en_io;

t_list *listaCompartidas;	//Una lista de estructuras de tipo t_compartida

typedef struct compartida {
	char *nombre;
	int valor;
} t_compartida;

int checkArgs(int args);
int crearLogger(t_log **logger);
int cargarConfig(t_config **config, char *path);
void cargarInfoIO(t_list **cabeceras, t_config *config, t_log *logger);
void cargarInfoCompartidas(t_list **listaCompartidas, t_config *config, t_log *logger);
int crearHilosIO(t_list *cabeceras, t_log *logger);
int contarOcurrenciasElementos(char *cadena);
t_datos_plp *crearConfiguracionPlp(t_config *config, t_log *logger);
t_datos_pcp *crearConfiguracionPcp(t_config *config, t_log *logger);

int enviarMensajePrograma(int *socket, char *motivo, char *mensaje);

// SYSCALLS
int syscall_entradaSalida(char *nombre_dispositivo, t_pcb *pcb_en_espera, uint32_t tiempoEnUnidades, t_log *logger);
int syscall_obtenerValorCompartida(char *nombre_compartida, int socket_respuesta, t_log *logger);

#endif /* KERNEL_H */
