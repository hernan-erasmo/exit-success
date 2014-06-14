kernel: ./src/kernel.c ./src/kernel.h ./src/plp.c ../utils/comunicacion.c
	mkdir -p bin
	gcc -g ./src/kernel.c ./src/plp.c ../utils/comunicacion.c -lcommons -lpthread -lparser-ansisop -o ./bin/kernel
