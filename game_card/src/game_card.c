#include "game_card.h"
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(void) {
	config = leer_config();
	iniciar_logger("gameCard.log","gamercard");

	int socket_br;

	//conectarse(socket_br);
	iniciarTallGrass();

//	t_new_pokemon* device = malloc(sizeof(t_new_pokemon));
//	device->cantidad= 1;
//	device->id_mensaje= 124;
//	device->pokemon= "device";
//	device->posicion[0]= 16;
//	device->posicion[1] = 51;

//	funcionHiloNewPokemon(device, socket_br);
//
//	free(device);

	printf(buscarBlockLibre());
	//mostrarBitmap(10);

	//void enviar_mensaje(uint32_t cod_op, void* mensaje, uint32_t socket_cliente, uint32_t size_mensaje) {

	//t_appeared_pokemon* deviceAP = armar_appeared(device);

	//enviar_mensaje(APPEARED_POKEMON,deviceAP,socket_br,sizeAppearedPokemon(deviceAP));

	//char* punto_montaje = config->punto_montaje_tallgrass;

	//int socket_gb = crear_conexion(config -> ip_gameBoy, config -> puerto_gameBoy);
	//enviar_mensaje(GC_LOCALIZED_POKEMON_BR, "Localized Pokemon", socket_br);
	//iniciar_servidor(config -> ip_gameCard,config -> puerto_gameCard);
	//suscribirme_a_colas();


	//enviar_new_pokemon(luken,socket_br);
	//t_new_pokemon* a = recibir_new_pokemon(socket_br, 30);

	//verificarPokemon(pikachu);
	//verificarAperturaPokemon(pikachu);

	terminar_programa(socket,config);
}

//punto_montaje = /home/utnso/workspace/tp-2020-1c-heartbleed/game_card/Montaje


void conectarse(int socket){
	socket = crear_conexion(config -> ip_broker, config -> puerto_broker);
	int time = config->tiempo_reintento_conexion;
	if (socket == -1 ){
		log_info(logger,"imposible conectar con broker, reintento en: %d",time);
		sleep(time);
		socket=0;
		conectarse(socket); //terrible negrada, pero anda o no nada?
	} else {
		log_info(logger,"conexion exitosa con broker ");
	}
}

void iniciarTallGrass(){

	punto_montaje = config->punto_montaje_tallgrass;
	log_info(logger,"Iniciando tallgrass en: %s",punto_montaje);

	if (!isDir(punto_montaje)){

		crearDirectorios(punto_montaje);
		crearMetadata(punto_montaje);
		crearBitmap(punto_montaje);
		crearBlocks(punto_montaje);

	} else {
		log_info(logger, "Tallgrass ya existente en %s",punto_montaje);
	}
}


void suscribirme_a_colas() {
	suscribirse_a(NEW_POKEMON);
	suscribirse_a(CATCH_POKEMON);
	suscribirse_a(GET_POKEMON);
}

void suscribirse_a(op_code cola) {
	uint32_t socket 			      = crear_conexion(config -> ip_broker, config -> puerto_broker);
	t_suscripcion* suscripcion 		  = malloc(sizeof(t_suscripcion));
	suscripcion -> id_proceso 		  = malloc(sizeof(char*));
	uint32_t tamanio_suscripcion	  = sizeof(t_suscripcion);

	suscripcion -> cola_a_suscribir   = cola;
	suscripcion -> id_proceso		  = "1"; //ESTE VALOR SE SACA DE CONFIG
	suscripcion -> socket 		      = socket;
	suscripcion -> tiempo_suscripcion = 0; //ESTE VALOR SIEMPRE ES 0

	enviar_mensaje(SUBSCRIPTION, suscripcion, socket, tamanio_suscripcion);

	free(suscripcion -> id_proceso);
	free(suscripcion);


}


t_config_game_card* leer_config() {

	t_config_game_card* config_game_card = malloc(sizeof(t_config_game_card));

	t_config_game_card* config = config_create("Debug/game_card.config");

	config_game_card -> tiempo_reintento_conexion = config_get_int_value(config, "TIEMPO_DE_REINTENTO_CONEXION");
	config_game_card -> tiempo_reintento_operacion = config_get_int_value(config, "TIEMPO_DE_REINTENTO_OPERACION");
	config_game_card -> punto_montaje_tallgrass = strdup(config_get_string_value(config, "PUNTO_MONTAJE_TALLGRASS"));
	config_game_card -> ip_broker = strdup(config_get_string_value(config, "IP_BROKER"));
	config_game_card -> puerto_broker = strdup(config_get_string_value(config, "PUERTO_BROKER"));
	config_game_card -> ip_gameBoy= strdup(config_get_string_value(config, "IP_GAMEBOY"));
	config_game_card -> puerto_gameBoy= strdup(config_get_string_value(config, "PUERTO_GAMECARD"));
	config_game_card -> ip_gameCard= strdup(config_get_string_value(config, "IP_GAMEBOY"));
	config_game_card -> puerto_gameCard= strdup(config_get_string_value(config, "PUERTO_GAMECARD"));
	config_game_card -> tiempo_retardo_operacion = config_get_int_value(config,"TIEMPO_RETARDO_OPERACION");

	config_destroy(config);

	return config_game_card;
}

void liberar_config(t_config_game_card* config) {
	free(config -> punto_montaje_tallgrass);
	free(config -> ip_broker);
	free(config -> puerto_broker);
	free(config);
}

void terminar_programa(int conexion,t_config_game_card* config) {
	liberar_config(config);
	liberar_logger(logger);
	liberar_conexion(conexion);
}

void crearDirectorios(char* path){

	char* blocks = concatenar(path,"/Blocks");
	char* metadata = concatenar(path,"/Metadata");
	char* files = concatenar(path,"/Files");

	//podria verificar si existen los directorios pero ni idea TODO
	//aclaro que esta terrible negrada es porque el fopen(w+b) no te crea directorios, python si obvio porque es mejor

	mkdir(path,0777);
	mkdir(metadata,0777);
	mkdir(files,0777);
	mkdir(blocks,0777);

}

void crearBlocks(char* path){

	char* realPath = concatenar(path,"/Metadata/Metadata.bin");
	t_metadata metadata = leerMetadata(realPath);

	int cantidadBloques = metadata.blocks;
	int sizeBlock = metadata.blocksize;
	int i = 1;

	char* blocksPath = concatenar(path,"/Blocks/");


	while(i <= cantidadBloques){
		char* c = string_itoa(i);
		char* block = concatenar(blocksPath,c);
		char* bloque = concatenar(block,".bin");
		createFileWithSize(bloque,sizeBlock);
		i++;
	}
	log_info(logger,"Se crearon %d blocks de tamaño: %d bytes",cantidadBloques,sizeBlock);

}

void createFileWithSize(char* path,int size) {

        int X = size - 1 ;

        FILE *fp = fopen(path, "wb");
        fseek(fp, X , SEEK_SET);
        fputc('\0', fp);

        fclose(fp);

}

void crearMetadata(char* path){
	char* realPath = concatenar(path,"/Metadata/Metadata.bin");

	FILE* file = fopen(realPath,"wb"); //write-binary

	t_metadata metadata;
	metadata.blocksize = 64;
	metadata.blocks = 5192; // es 5192 pero no voy a crear miles de bin
	metadata.magic = "TALL_GRASS";

	int tam = tamanio_de_metadata(metadata);

	fwrite(&metadata,tam,1,file);

	fclose(file);

	log_info(logger,"Se creo Metadata.bin en: %s",realPath);

}

void crearPokemon(t_new_pokemon* newPoke){

	char* metaPath = obtenerPathMetaFile(newPoke);
	char* dirPokemon = obtenerPathDirPokemon(newPoke);

	t_file_metadata metadata;
	metadata.blocks = list_create();

	asignarBlocks(&metadata.blocks);

	metadata.directory = 'N';
	metadata.open = 'N';
	metadata.size = 12; //TODO Y ESTO?

	escribirEnBlocks(metadata,newPoke);


	FILE* file = fopen(metaPath,"wb");

	fwrite(&metadata,sizeof(tamanio_de_file_metadata(metadata)),1,file);

	fclose(file);

	unlock_file(metaPath);

}

void escribirEnBlocks(t_file_metadata metadata,t_new_pokemon* newPoke){

	char* path = concatenar(config->punto_montaje_tallgrass,"/Blocks/");
	char* blockPath = concatenar(path,metadata.blocks->head->data);
	char* finalPath = concatenar(blockPath,".bin");

	FILE* block = fopen(finalPath,"wb");

	char* a_escribir = posicion_into_string(newPoke);
	int i = 0;

	while (fileSize(finalPath) <= 64){

		while (i <= string_length(a_escribir)){

					fwrite(&a_escribir[i],1,1,block);
		}

	}


}

inicializarPokemon(t_new_pokemon* newPoke){



}

char* posicion_into_string(t_new_pokemon* newpoke){

	char* a = concatenar(newpoke->posicion[0],"-");
	char* b = concatenar(a,newpoke->posicion[1]);
	char* c = concatenar(b,"=");
	char* d = concatenar(c,newpoke->cantidad);
	char* e = concatenar(d,"\n");
	return e;
}
//tengo 64 bytes para escribir en un block
//un char pesa 1 byte

t_list* asignarBlocks(t_list* lista){

	char* block = buscarBlockLibre();

	list_add(lista,block);

	return lista;
}


char* buscarBlockLibre(){ //TODO, EL BITARRAY EN UN FILE? PARA QUE

	//TODO ACA ESTA HARDCODEADO EL BITARRAY JUSTO POR ESO, NO LO PUEDO LEER DEL ARCHIVO
	//TODO leer desde el Bitmap.bin.

	int blocks = 5192;
	int size_in_bytes = 8 / blocks;
	char* bytes = calloc(size_in_bytes, sizeof(char));
	t_bitarray* bitarray = bitarray_create_with_mode(bytes, size_in_bytes, LSB_FIRST);

	int blockLibre;

	bitarray_set_bit(bitarray,0);
	bitarray_set_bit(bitarray,1);
	bitarray_set_bit(bitarray,2);
	bitarray_set_bit(bitarray,3);
	bitarray_set_bit(bitarray,4);
	bitarray_set_bit(bitarray,5);

	for(int i=0; i<= 5192 ; i++){

		if (bitarray_test_bit(bitarray,i) != true){

			blockLibre = i + 1;				//porque el array arranca en 0
			break;
		} else {
				log_info(logger,"No hay mas blocks libres ,kpo");
		}
	}

	//TODO guardar y grabar el bitarray en el Bitmap.bin
	return string_itoa(blockLibre);

}
int tamanio_de_metadata(t_metadata metadata){
	int stringLong = sizeof("TALL_GRASS") + 1 ;
	return stringLong + (sizeof(int) *2) ;
}

int tamanio_file_metadata(t_file_metadata fileMeta){
	int listSize = list_size(fileMeta.blocks);
	return sizeof(char) + listSize + sizeof(int) + sizeof(char);
}

t_metadata leerMetadata(char* path){
	FILE* file = fopen(path,"rb"); //read-binary

	if (!isFile(path)){
		log_warning(logger,"No existe el archivo a leer");
	}

	t_metadata metadata; //despues vemos como se usa esta verga

	fread(&(metadata.blocksize),sizeof(int),1,file);
	fread(&(metadata.blocks),sizeof(int),1,file);
	fread(&(metadata.magic),13,1,file);


	fclose(file);

	return metadata;
}



int fileSize(char* path){
	FILE* f = fopen(path,"r");
	fseek(f, 0L, SEEK_END);
	int i = ftell(f);
	fclose(f);
	return i;
}

bool archivoVacio(char* path){
	return fileSize(path) == 0;
}

bool isFile(char* path){
	FILE* f = fopen(path,"rb");
	fclose(f);
	return f != NULL;
}

bool isDir(const char* name){
    DIR* directory = opendir(name);

    if(directory != NULL)
    {
     closedir(directory);
     return true;
    } else {
    	return false;
    }
}

void crearBitmap(char* path){

	char* realPath = concatenar(path,"/Metadata/Bitmap.bin");
	t_metadata fileSystemMetadata = leerMetadata(concatenar(path,"/Metadata/Metadata.bin"));

	int blocks = fileSystemMetadata.blocks;

	int size_in_bytes = 8 / blocks;

	char* bytes = calloc(size_in_bytes, sizeof(char));

	t_bitarray* bitarray = bitarray_create_with_mode(bytes, size_in_bytes, LSB_FIRST);

	log_info(logger,"Se creo el bitarray de %d posiciones, con un tamaño de %d bytes",blocks,size_in_bytes);

	FILE* file = fopen(realPath,"wb");

	fwrite(&bitarray,sizeof(t_bitarray),1,file);

	bitarray_destroy(bitarray);
	free(bytes);
	fclose(file);

}

void mostrarBitmap(){

	char* path = concatenar(config->punto_montaje_tallgrass,"/Metadata/Bitmap.bin");

	t_bitarray* bitarray;

	FILE* f = fopen(path,"wb");


	fread(&bitarray,649,1,f);
	fclose(f);

	for (int i =0; i<10;i++){
		bool x = bitarray_test_bit(bitarray,i);
		printf("%d",x);
	}

	bitarray_destroy(bitarray);
}

int bitarray_default_size(){
	t_metadata metadata = leerMetadata(concatenar(config->punto_montaje_tallgrass,"/Metadata/Metadata.bin"));
	return 8 / metadata.blocks;
}

bool isDirectory(char* path){
	t_file_metadata fileMeta;
	FILE* file = fopen(path,"rb");
	fread(&fileMeta.directory,sizeof(char),1,file);
	fclose(file);

	return fileMeta.directory == 'Y';
}

bool isOpen(char* path){
	t_file_metadata fileMeta;
	FILE* file = fopen(path,"rb");

	fread(&fileMeta.open,sizeof(char),1,file);
	fclose(file);
		return fileMeta.open == 'Y';
}

void lock_file(char* path){
	FILE* f = fopen(path,"wb");
	t_file_metadata fileMeta;
	fileMeta.open = 'Y';
	fwrite(&fileMeta.open,sizeof(char),1,f);
	fclose(f);
}

void unlock_file(char* path){
	FILE* f = fopen(path,"wb");
		t_file_metadata fileMeta;
		fileMeta.open = 'N';
		fwrite(&fileMeta.open,sizeof(char),1,f);
		fclose(f);
}

void process_request(uint32_t cod_op, uint32_t cliente_fd){ // Cada case depende del que toque ese modulo.
	uint32_t* size;
	void* msg;

	log_info(logger,"Codigo de operacion %d",cod_op);

	switch (cod_op) {
		case GET_POKEMON:
//			msg = malloc(sizeof(t_get_pokemon));
//			msg = recibir_mensaje(cliente_fd, &size);
//			informarAlBroker(msg,cliente_fd,GET_POKEMON);
//			agregar_mensaje(GET_POKEMON, size, msg, cliente_fd);
//			free(msg);
			break;
		case CATCH_POKEMON:
//			msg = malloc(sizeof(t_catch_pokemon));
//			msg = recibir_mensaje(cliente_fd, &size);
//			informarAlBroker(msg,cliente_fd,CATCH_POKEMON);
//			agregar_mensaje(CATCH_POKEMON, size, msg, cliente_fd);
//			free(msg);
			break;
		case NEW_POKEMON:
			//void* recibir_mensaje(uint32_t socket_cliente, uint32_t* size);
			msg = malloc(sizeof(t_new_pokemon));
			msg = recibir_new_pokemon(cliente_fd,size); // retorna un t_new_pokemon
			informarAlBroker(cliente_fd, NEW_POKEMON);
			funcionHiloNewPokemon(msg,cliente_fd);

			//agregar_mensaje(NEW_POKEMON, size, msg, cliente_fd); ??
			free(msg);
			break;

		case 0:
			log_info(logger,"No se encontro el tipo de mensaje");
			pthread_exit(NULL);
		case -1:
			pthread_exit(NULL);
	}
}

void informarAlBroker(int socket,op_code codigo){
	char* aviso = concatenar("ACK[gamecard]: ",(char*) codigo);
	int tamanioAviso = sizeof(aviso) + 1;
	enviar_mensaje(codigo,aviso,socket,tamanioAviso);
}

void funcionHiloNewPokemon(t_new_pokemon* new_pokemon,int socket){

	verificarExistenciaPokemon(new_pokemon);
	verificarAperturaPokemon(new_pokemon,socket);

	if (existePokemonEnPosicion(new_pokemon)){
		log_info(logger,"Existe pokemon \n");

		//agregarAPoisiconExistente(); TODO PARA BLOCKS Y FILESYTEM
	} else {
		log_info(logger,"No existe pokemon \n");

		//agregarAlFinalDelArchivo(); TODO PARA BLOCKS Y FILESYTEM
	}

	t_appeared_pokemon* appeared_pokemon = armar_appeared(new_pokemon);


	enviar_appeared_pokemon(appeared_pokemon,socket);
	free(appeared_pokemon->id_mensaje);
	free(appeared_pokemon->id_mensaje_correlativo);
	free(appeared_pokemon->pokemon);
	free(appeared_pokemon->posicion);

	char* path = obtenerPathMetaFile(new_pokemon);
	unlock_file(path);

}

t_appeared_pokemon* armar_appeared(t_new_pokemon* new_pokemon){

		t_appeared_pokemon* appeared_pokemon = malloc(sizeof(t_appeared_pokemon));

		appeared_pokemon->posicion[0] = new_pokemon->posicion[0];
		appeared_pokemon->posicion[1] = new_pokemon->posicion[1];
		appeared_pokemon->pokemon = new_pokemon->pokemon;
		appeared_pokemon->id_mensaje_correlativo = 0;
		appeared_pokemon->id_mensaje = new_pokemon->id_mensaje; // TODO que onda este ID
		return appeared_pokemon;
}


void verificarExistenciaPokemon(t_new_pokemon* newpoke){
	punto_montaje = config->punto_montaje_tallgrass;

	char* montaje = concatenar(punto_montaje,"/Files/");
	char* path = concatenar(montaje,newpoke->pokemon);
	if (existeDirectorio(path)){

		log_info(logger,"Existe el pokemon %s en : %s",newpoke->pokemon,path);

	} else{

		mkdir(path, 0777);
		log_info(logger,"Se creo el directorio %s en %s",newpoke->pokemon,path);
		char* metaPath = concatenar(path,"/Metadata.bin");

		crearPokemon(newpoke);
		log_info(logger,"Se creo el file_metadata de %s",newpoke->pokemon);
	}

}

int existeDirectorio(char* path){
	DIR* dir = opendir(path);
	int x = dir;
	closedir(dir);
	return x;
}

void verificarAperturaPokemon(t_new_pokemon* newpoke,int socket){

	char* path = obtenerPathMetaFile(newpoke);

	if (isOpen(path)){
		int secs = config->tiempo_retardo_operacion;
		log_info(logger,"No se puede acceder a este pokemon porque esta en uso (OPEN=Y), reintentando en: %d",secs);
		sleep(secs);
		funcionHiloNewPokemon(newpoke,socket);

	} else {

		log_info(logger,"Se puede acceder al pokemon, lockeo el archivo %s",newpoke->pokemon);
		lock_file(path);

		}
	}

char* obtenerPathMetaFile(t_new_pokemon* pokemon){
	punto_montaje = config->punto_montaje_tallgrass;
	char* path  = concatenar (punto_montaje,"/Files/");
	char* path2 = concatenar (path,pokemon->pokemon); //terrible negrada esto, pero anda o no anda?
	char* path3 = concatenar(path2,"/Metadata.bin");
	return path3;
}

char* obtenerPathDirPokemon(t_new_pokemon* pokemon){
	punto_montaje = config->punto_montaje_tallgrass;
	char* path = concatenar(punto_montaje,"/Files/");
	char* path2 = concatenar(path,pokemon->pokemon);
	return path2;
}

bool existePokemonEnPosicion(t_new_pokemon* pokemon){

	return 1; //TODO TALLGRASS
}

void enviar_new_pokemon(t_new_pokemon* pokemon, uint32_t socket_cliente) {
	t_buffer* buffer = malloc(sizeof(t_buffer));

	buffer -> size = sizeNewPokemon(pokemon);
	buffer -> stream = malloc(buffer -> size);
	buffer -> stream = pokemon;
	log_info(logger,"Armando paquete New_Pokemon");
	//TAMBIEN HAY QUE LLENAR EL CAMPO DE ID DEL PAQUETE
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete -> codigo_operacion = NEW_POKEMON;
	paquete -> buffer = buffer;
	//HABRIA QUE DARLE UN VALOR AL SIZE SERIALIZADO
	uint32_t size_serializado;
	void* stream = serializar_paquete(paquete, &size_serializado);
	log_info(logger,"Paquete serializado con tamaño :%d",size_serializado);
	//EN VEZ DE SERIALIZARLO ACA PODES USAR LA FUNCION ENVIAR_MENSAJE
	//DEL UTILS QUE YA TE LO SERIALIZA ADENTRO.
	send(socket_cliente, stream, size_serializado, 0);
	log_info(logger,"Paquete enviado");
	//free(buffer -> stream);
	free(buffer);
	free(paquete);
	free(stream);
}

void enviar_appeared_pokemon(t_appeared_pokemon* appeared_pokemon,int socket){
		t_buffer* buffer = malloc(sizeof(t_buffer));

		buffer -> size = sizeAppearedPokemon(appeared_pokemon);
		buffer -> stream = malloc(buffer -> size);
		buffer -> stream = appeared_pokemon;
		log_info(logger,"Armando paquete APPEARED_POKEMON");

		t_paquete* paquete = malloc(sizeof(t_paquete));
		paquete -> codigo_operacion = APPEARED_POKEMON;
		paquete -> buffer = buffer;

		uint32_t size_serializado;
		void* stream = serializar_paquete(paquete, &size_serializado);
		log_info(logger,"Paquete serializado con tamaño :%d",size_serializado);
		send(socket, stream, size_serializado, 0);
		log_info(logger,"Paquete enviado");
		//free(buffer -> stream);
		free(buffer);
		free(paquete);
		free(stream);
}

t_new_pokemon* recibir_new_pokemon(uint32_t socket_cliente, uint32_t* size){
		log_info(logger, "recibiendo new_pokemon");
		recv(socket_cliente, size, sizeof(uint32_t), MSG_WAITALL);
		log_info(logger, "tamaño new_pokemon: %d", *size);
		void* buffer = malloc(*size);
		recv(socket_cliente, buffer, *size, MSG_WAITALL);
		log_info(logger, "mensaje recibido: %s", buffer);

		t_new_pokemon* pokemon = deserealizar_new_pokemon(buffer);
		return pokemon;
	}

t_appeared_pokemon* recibir_appeared_pokemon(uint32_t socket_cliente, uint32_t* size){
		log_info(logger, "recibiendo appeared_pokemon");
		recv(socket_cliente, size, sizeof(uint32_t), MSG_WAITALL);
		log_info(logger, "tamaño appeared_pokemon: %d", *size);
		void* buffer = malloc(*size);
		recv(socket_cliente, buffer, *size, MSG_WAITALL);
		log_info(logger, "mensaje recibido: %s", buffer);

		t_new_pokemon* pokemon = deserealizar_appeared_pokemon(buffer);
		return pokemon;
	}

uint32_t sizeNewPokemon(t_new_pokemon* pokemon){
	return sizeof(uint32_t) * 4 + strlen(pokemon->pokemon) + 1;
}


uint32_t sizeAppearedPokemon(t_appeared_pokemon* pokemon){
	return sizeof(uint32_t) * 4 + strlen(pokemon->pokemon) + 1;
}



