typedef struct paquete_programa {
	char id;
	char *mensaje;
	uint32_t sizeMensaje;
	uint32_t tamanio_total;
} t_paquete_programa;

int checkArgs(int args);
int crearLogger(t_log **logger);
int cargarConfig(t_config **config);
int crearConexion(int *unSocket, struct sockaddr_in *socketInfo, t_config *config, t_log *logger);
int crearSocket(struct sockaddr_in *socketInfo, t_config *config);
char *cargarScript(FILE *script, char **contenidoScript);
int enviarDatos(FILE *script, int unSocket, t_log *logger);
void finalizarEnvio(int *unSocket);
void inicializarPaquete(t_paquete_programa *paquete, uint32_t sizeMensaje, char **contenidoScript);
char *serializarPaquete(t_paquete_programa *paquete, t_log *logger);