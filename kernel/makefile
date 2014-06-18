kernel: ./src/kernel.c ./src/kernel.h ./src/plp.c ./src/pcp.c ../utils/comunicacion.c ../utils/interfaz_umv.c
	mkdir -p bin
	gcc -g ./src/kernel.c ./src/plp.c ./src/pcp.c ../utils/comunicacion.c ../utils/interfaz_umv.c -lcommons -lpthread -lparser-ansisop -o ./bin/kernel
