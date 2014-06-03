#include <stdio.h>

#include <parser/metadata_program.h>
#include <parser/parser.h>

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