kernel: ./src/kernel.c ./src/kernel.h ./src/plp.c ./src/pcp.c ./src/vaciar_exit.c ./src/wt_nar.c ../utils/comunicacion.c ../utils/interfaz_umv.c
	mkdir -p bin
	gcc -g ./src/kernel.c ./src/plp.c ./src/pcp.c ./src/vaciar_exit.c ./src/wt_nar.c ../utils/comunicacion.c ../utils/interfaz_umv.c -lcommons -lpthread -lparser-ansisop -o ./bin/kernel
