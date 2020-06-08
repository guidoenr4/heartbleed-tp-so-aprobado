#include "broker.h"


int main(void) {
	iniciar_programa();
	terminar_programa(logger, config_broker);
	return 0;
}

void iniciar_programa(){
	id_mensaje_univoco = 0;
	leer_config();
	iniciar_logger(config_broker->log_file, "broker");
	reservar_memoria();
	iniciar_semaforos();
	crear_colas_de_mensajes();
    crear_listas_de_suscriptores();
	log_info(logger, "IP: %s", config_broker -> ip_broker);
	iniciar_servidor(config_broker -> ip_broker, config_broker -> puerto);

}

void reservar_memoria(){
	memoria_cache = malloc(config_broker ->  size_memoria);
}

void iniciar_semaforos(){
    sem_init(&semaforo, 0, 1);
    sem_init(&mutex_id_correlativo,0,1);
}

void destruir_semaforos(){
	sem_destroy(&semaforo);
	sem_destroy(&mutex_id_correlativo);
}

void crear_colas_de_mensajes(){

	 cola_new = list_create();
	 cola_appeared = list_create();
	 cola_get = list_create();
	 cola_localized = list_create();
	 cola_catch = list_create();
	 cola_caught = list_create();
}

void crear_listas_de_suscriptores(){

	 lista_suscriptores_new = list_create();
	 lista_suscriptores_appeared = list_create();
	 lista_suscriptores_get = list_create();
	 lista_suscriptores_localized = list_create();
	 lista_suscriptores_catch = list_create();
	 lista_suscriptores_caught = list_create();
}

void leer_config() {

	t_config* config;

	config_broker = malloc(sizeof(t_config_broker));

	config = config_create("broker.config");

	if(config == NULL){
		    	printf("No se pudo encontrar el path del config.");
		    	exit(-2);
	}
	config_broker -> size_memoria = config_get_int_value(config, "TAMANO_MEMORIA");
	config_broker -> size_min_memoria = config_get_int_value(config, "TAMANO_MEMORIA");
	config_broker -> algoritmo_memoria = strdup(config_get_string_value(config, "ALGORITMO_MEMORIA"));
	config_broker -> algoritmo_reemplazo = strdup(config_get_string_value(config, "ALGORITMO_REEMPLAZO"));
	config_broker -> algoritmo_particion_libre = strdup(config_get_string_value(config, "ALGORITMO_PARTICION_LIBRE"));
	config_broker -> ip_broker = strdup(config_get_string_value(config, "IP_BROKER"));
	config_broker -> puerto = strdup(config_get_string_value(config, "PUERTO_BROKER"));
	config_broker -> frecuencia_compactacion = config_get_int_value(config, "FRECUENCIA_COMPACTACION");
	config_broker -> log_file = strdup(config_get_string_value(config, "LOG_FILE"));

	config_destroy(config);
}

void liberar_config(t_config_broker* config) {
	free(config -> algoritmo_memoria);
	free(config -> algoritmo_reemplazo);
	free(config -> algoritmo_particion_libre);
	free(config -> ip_broker);
	free(config -> puerto);
	free(config->log_file);
	free(config);
}

void terminar_programa(t_log* logger, t_config_broker* config) {
	liberar_listas();
	liberar_config(config);
	liberar_logger(logger);
	liberar_memoria_cache();
	destruir_semaforos();
}

void liberar_memoria_cache(){
	free(memoria_cache);
}

void liberar_listas(){
	list_destroy(cola_new);
	list_destroy(cola_appeared);
	list_destroy(cola_get);
	list_destroy(cola_localized);
	list_destroy(cola_catch);
	list_destroy(cola_caught);
	list_destroy(lista_suscriptores_new);
	list_destroy(lista_suscriptores_appeared);
	list_destroy(lista_suscriptores_get);
	list_destroy(lista_suscriptores_localized);
	list_destroy(lista_suscriptores_catch);
	list_destroy(lista_suscriptores_caught);
}


//---------------------------------------------------------------------------------------------------------------------------

/*EL SERVICE DEL BROKER*/
void process_request(uint32_t cod_op, uint32_t cliente_fd) {
	uint32_t size;
	void* msg;

	log_info(logger,"Codigo de operacion %d",cod_op);

	switch (cod_op) {
		case GET_POKEMON:
			msg = malloc(sizeof(t_get_pokemon));
			msg = recibir_mensaje(cliente_fd, &size);
			agregar_mensaje(GET_POKEMON, size, msg, cliente_fd);
			free(msg);
			break;
		case CATCH_POKEMON:
			msg = malloc(sizeof(t_catch_pokemon));
			msg = recibir_mensaje(cliente_fd, &size);
			agregar_mensaje(CATCH_POKEMON, size, msg, cliente_fd);
			free(msg);
			break;
		case LOCALIZED_POKEMON:
			msg = malloc(sizeof(t_localized_pokemon));
			msg = recibir_mensaje(cliente_fd, &size);
			agregar_mensaje(LOCALIZED_POKEMON, size, msg, cliente_fd);
			free(msg);
			break;
		case CAUGHT_POKEMON:
			msg = malloc(sizeof(t_caught_pokemon));
			msg = recibir_mensaje(cliente_fd, &size);
			agregar_mensaje(CAUGHT_POKEMON, size, msg, cliente_fd);
			free(msg);
			break;
		case APPEARED_POKEMON:
			msg = malloc(sizeof(t_caught_pokemon));
			msg = recibir_mensaje(cliente_fd, &size);
			agregar_mensaje(APPEARED_POKEMON, size, msg, cliente_fd);
			free(msg);
			break;
		case NEW_POKEMON:
			msg = malloc(sizeof(t_new_pokemon));
			msg = recibir_mensaje(cliente_fd, &size);
			agregar_mensaje(NEW_POKEMON, size, msg, cliente_fd);
			free(msg);
			break;
		case SUBSCRIPTION:
			msg = malloc(sizeof(t_suscripcion));
			msg = recibir_mensaje(cliente_fd, &size);
			agregar_mensaje(SUBSCRIPTION, size, msg, cliente_fd);
			free(msg);
			break;
		case ACK:
			msg = malloc(sizeof(t_ack));
			uint32_t id_mensaje;
			msg = recibir_confirmacion_de_recepcion(&cliente_fd, &id_mensaje); // -> tenemos que saber quien es
			//En realidad el mensaje se desencola cuando TODOS ya lo recibieron.
			//Habría que revisar.
			//Se itera sobre la lista de mensajes -> matchea el mensaje que se confirma y se agrega a la lista de suscriptores recibido.
			actualizar_mensaje_confirmado(msg);

			/*t_mensaje* mensaje;
			t_list* suscriptores = encontrar_suscriptores();//-> encontrar tu mensaje con sus sucripptores_ok
			actualizar_suscriptores(mensaje, suscriptores);//-> actualizas la lista con el suscriptor nuevo y ademas chequeas si podes eliminar el mensaje*/
			free(msg);
			break;
		case 0:
			log_info(logger,"No se encontro el tipo de mensaje");
			pthread_exit(NULL);
		case -1:
			pthread_exit(NULL);
	}

}
//REVISAR
void actualizar_mensaje_confirmado(t_ack* mensaje){
	op_code cola_de_mensaje_confirmado = mensaje -> tipo_mensaje;

	void actualizar_suscripto(void* mensaje){
		//Se busca dentro del mensaje en la lista de los susriptores enviados.
		//Se saca de esa lista.
		//Se pasa a la lista de suscriptores recibidos.
	}

	switch(cola_de_mensaje_confirmado){
		case GET_POKEMON:
			list_iterate(cola_get, actualizar_suscripto);
			break;
		case CATCH_POKEMON:
			list_iterate(cola_catch, actualizar_suscripto);
			break;
		case LOCALIZED_POKEMON:
			list_iterate(cola_localized, actualizar_suscripto);
			break;
		case CAUGHT_POKEMON:
			list_iterate(cola_caught, actualizar_suscripto);
			break;
		case APPEARED_POKEMON:
			list_iterate(cola_appeared, actualizar_suscripto);
			break;
		case NEW_POKEMON:
			list_iterate(cola_new, actualizar_suscripto);
			break;
		default:
			log_info(logger, "El mensaje no se encuentra disponible");
			break;
	}

}

t_list* encontrar_suscriptores(){
	t_list* suscriptores;
	return suscriptores;
}

void actualizar_suscriptores(t_mensaje* mensaje){
	//validas que una vez que esten todos ok, bye bye mensaje
	desencolar_mensaje(mensaje);
}

void* recibir_mensaje(uint32_t socket_cliente, uint32_t* size) {
	//t_paquete* paquete = malloc(sizeof(t_paquete));
	//Se recibe un paquete enviado por otro modulo
	void* buffer;
	log_info(logger, "Recibiendo mensaje.");
	sem_wait(&semaforo);
	recv(socket_cliente, size, sizeof(uint32_t), MSG_WAITALL);
	log_info(logger, "Tamano de paquete recibido: %d", *size);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);
	sem_post(&semaforo);
	log_info(logger, "Mensaje recibido: %s", buffer);
    return buffer;
}

void agregar_mensaje(uint32_t cod_op, uint32_t size, t_paquete* paquete, uint32_t socket_cliente){
	log_info(logger, "Agregando mensaje");
	log_info(logger, "Size: %d", size);
	log_info(logger, "Socket_cliente: %d", socket_cliente);
	log_info(logger, "Payload: %s", (char*) paquete);

	t_mensaje* mensaje = malloc(sizeof(t_mensaje));
	uint32_t nuevo_id;

	if(paquete -> id_mensaje != 0){
		paquete -> id_mensaje_correlativo = paquete -> id_mensaje;
		mensaje -> id_correlativo 		  = mensaje -> id_mensaje;
		nuevo_id						  = generar_id_univoco();
		paquete -> id_mensaje 			  = nuevo_id;
	} else {
		nuevo_id = generar_id_univoco();
		paquete -> id_mensaje 			  = nuevo_id;
	}

	//Este id habría que ver como pasarlo al id de un t_get por ejemplo.

	mensaje -> payload 		    = paquete;
	mensaje -> codigo_operacion = cod_op;
	//mensaje -> id_correlativo = generar_id_univoco();

	sem_post(&mutex_id_correlativo);
	send(socket_cliente, &(paquete -> id_mensaje) , sizeof(uint32_t), 0); //Avisamos,che te asiganmos un id al mensaje
	sem_post(&mutex_id_correlativo);

	//mensaje->id_mensaje = generar_id_univoco();
	// En memori deberia guardarse solo el contenido y algunos id o referencia, no todo el mensaje.
	guardar_en_memoria(mensaje);

	sem_wait(&semaforo);
	encolar_mensaje(mensaje, cod_op);
	sem_post(&semaforo);
}

uint32_t generar_id_univoco(){
	pthread_mutex_t mutex_id_univoco = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&mutex_id_univoco);
	id_mensaje_univoco++;
	pthread_mutex_unlock(&mutex_id_univoco);

	pthread_mutex_destroy(&mutex_id_univoco);

	return id_mensaje_univoco;
}

void encolar_mensaje(t_mensaje* mensaje, op_code codigo_operacion){

	switch (codigo_operacion) {
			case GET_POKEMON:
				list_add(cola_get, mensaje);
				log_info(logger, "Mensaje agregado a cola de mensajes get.");
				break;
			case CATCH_POKEMON:
				list_add(cola_catch, mensaje);
				log_info(logger, "Mensaje agregado a cola de mensajes catch.");
				break;
			case LOCALIZED_POKEMON:
				list_add(cola_localized, mensaje);
				log_info(logger, "Mensaje agregado a cola de mensajes localized.");
				break;
			case CAUGHT_POKEMON:
				list_add(cola_caught, mensaje);
				log_info(logger, "Mensaje agregado a cola de mensajes caught.");
				break;
			case APPEARED_POKEMON:
				list_add(cola_appeared, mensaje);
				log_info(logger, "Mensaje agregado a cola de mensajes appeared.");
				break;
			case NEW_POKEMON:
				list_add(cola_new, mensaje);
				log_info(logger, "Mensaje agregado a cola de mensajes new.");
				break;
			case SUBSCRIPTION:
				recibir_suscripcion(mensaje);
				break;
			default:
				log_info(logger, "El codigo de operacion es invalido");
				exit(-6);
	}
}

//-----------------------SUSCRIPCIONES------------------------//
void recibir_suscripcion(t_mensaje* mensaje){

	t_suscripcion* mensaje_suscripcion = malloc(sizeof(t_suscripcion));

	mensaje_suscripcion 			   = deserealizar_suscripcion(mensaje -> payload);
	t_suscriptor* suscripcion    				       = armar_suscripcion_a_guardar(mensaje_suscripcion);

	log_info(logger, "Se recibe una suscripción.");
		switch (mensaje_suscripcion -> cola_a_suscribir) {
			 case GET_POKEMON:
				suscribir_a_cola(lista_suscriptores_get, suscripcion, mensaje_suscripcion -> cola_a_suscribir);
				break;
			 case CATCH_POKEMON:
				suscribir_a_cola(lista_suscriptores_catch, suscripcion, mensaje_suscripcion -> cola_a_suscribir);
				break;
			 case LOCALIZED_POKEMON:
				suscribir_a_cola(lista_suscriptores_localized, suscripcion, mensaje_suscripcion -> cola_a_suscribir);
				break;
			 case CAUGHT_POKEMON:
				suscribir_a_cola(lista_suscriptores_caught, suscripcion, mensaje_suscripcion -> cola_a_suscribir);
				break;
			 case APPEARED_POKEMON:
				suscribir_a_cola(lista_suscriptores_appeared,suscripcion, mensaje_suscripcion -> cola_a_suscribir);
				break;
			 case NEW_POKEMON:
				suscribir_a_cola(lista_suscriptores_new, suscripcion, mensaje_suscripcion -> cola_a_suscribir);
				break;
			 default:
				log_info(logger, "Ingrese un codigo de operacion valido");
				break;
		 }

	 free(mensaje_suscripcion);
	 free(suscripcion);

}

t_suscriptor* armar_suscripcion_a_guardar(t_suscripcion* mensaje_suscripcion){
	t_suscriptor* nueva_suscripcion = malloc(sizeof(t_suscriptor));
	nueva_suscripcion -> emisor   = mensaje_suscripcion -> id_proceso;
	nueva_suscripcion -> socket   = mensaje_suscripcion -> socket;
	nueva_suscripcion -> temporal = mensaje_suscripcion -> tiempo_suscripcion;
	return nueva_suscripcion;
}


//Ver de agregar threads.
void suscribir_a_cola(t_list* lista_suscriptores, t_suscriptor* suscripcion, op_code cola_a_suscribir){

	log_info(logger, "EL cliente fue suscripto a la cola de mensajes:%s.", (char*)(cola_a_suscribir));

	//Nombre malisimo, hay que revisar.
	informar_mensajes_previos(suscripcion,cola_a_suscribir);

	bool es_la_misma_suscripcion(void* una_suscripcion){
		t_suscriptor* otra_suscripcion = una_suscripcion;
		return otra_suscripcion -> emisor == suscripcion -> emisor;
	}

	if(suscripcion -> temporal != 0){
		sleep(suscripcion -> temporal);
		list_remove_by_condition(lista_suscriptores, es_la_misma_suscripcion);
		//list_remove_and_destroy_by_condition(lista_suscriptores, es_la_misma_suscripcion, destruir_suscripcion);
		log_info(logger, "La suscripcion fue anulada correctamente.");
	}

}

//REVISAR FUERTE
void destruir_suscripcion(void* suscripcion) {
	t_suscriptor* una_suscripcion = suscripcion;
	free(una_suscripcion -> emisor);
	free((t_suscriptor*)suscripcion);
	free(una_suscripcion);
}


//REVISAR SI LOS HISTORIALES ESTÁN BIEN RELACIONADOS
//REVISAR EL PARAMETRO QUE RECIBE
void informar_mensajes_previos(t_suscriptor* una_suscripcion, op_code cola_a_suscribir){

	switch(cola_a_suscribir){
		case GET_POKEMON: //GAME_CARD SUSCRIPTO
			descargar_historial_mensajes(GET_POKEMON, una_suscripcion -> socket);
			log_info(logger, "El proceso suscripto recibe los mensajes get del historial");
			break;
		case CATCH_POKEMON: //GAME_CARD SUSCRIPTO
			descargar_historial_mensajes(CATCH_POKEMON, una_suscripcion -> socket);
			log_info(logger, "El proceso suscripto recibe los mensajes catch del historial");
			break;
		case LOCALIZED_POKEMON: //TEAM SUSCRIPTO
			descargar_historial_mensajes(LOCALIZED_POKEMON, una_suscripcion -> socket);
			log_info(logger, "El proceso suscripto recibe los mensajes localized del historial");
			break;
		case CAUGHT_POKEMON: //TEAM SUSCRIPTO
			descargar_historial_mensajes(CAUGHT_POKEMON, una_suscripcion -> socket);
			log_info(logger, "El proceso suscripto recibe los mensajes caught del historial");
			break;
		case NEW_POKEMON: //GAME_CARD SUSCRIPTO
			descargar_historial_mensajes(NEW_POKEMON, una_suscripcion -> socket);
			log_info(logger, "El proceso suscripto recibe los mensajes new del historial");
			break;
		case APPEARED_POKEMON: //TEAM SUSCRIPTO
			descargar_historial_mensajes(APPEARED_POKEMON, una_suscripcion -> socket);
			log_info(logger, "El proceso suscripto recibe los mensajes appeared del historial");
			break;
		default:
			log_info(logger, "No se pudo descargar el historial de mensajes satisfactoriamente.");
			break;
	}
}

void descargar_historial_mensajes(op_code tipo_de_mensaje, uint32_t socket_cliente){
	//Se recurre a la variable memoria_cache.
	//Se puede usar la lista auxiliar de mensajes para hacerlo (con el tipo de mensaje).
	//Habria que filtrar por codigo de operación los mensajes en la cache.
	//Despues enviar todos los mensajes de cada cola a la que fue suscripta.
}

t_suscripcion* deserealizar_suscripcion(void* stream){
	t_suscripcion* suscripcion = malloc(sizeof(t_suscripcion));
	memcpy(&(suscripcion->socket), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memcpy(&(suscripcion->tiempo_suscripcion), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memcpy(&(suscripcion->cola_a_suscribir), stream, sizeof(op_code));
	stream += sizeof(op_code);
	return suscripcion;
}

t_ack* recibir_confirmacion_de_recepcion(uint32_t socket_cliente, uint32_t id_mensaje){
	t_ack* acknowledgment;
	//REVISAR EL RECV --> recibe el paquete en si o el stream?
	//Una vez recibido me fijo si hay que deserializar y después hago las demás
	//O sea despues actualizo las listas de suscriptores dentro de cada mensaje.

	/*recv(socket_cliente, &acknowledgment, sizeof(op_code), MSG_WAITALL);
	recv(socket_cliente, id_mensaje, sizeof(uint32_t), MSG_WAITALL);*/
	return acknowledgment;

}

//---------------------------MENSAJES---------------------------//

//REViSAR
void desencolar_mensaje(t_mensaje* mensaje){

 switch (mensaje->codigo_operacion) {
			case GET_POKEMON:
				list_remove_and_destroy_element(cola_get, mensaje->id_correlativo, sizeof(t_mensaje));
				log_info(logger, "Mensaje eliminado a cola de mensajes get.");

				break;
			case CATCH_POKEMON:
				list_remove_and_destroy_element(cola_catch, mensaje->id_correlativo,sizeof(t_mensaje));
				log_info(logger, "Mensaje eliminado a cola de mensajes catch.");
				break;
			case LOCALIZED_POKEMON:
				list_remove_and_destroy_element(cola_localized, mensaje->id_correlativo,sizeof(t_mensaje));
				log_info(logger, "Mensaje eliminado a cola de mensajes localized.");
				break;
			case CAUGHT_POKEMON:
				list_remove_and_destroy_element(cola_caught, mensaje->id_correlativo,sizeof(t_mensaje));
				log_info(logger, "Mensaje eliminado a cola de mensajes caught.");
				break;
			case APPEARED_POKEMON:
				list_remove_and_destroy_element(cola_appeared, mensaje->id_correlativo,sizeof(t_mensaje));
				log_info(logger, "Mensaje eliminado a cola de mensajes appeared.");
				break;
			case NEW_POKEMON:
				list_remove_and_destroy_element(cola_new, mensaje->id_correlativo,sizeof(t_mensaje));
				log_info(logger, "Mensaje eliminado a cola de mensajes new.");
				break;
			default:
				log_info(logger, "El codigo de operacion es invalido");
				exit(-6);

}
}

void enviar_mensajes_get(){
	void mensajear_suscriptor(void* suscriptor){
		//list_iterate(cola_get, enviar_mensaje_get);
	}

	void enviar_mensaje_get(void* mensaje){
		void* suscriptor;
		t_mensaje* mensaje_a_enviar = malloc(sizeof(t_mensaje));
	    mensaje_a_enviar = mensaje;
	    if(list_find(mensaje_a_enviar->suscriptor_enviado,suscriptor)) {
		enviar_mensaje(GET_POKEMON, mensaje_a_enviar-> payload, socket, sizeof(t_paquete)); //REVISAR SIZEOF
		free(mensaje_a_enviar);
		agregar_suscriptor(mensaje_a_enviar, suscriptor);///agregamos el suscrptor a mensaje_> suscriptor_>enviado
	    }

	}
	//Habría que ver como iterar on la lista de mensajes, porque estamos usando la lista de suscriptores
    list_iterate( lista_suscriptores_get, mensajear_suscriptor); //El segundo parámetro es una operación que hace enviar a los sockets un paquete?



}

void agregar_suscriptor(t_mensaje* mensaje, void* suscriptor){
	list_add(mensaje -> suscriptor_enviado, suscriptor);
}

//--------------MEMORIA-------------//
void ubicar_particion_de_memoria(){
	char* algoritmo_de_memoria = config_broker -> algoritmo_memoria;
	if(string_equals_ignore_case(algoritmo_de_memoria, "BS")){
		//ubicar_con_particiones_dinamicas();
	} else {
		//ubicar_con_buddy_system();
	}
}

void eliminar_particion_de_memoria(){
	char* algoritmo_de_reemplazo = config_broker -> algoritmo_reemplazo;
	if(string_equals_ignore_case(algoritmo_de_reemplazo, "FF")){
		//eliminar con fifo
	} else {
		//eliminar con lru
	}
}

void compactar_memoria(){
	//Esto debería ser un hilo que periódicamente haga la compactación?
	//uint32_t frecuencia_de_compactacion = config_broker -> frecuencia_compactacion;

}


void guardar_en_memoria(t_mensaje* mensaje){

	if(string_equals_ignore_case(config->algoritmo_memoria,"BS"))
	{
       /*
        *buscarparticionlibre
        ubicarla
        */
	}
	if(string_equals_ignore_case(config->algoritmo_memoria,"PARTICIONES"))
	{
		/*       *chequear_algoritmo_particion -> buscarparticionlibre
		 *        *chequear_algoritmo_reemplazo -> ubicarla   */
	}

}









