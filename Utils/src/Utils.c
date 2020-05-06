#include "Utils.h"

void iniciar_logger(char* file, char* program_name) {

	if(file == NULL){
			printf("No se pudo encontrar el path del config.");
			return exit(-1);
	}

	logger = log_create(file, program_name, 1, LOG_LEVEL_INFO);

	if (logger == NULL){
		printf("ERROR EN LA CREACION DEL LOGGER/n");
		exit(-2);
	}
}

void* serializar_paquete(t_paquete* paquete, int* bytes) {
	int malloc_size = paquete -> buffer -> size + sizeof(op_code) + sizeof(int);
	void* stream = malloc(malloc_size);
	int offset = 0;

	memcpy(stream+offset, &(paquete -> codigo_operacion), sizeof(paquete -> codigo_operacion));
	offset += sizeof(paquete -> codigo_operacion);
	memcpy(stream + offset, &(paquete -> buffer -> size), sizeof(paquete -> buffer -> size));
	offset += sizeof(paquete -> buffer -> size);
	memcpy(stream + offset, paquete -> buffer -> stream, paquete -> buffer -> size);
	offset += paquete -> buffer -> size;

	(*bytes) = malloc_size;
	log_info(logger, "bytes: %d", *bytes);
	log_info(logger, "cod op a enviar %d", paquete -> codigo_operacion);
	log_info(logger, "tam a enviar %d", paquete -> buffer -> size);
	log_info(logger, "mensaje a enviar %s", paquete -> buffer -> stream);

	return stream;
}

int crear_conexion(char *ip, char* puerto) {
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info -> ai_family, server_info -> ai_socktype, server_info -> ai_protocol);

	if(connect(socket_cliente, server_info -> ai_addr, server_info -> ai_addrlen) == -1)
		printf("error");

	freeaddrinfo(server_info);

	return socket_cliente;
}

void enviar_mensaje(int cod_op, void* mensaje, int socket_cliente) {
	t_buffer* buffer = malloc(sizeof(t_buffer));

	buffer -> size = sizeof(mensaje);
	//buffer -> stream = malloc(buffer -> size);
	buffer -> stream = mensaje;
	log_info(logger,"Armando paquete");

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete -> codigo_operacion = cod_op;
	paquete -> buffer = buffer;

	int size_serializado;
	void* stream = serializar_paquete(paquete, &size_serializado);
	log_info(logger,"Paquete serializado con tamaño :%d",size_serializado);
	send(socket_cliente, stream, size_serializado, 0);
	log_info(logger,"Paquete enviado");
	//free(buffer -> stream);
	free(buffer);
	free(paquete);
	free(stream);
}

/*void* recibir_mensaje(int socket_cliente, int* size) {
	log_info(logger, "Recibiendo mensaje.");
	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	log_info(logger, "Tamano de paquete recibido: %d", *size);
	void* buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);
	log_info(logger, "Mensaje recibido: %s", buffer);
	return buffer;
}*/

void iniciar_servidor(char *IP, char *PUERTO) {
	int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(IP, PUERTO, &hints, &servinfo);

    for (p = servinfo; p != NULL; p = p -> ai_next) {
        if ((socket_servidor = socket(p -> ai_family, p -> ai_socktype, p -> ai_protocol)) == -1)
            continue;

        if (bind(socket_servidor, p -> ai_addr, p -> ai_addrlen) == -1) {
            close(socket_servidor);
            continue;
        }
        break;
    }

	listen(socket_servidor, SOMAXCONN);

    freeaddrinfo(servinfo);
    log_info(logger, "Puerto: %s", PUERTO);
    log_info(logger, "Servidor levantado.");
    while(1)
    	esperar_cliente(socket_servidor);
}

void esperar_cliente(int socket_servidor) {

	struct sockaddr_in dir_cliente;

	int tam_direccion = sizeof(struct sockaddr_in);
	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);
	pthread_create(&thread, NULL, (void*)serve_client, &socket_cliente);
	pthread_detach(thread);

}

void serve_client(int* socket) {
	int cod_op;

	if(recv(*socket, &cod_op, sizeof(int), MSG_WAITALL) == -1)
		cod_op = -1;

	log_info(logger,"Se conecto un cliente con socket: %d",*socket);
	process_request(cod_op, *socket);
	close(*socket);
}

void liberar_conexion(int socket_cliente) {
	close(socket_cliente);
}

void liberar_logger(){
	if(logger != NULL){
		log_destroy(logger);
	}
}
