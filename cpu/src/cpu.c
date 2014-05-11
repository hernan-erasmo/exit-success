#include <stdio.h>

#include <parser/metadata_program.h>
#include <parser/parser.h>

typedef struct pcb {
	int32_t id;			//Identificador del programa
	void *seg_cod;		//Puntero al comienzo del segmento de código en la umv
	void *seg_stack;	//Puntero al comienzo del segmento de stack en la umv
	void *cursor_stack;	//Puntero al primer byte del contexto de ejcución actual
	void *seg_i_cod;	//Puntero al comienzo del índice de código en la umv
	void *seg_i_etq;	//Puntero al comienzo del índice de etiquetas en la umv
	int32_t p_counter;	//Contiene el nro de la próxima instrucción a ejecutar
	int32_t size_ctxt_actual;	//Cant de variables (locales y parámetros) del contexto de ejecución actual
	int32_t	size_i_etq;	//Cantidad de bytes que ocupa el índice de etiquetas
} t_pcb;

void finalizar(void);

int main(int argc, char *argv[])
{
	char* const asd = "end";

	AnSISOP_funciones *funciones = malloc(sizeof(AnSISOP_funciones));
	funciones->AnSISOP_finalizar = finalizar;

	analizadorLinea(asd, funciones, NULL);

	free(funciones);

	return 0;
}

void finalizar(void)
{
	printf("Estoy adentro de la función finalizar. Adiós para siempre.\n");

	return;
}