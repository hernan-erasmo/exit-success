cpu: ./src/cpu.c ./src/cpu.h ../utils/comunicacion.c ../utils/interfaz_umv.c ./src/implementacion_primitivas.c
	mkdir -p ./bin
	gcc -g ./src/cpu.c ../utils/comunicacion.c ../utils/interfaz_umv.c ./src/implementacion_primitivas.c -lparser-ansisop -lcommons -o ./bin/cpu

run-cpu:
	$(MAKE) cpu
	valgrind --leak-check=full ./bin/cpu ./config_files/cpu.config

prod-cpu:
	./bin/cpu ./config_files/cpu.config
