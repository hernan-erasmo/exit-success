umv: ./src/umv.c ./src/atencionConexiones.c ./src/segmento.c ./src/consola.c ../utils/comunicacion.c
	mkdir -p bin
	gcc -g ./src/umv.c ./src/atencionConexiones.c ./src/segmento.c ./src/consola.c ../utils/comunicacion.c -lcommons -lpthread -o ./bin/umv

run-umv: 
	$(MAKE) umv
	valgrind --leak-check=full ./bin/umv ./config_files/umv.config

prod-umv-prueba1:
	./bin/umv ./config_files/prueba1.config
