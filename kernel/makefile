kernel: ./src/kernel.c ./src/kernel.h ./src/plp.c ./src/pcp.c ./src/vaciar_exit.c ./src/wt_nar.c ./src/dispatcher.c ./src/hilo_semaforo.c ./src/hilo_entrada_salida.c ../utils/comunicacion.c ../utils/interfaz_umv.c
	mkdir -p bin
	gcc -g ./src/kernel.c ./src/plp.c ./src/pcp.c ./src/vaciar_exit.c ./src/wt_nar.c ./src/dispatcher.c ./src/hilo_semaforo.c ./src/hilo_entrada_salida.c ../utils/comunicacion.c ../utils/interfaz_umv.c -lcommons -lpthread -lparser-ansisop -o ./bin/kernel

run-kernel:
	$(MAKE) kernel
	valgrind --leak-check=full ./bin/kernel ./config_files/kernel.config

prod-kernel:
	./bin/kernel ./config_files/kernel.config

prod-kernel-prueba1:
	./bin/kernel ./config_files/prueba1.config

prod-kernel-prueba_io_plp:
	./bin/kernel ./config_files/prueba_io_plp.config

prod-kernel-prueba_umv:
	./bin/kernel ./config_files/prueba_umv.config
