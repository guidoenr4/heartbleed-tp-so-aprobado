#include "broker.h"

int main(void) {
	iniciar_programa();


	signal(SIGUSR1, sig_handler);


	/*enviar_mensajes(cola_get, lista_suscriptores_get);
	enviar_mensajes(cola_catch, lista_suscriptores_catch);
	enviar_mensajes(cola_localized, lista_suscriptores_localized);
	enviar_mensajes(cola_caught, lista_suscriptores_caught);
	enviar_mensajes(cola_appeared, lista_suscriptores_appeared);
	enviar_mensajes(cola_new, lista_suscriptores_new);*/

	//uint32_t thread = pthread_create(&hilo_mensaje, NULL, gestionar_mensaje, NULL);

	terminar_programa(logger);
	return 0;
}

void sig_handler(uint32_t signo) {
    if(signo == SIGUSR1){
        dump_de_memoria();
    }
}

void iniciar_programa(){
	id_mensaje_univoco = 0;
	particiones_liberadas = 0;
	//Variable global (revisar?).
	//Es para hacer el dump de la cache.
	numero_particion = 1;
	leer_config();
	iniciar_logger(config_broker->log_file, "broker");
	reservar_memoria();
	iniciar_semaforos_broker();
	crear_colas_de_mensajes();
    crear_listas_de_suscriptores();
    //crear_hilo_segun_algoritmo();
    //crear_hilo_por_mensaje();
	log_info(logger, "IP: %s", config_broker -> ip_broker);
	iniciar_servidor(config_broker -> ip_broker, config_broker -> puerto);

}

void reservar_memoria(){
	//La memoria en sí tiene que ser un void*
	//La memoria_cache sería la lista auxiliar para esa memoria?
	memoria = malloc(config_broker ->  size_memoria);
	if(string_equals_ignore_case(config_broker -> algoritmo_memoria,"BS")){
		arrancar_buddy();
	}

    if(string_equals_ignore_case(config_broker -> algoritmo_memoria,"PARTICIONES")){
		memoria_con_particiones = list_create();
        iniciar_memoria_particiones(memoria_con_particiones);
        //Al principio la lista tiene un unico elemento que es la memoria entera.
	}
}

void crear_colas_de_mensajes(){

	 cola_new 		= list_create();
	 cola_appeared  = list_create();
	 cola_get 		= list_create();
	 cola_localized = list_create();
	 cola_catch 	= list_create();
	 cola_caught 	= list_create();
}

void crear_listas_de_suscriptores(){

	 lista_suscriptores_new 	  = list_create();
	 lista_suscriptores_appeared  = list_create();
	 lista_suscriptores_get 	  = list_create();
	 lista_suscriptores_localized = list_create();
	 lista_suscriptores_catch 	  = list_create();
	 lista_suscriptores_caught 	  = list_create();
}

void leer_config() {

	t_config* config;

	config_broker = malloc(sizeof(t_config_broker));

	config = config_create("broker.config");

	if(config == NULL){
		    	printf("No se pudo encontrar el path del config.");
		    	exit(-2);
	}
	config_broker -> size_memoria 			   = config_get_int_value(config, "TAMANO_MEMORIA");
	config_broker -> size_min_memoria 		   = config_get_int_value(config, "TAMANO_MEMORIA");
	config_broker -> algoritmo_memoria 		   = config_get_string_value(config, "ALGORITMO_MEMORIA");
	config_broker -> algoritmo_reemplazo 	   = config_get_string_value(config, "ALGORITMO_REEMPLAZO");
	config_broker -> algoritmo_particion_libre = config_get_string_value(config, "ALGORITMO_PARTICION_LIBRE");
	config_broker -> ip_broker 				   = config_get_string_value(config, "IP_BROKER");
	config_broker -> puerto 				   = config_get_string_value(config, "PUERTO_BROKER");
	config_broker -> frecuencia_compactacion   = config_get_int_value(config, "FRECUENCIA_COMPACTACION");
	config_broker -> log_file 				   = config_get_string_value(config, "LOG_FILE");
	config_broker -> memory_log 			   = config_get_string_value(config, "DUMP_CACHE");
}

void liberar_config(t_config_broker* config) {
	free(config -> algoritmo_memoria);
	free(config -> algoritmo_reemplazo);
	free(config -> algoritmo_particion_libre);
	free(config -> ip_broker);
	free(config -> puerto);
	free(config -> log_file);
	free(config -> memory_log);
	free(config);
}

void terminar_programa(t_log* logger){
	liberar_listas();
	liberar_config(config_broker);
	liberar_logger(logger);
	liberar_memoria_cache();
	liberar_semaforos_broker();
	config_destroy(config);
}

void liberar_memoria_cache(){
	free(memoria);
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
	op_code* codigo_op = malloc(sizeof(op_code));
	void* stream = recibir_paquete(cliente_fd, &size, codigo_op);
	cod_op = (*codigo_op);
	log_info(logger,"Codigo de operacion %d", cod_op);
	void* mensaje_e_agregar = deserealizar_paquete(stream, *codigo_op, size);

	switch (cod_op) {
		case GET_POKEMON:
			agregar_mensaje(GET_POKEMON, size, mensaje_e_agregar, cliente_fd);
			break;
		case CATCH_POKEMON:
			agregar_mensaje(CATCH_POKEMON, size, mensaje_e_agregar, cliente_fd);
			break;
		case LOCALIZED_POKEMON:
			agregar_mensaje(LOCALIZED_POKEMON, size, mensaje_e_agregar, cliente_fd);
			break;
		case CAUGHT_POKEMON:
			agregar_mensaje(CAUGHT_POKEMON, size, mensaje_e_agregar, cliente_fd);
			break;
		case APPEARED_POKEMON:
			agregar_mensaje(APPEARED_POKEMON, size, mensaje_e_agregar, cliente_fd);
			break;
		case NEW_POKEMON:
			agregar_mensaje(NEW_POKEMON, size, mensaje_e_agregar, cliente_fd);
			break;
		case SUBSCRIPTION:
			recibir_suscripcion(mensaje_e_agregar);
			break;
		case ACK:
			actualizar_mensajes_confirmados(mensaje_e_agregar);
			break;
		case 0:
			log_info(logger,"No se encontro el tipo de mensaje");
			pthread_exit(NULL);
		case -1:
			pthread_exit(NULL);
	}
	free(codigo_op);
	free(stream);
//REVISAR DONDE Y CUANDO HACER EL FREE DE LOS MENSAJES QUE SE AGREGARON
}


void agregar_mensaje(uint32_t cod_op, uint32_t size, void* mensaje, uint32_t socket_cliente){
	log_info(logger, "...Agregando mensaje");
	log_info(logger, "...Size: %d", size);
	log_info(logger, "...Socket_cliente: %d", socket_cliente);
	log_info(logger, "...Payload: %s", (char*) mensaje);
	t_mensaje* mensaje_a_agregar = malloc(sizeof(t_mensaje));
	uint32_t nuevo_id     = generar_id_univoco();


	mensaje_a_agregar -> id_mensaje = nuevo_id;
    //Revisar la opcion de localized.
    if(cod_op == APPEARED_POKEMON || cod_op == LOCALIZED_POKEMON || cod_op == CAUGHT_POKEMON){
      //  mensaje_a_agregar -> id_correlativo = mensaje -> id_correlativo; //revisar si esto funciona
    } else {
        mensaje_a_agregar -> id_correlativo = 0;
    }

    if(string_equals_ignore_case(config_broker -> algoritmo_memoria, "BS")){
        t_memoria_buddy* buddy = malloc(sizeof(t_memoria_buddy));
        mensaje_a_agregar -> payload = buddy;
    } else if(string_equals_ignore_case(config_broker -> algoritmo_memoria, "PARTICIONES")){
        t_memoria_dinamica* particion = malloc(sizeof(t_memoria_dinamica));
		mensaje_a_agregar -> payload = particion;
    } else {
        log_error(logger, "...No se reconoce el algoritmo de memoria.");
    }


	mensaje_a_agregar -> codigo_operacion    = cod_op;
	mensaje_a_agregar -> suscriptor_enviado  = list_create();
	mensaje_a_agregar -> suscriptor_recibido = list_create();
    mensaje_a_agregar -> tamanio_mensaje     = size;


	sem_post(&mutex_id);
	send(socket_cliente, &(nuevo_id) , sizeof(uint32_t), 0); //Avisamos,che te asiganmos un id al mensaje
	sem_post(&mutex_id);


    guardar_en_memoria(mensaje_a_agregar, mensaje);

	sem_wait(&semaforo);
	encolar_mensaje(mensaje_a_agregar, cod_op);
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
			default:
				log_error(logger, "El codigo de operacion es invalido");
				exit(-6);
	}
}

//-----------------------SUSCRIPCIONES------------------------//
void recibir_suscripcion(t_suscripcion* mensaje_suscripcion){

	op_code cola_a_suscribir		   = mensaje_suscripcion -> cola_a_suscribir;

	log_info(logger, "Se recibe una suscripción.");

		switch (cola_a_suscribir) {
			 case GET_POKEMON:
				suscribir_a_cola(lista_suscriptores_get, mensaje_suscripcion, cola_a_suscribir);
				break;
			 case CATCH_POKEMON:
				suscribir_a_cola(lista_suscriptores_catch, mensaje_suscripcion, cola_a_suscribir);
				break;
			 case LOCALIZED_POKEMON:
				suscribir_a_cola(lista_suscriptores_localized, mensaje_suscripcion, cola_a_suscribir);
				break;
			 case CAUGHT_POKEMON:
				suscribir_a_cola(lista_suscriptores_caught, mensaje_suscripcion, cola_a_suscribir);
				break;
			 case APPEARED_POKEMON:
				suscribir_a_cola(lista_suscriptores_appeared, mensaje_suscripcion, cola_a_suscribir);
				break;
			 case NEW_POKEMON:
				suscribir_a_cola(lista_suscriptores_new, mensaje_suscripcion, cola_a_suscribir);
				break;
			 default:
				log_info(logger, "Ingrese un codigo de operacion valido");
				break;
		}

}

//Ver de agregar threads.
void suscribir_a_cola(t_list* lista_suscriptores, t_suscripcion* suscripcion, op_code cola_a_suscribir){
	// --> Esto es solo para probar, una vez que funciones se saca.
	char* cola;
	switch(cola_a_suscribir){
		case GET_POKEMON:
			cola = "cola get";
			break;
		case CATCH_POKEMON:
			cola = "cola catch";
			break;
		case LOCALIZED_POKEMON:
			cola = "cola localized";
			break;
		case CAUGHT_POKEMON:
			cola = "cola caught";
			break;
		case APPEARED_POKEMON:
			cola = "cola appeared";
			break;
		case NEW_POKEMON:
			cola = "cola new";
			break;
		default:
			log_error(logger, "Se desconoce la cola a suscribir.");
			break;
	}

	log_info(logger, "EL cliente fue suscripto a la cola de mensajes: %s.", cola);
	list_add(lista_suscriptores, suscripcion);

	informar_mensajes_previos(suscripcion, cola_a_suscribir);

	bool es_la_misma_suscripcion(void* una_suscripcion){
		t_suscripcion* otra_suscripcion = una_suscripcion;
		return otra_suscripcion -> id_proceso == suscripcion -> id_proceso;
	}

	if(suscripcion -> tiempo_suscripcion != 0){
		sleep(suscripcion -> tiempo_suscripcion);
		list_remove_by_condition(lista_suscriptores, es_la_misma_suscripcion);
		//list_remove_and_destroy_by_condition(lista_suscriptores, es_la_misma_suscripcion, destruir_suscripcion);
		log_info(logger, "La suscripcion fue anulada correctamente.");
	}

}

//REVISAR FUERTE
void destruir_suscripcion(void* suscripcion) {
	free(suscripcion);
}


//REVISAR SI LOS HISTORIALES ESTÁN BIEN RELACIONADOS
//REVISAR EL PARAMETRO QUE RECIBE
void informar_mensajes_previos(t_suscripcion* una_suscripcion, op_code cola_a_suscribir){

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

void descargar_historial_mensajes(op_code tipo_mensaje, uint32_t socket_cliente){

    if(string_equals_ignore_case(config_broker -> algoritmo_memoria, "BS")){
        enviar_mensajes_cacheados_en_buddy_system(tipo_mensaje, socket_cliente);
        log_info(logger, "Los mensajes fueron descargados del historial");
    } else if (string_equals_ignore_case(config_broker -> algoritmo_memoria, "PARTICIONES")){
        enviar_mensajes_cacheados_en_particiones(tipo_mensaje, socket_cliente);
        log_info(logger, "Los mensajes fueron descargados del historial");
    } else {
        log_error(logger, "No se reconoce el algoritmo de memoria a implementar (?.");
    }
}


void enviar_mensajes_cacheados_en_buddy_system(op_code tipo_mensaje, uint32_t socket){
    //REVSAR COMO HACERLO CON RESPECTO AL BS PLANTEADO
    log_info(logger, "Los mensajes del buddy system fueron enviados.");
}

void enviar_mensajes_cacheados_en_particiones(op_code tipo_mensaje, uint32_t socket){

    void mandar_mensajes_viejos(void* mensaje){
        t_mensaje* un_mensaje = mensaje;
        t_memoria_dinamica* particion = un_mensaje -> payload;
        uint32_t offset = 0;

        uint32_t tamanio_total = (particion -> tamanio) + sizeof(uint32_t) + sizeof(op_code);

        void* stream = malloc(tamanio_total);

        memcpy(stream + offset, (&tipo_mensaje), sizeof(op_code));
        offset += sizeof(op_code);

        memcpy(stream + offset, (&tamanio_total), sizeof(uint32_t));
        offset += sizeof(uint32_t);

        memcpy(stream + offset, (memoria + (particion -> base)), (particion -> tamanio));
        offset += (particion -> tamanio);

        send(socket, stream, tamanio_total, 0);
		//REVISAR
		actualizar_ultima_referencia(mensaje);
    }

    switch(tipo_mensaje){
		case GET_POKEMON:
			list_iterate(cola_get, mandar_mensajes_viejos);
			break;
		case CATCH_POKEMON:
			list_iterate(cola_get, mandar_mensajes_viejos);
			break;
		case LOCALIZED_POKEMON:
			list_iterate(cola_get, mandar_mensajes_viejos);
			break;
		case CAUGHT_POKEMON:
			list_iterate(cola_get, mandar_mensajes_viejos);
			break;
		case APPEARED_POKEMON:
			list_iterate(cola_get, mandar_mensajes_viejos);
			break;
		case NEW_POKEMON:
			list_iterate(cola_get, mandar_mensajes_viejos);
			break;
		default:
			log_info(logger, "...No se puede enviar el mensaje pedido.");
			break;
	}


    log_info(logger, "Los mensajes de las particiones fueron enviados.");
}

void actualizar_ultima_referencia(t_mensaje* un_mensaje){
	t_memoria_dinamica* payload = (t_memoria_dinamica*)un_mensaje -> payload;
	payload ->ultima_referencia =  timestamp();
	log_info(logger, "...Se actualizó la última referencia del mensaje con id: %d", un_mensaje -> id_mensaje);
}


//---------------------------MENSAJES---------------------------//

//REVISAR
void actualizar_mensajes_confirmados(t_ack* mensaje_confirmado){
	op_code cola_de_mensaje_confirmado = mensaje_confirmado -> tipo_mensaje;

	void actualizar_suscripto(void* mensaje){
		t_mensaje* mensaje_ok = mensaje;
		if(mensaje_ok -> id_mensaje == mensaje_confirmado -> id_mensaje){
			eliminar_suscriptor_de_enviados_sin_confirmar(mensaje_ok, mensaje_confirmado -> id_proceso);
			agregar_suscriptor_a_enviados_confirmados(mensaje_ok, mensaje_confirmado -> id_proceso);
		}
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
	//Se chequea si un mensaje fue recibido por todos los suscriptores.
	//Si es así, se elimina el mensaje.
	eliminar_mensajes_confirmados();

}
//REVISAR SI ESTÁ BIEN UBICADA CUANDO SE INVOCA
void eliminar_mensajes_confirmados(){

	borrar_mensajes_confirmados(GET_POKEMON, cola_get, lista_suscriptores_get);
	borrar_mensajes_confirmados(CATCH_POKEMON, cola_catch, lista_suscriptores_catch);
	borrar_mensajes_confirmados(LOCALIZED_POKEMON, cola_localized, lista_suscriptores_localized);
	borrar_mensajes_confirmados(CAUGHT_POKEMON, cola_caught, lista_suscriptores_caught);
	borrar_mensajes_confirmados(APPEARED_POKEMON, cola_appeared, lista_suscriptores_appeared);
	borrar_mensajes_confirmados(NEW_POKEMON, cola_new, lista_suscriptores_new);

}
// HAY QUE REVISARLO PERO MINIMO 5 VECES O_o
void borrar_mensajes_confirmados(op_code tipo_lista, t_list* cola_mensajes, t_list* suscriptores){

	t_list* lista_id_suscriptores = list_create();

	void* id_suscriptor(void* un_suscriptor){
		t_suscripcion* suscripto = un_suscriptor;
		uint32_t* id = &(suscripto -> id_proceso);
		return id;
	}

	lista_id_suscriptores = list_map(suscriptores, id_suscriptor);

	bool mensaje_recibido_por_todos(void* mensaje){
		t_mensaje* un_mensaje = mensaje;

		bool suscriptor_recibio_mensaje(void* suscripto){
			uint32_t* un_suscripto = suscripto;

			bool es_el_mismo_suscripto(void* id_suscripto){
				uint32_t* alguna_suscripcion = id_suscripto;
				return (*alguna_suscripcion) == (*un_suscripto);
			}

			return list_any_satisfy(un_mensaje -> suscriptor_recibido, es_el_mismo_suscripto);
		}

		return list_all_satisfy(lista_id_suscriptores, suscriptor_recibio_mensaje);
	}

	switch(tipo_lista){
	case GET_POKEMON:
		list_remove_and_destroy_by_condition(cola_get, mensaje_recibido_por_todos, eliminar_mensaje);
		break;
	case CATCH_POKEMON:
		list_remove_and_destroy_by_condition(cola_catch, mensaje_recibido_por_todos, eliminar_mensaje);
		break;
	case LOCALIZED_POKEMON:
		list_remove_and_destroy_by_condition(cola_localized, mensaje_recibido_por_todos, eliminar_mensaje);
		break;
	case CAUGHT_POKEMON:
		list_remove_and_destroy_by_condition(cola_caught, mensaje_recibido_por_todos, eliminar_mensaje);
		break;
	case APPEARED_POKEMON:
		list_remove_and_destroy_by_condition(cola_appeared, mensaje_recibido_por_todos, eliminar_mensaje);
		break;
	case NEW_POKEMON:
		list_remove_and_destroy_by_condition(cola_new, mensaje_recibido_por_todos, eliminar_mensaje);
		break;
	default:
		log_error(logger, "El mensaje no fue eliminado correctamente");
		break;
	}
	log_info(logger, "Los mensajes confirmados por todos los suscriptores fueron eliminados.");
	list_destroy(lista_id_suscriptores);

}

void eliminar_suscriptor_de_enviados_sin_confirmar(t_mensaje* mensaje, uint32_t suscriptor){

	bool es_el_mismo_suscriptor(void* un_suscripto){
		uint32_t* suscripto = un_suscripto;
		return suscriptor == (*suscripto);
	}

	list_remove_by_condition(mensaje -> suscriptor_enviado, es_el_mismo_suscriptor);
}

void agregar_suscriptor_a_enviados_confirmados(t_mensaje* mensaje, uint32_t confirmacion){
	list_add(mensaje -> suscriptor_recibido, &confirmacion);
}
//Se le pasa por parametro la cola y la lista de sus suscriptores segun se necesite.
//Por ejemplo:
//enviar_mensajes(cola_get, lista_suscriptores_get);
void enviar_mensajes(t_list* cola_de_mensajes, t_list* lista_suscriptores){

	void mensajear_suscriptores(void* mensaje){
			t_mensaje* un_mensaje = mensaje;

			void mandar_mensaje(void* suscriptor){
				t_suscripcion* un_suscriptor = suscriptor;

				if(no_tiene_el_mensaje(un_mensaje, un_suscriptor -> id_proceso)){
					uint32_t tamanio_mensaje = size_mensaje(un_mensaje -> payload, un_mensaje -> codigo_operacion);
					enviar_mensaje(un_mensaje -> codigo_operacion, un_mensaje -> payload, un_suscriptor -> socket, tamanio_mensaje);
					agregar_suscriptor_a_enviados_sin_confirmar(un_mensaje, un_suscriptor -> id_proceso);
				}
			}
			list_iterate(lista_suscriptores, mandar_mensaje);
		}
	list_iterate(cola_de_mensajes, mensajear_suscriptores);
}

bool no_tiene_el_mensaje(t_mensaje* mensaje, uint32_t un_suscripto){
	bool mensaje_enviado;
	bool mensaje_recibido;

	bool es_el_mismo_suscripto(void* suscripto){
		uint32_t* id_suscripcion = suscripto;
		return (*id_suscripcion) == un_suscripto;
	}

	mensaje_enviado  = list_any_satisfy(mensaje -> suscriptor_enviado, es_el_mismo_suscripto);
	mensaje_recibido = list_any_satisfy(mensaje -> suscriptor_recibido, es_el_mismo_suscripto);

	return !mensaje_enviado && !mensaje_recibido;
}

void agregar_suscriptor_a_enviados_sin_confirmar(t_mensaje* mensaje_enviado, uint32_t un_suscriptor){
	list_add(mensaje_enviado -> suscriptor_enviado, &un_suscriptor);
}

//--------------MEMORIA-------------//

void dump_de_memoria(){

    //Se tiene que crear un nuevo archivo y loggear ahi todas las cosas.
    iniciar_logger(config_broker -> memory_log, "broker");

    //Muestra el inicio del dump.
    time_t t;
    struct tm *tm;
    char fechayhora[25];
    t = time(NULL);
    tm = localtime(&t);
    strftime(fechayhora, 25, "%d/%m/%Y %H:%M:%S", tm);
    printf ("Hoy es: %s\n", fechayhora);

    log_info(logger_memoria, "Dump: %s", fechayhora);

    if(string_equals_ignore_case(config_broker -> algoritmo_memoria, "PARTICIONES")){
    //Se empieza a loguear partición por partición --> considero la lista de particiones en memoria ("memoria auxiliar").
    list_iterate(memoria_con_particiones, dump_info_particion);
    } else if(string_equals_ignore_case(config_broker -> algoritmo_memoria, "BS")){
    //Ver que se necesita hacer para el BS

    } else {
        log_error(logger_memoria, "(? No se reconoce el algoritmo de memoria a implementar.");
    }
}

void compactar_memoria(){
	//Esto debería ser un hilo que periódicamente haga la compactación?
	if(string_equals_ignore_case(config_broker -> algoritmo_memoria, "BS")){

    } else if (string_equals_ignore_case(config_broker -> algoritmo_memoria, "PARTICIONES")){
        compactar_particiones_dinamicas();
    } else {
        log_error(logger, "??? No se reconoce el algoritmo de memoria.");
    }

}

void arrancar_memoria(){
    if(string_equals_ignore_case(config_broker -> algoritmo_memoria,"BS")){
        //ARMO LA ESTRUCTURA INICIAL DEL BS, SIN GUARDAR NADA TODAVIA. DESPUÉS CUANDO TENGA QUE
        //GUARDAR ALGO SE REUTILIZA LA ESTRUCTURA QUE SE DEFINIÓ ACÁ.
    }

    if(string_equals_ignore_case(config_broker -> algoritmo_memoria,"PARTICIONES")){
		t_list* memoria_con_particiones = list_create();
        iniciar_memoria_particiones(memoria_con_particiones);
        //Al principio la lista tiene un unico elemento que es la memoria entera.
	}
}


void guardar_en_memoria(t_mensaje* mensaje, void* mensaje_original){

	void* contenido; //= armar_contenido_de_mensaje(mensaje_original, mensaje -> codigo_operacion);


	if(string_equals_ignore_case(config_broker -> algoritmo_memoria,"BS")){
		uint32_t exponente = 0;
		if(mensaje -> tamanio_mensaje > config_broker -> size_min_memoria)
			exponente = obtenerPotenciaDe2(mensaje->tamanio_mensaje);
		else
			exponente = config_broker -> size_min_memoria;
      t_node* primer_nodo = (t_node*) memoria_cache->head->data;
      uint32_t pudoGuardarlo = recorrer_fifo(primer_nodo, exponente, mensaje -> payload);
	  if(!pudoGuardarlo)
	  {
		  log_error(logger,"no hay memoria suficiente para guardarlo");
	  }
	}


	if(string_equals_ignore_case(config_broker -> algoritmo_memoria, "PARTICIONES")){
	 t_memoria_dinamica* nueva_particion;
	 if(list_size(memoria_con_particiones) > 1){
          guardar_particion(mensaje, contenido);
     } else {
		  nueva_particion = armar_particion(mensaje -> tamanio_mensaje, 0, mensaje, 1);
    	  ubicar_particion(0, mensaje);
		  memcpy(memoria, contenido, mensaje -> tamanio_mensaje);
     }
	}

	free(contenido);

}

//--------------PARTICIONES-------------//


//ESTO ESTA PENSADO PARA PARTICIONES DINAMICAS, HAY QUE VER BS.
void reemplazar_particion_de_memoria(t_mensaje* mensaje, void* contenido_mensaje){
	//Revisar si necesita malloc.
    char* algoritmo_de_reemplazo = config_broker -> algoritmo_reemplazo;
	t_memoria_dinamica* particion_a_reemplazar;

    particion_a_reemplazar = seleccionar_particion_victima_de_reemplazo();

    uint32_t indice = encontrar_indice(particion_a_reemplazar);

    t_memoria_dinamica* particion_vacia = armar_particion(particion_a_reemplazar -> tamanio, particion_a_reemplazar -> base, NULL, 0);

    particion_a_reemplazar = list_replace(memoria_con_particiones, indice, particion_vacia);

    liberar_particion_en_cache(particion_a_reemplazar);
    log_info(logger, "... Se libera una partición y se intenta ubicar nuevamente el mensaje.");

    guardar_particion(mensaje, contenido_mensaje);
}


void liberar_particion_en_cache(t_memoria_dinamica* una_particion){

    uint32_t posicion_inicial_a_borrar = una_particion -> base;
    uint32_t limite                    = una_particion -> tamanio;


    log_info(logger, "El mensaje fue eliminado correctamente: %s", memoria + posicion_inicial_a_borrar);

    liberar_particion_dinamica(una_particion);
}


t_memoria_dinamica* seleccionar_particion_victima_de_reemplazo(){
	//Sabiendo que el payload es el contenido del mensaje incluido el id...
    t_memoria_dinamica* particion_victima;
    t_memoria_dinamica* particion_a_reemplazar;

    bool fue_cargada_primero(void* particion2){
        t_memoria_dinamica* una_particion = particion2;

		bool tiempo_de_carga_menor_o_igual(void* particion1){
        t_memoria_dinamica* otra_particion = particion1;
        return (otra_particion -> ultima_referencia) >= (una_particion -> tiempo_de_carga);
		//Se chequea que el tiempo de carga sea menor o igual.
    	}

		return list_all_satisfy(memoria_con_particiones, tiempo_de_carga_menor_o_igual);
    }


    bool fue_accedida_hace_mas_tiempo(void* particion4){
        t_memoria_dinamica* one_particion = particion4;

		bool tiempo_de_acceso_menor_o_igual(void* particion3){
        t_memoria_dinamica* particion = particion3;
        return (particion -> ultima_referencia) >= (one_particion -> ultima_referencia);
		//Se chequea el tiempo de acceso menor o igual.
    	}

        return list_all_satisfy(memoria_con_particiones, tiempo_de_acceso_menor_o_igual);
    }

    if(string_equals_ignore_case(config_broker ->algoritmo_reemplazo, "FIFO")){
		particion_victima = list_find(memoria_con_particiones, fue_cargada_primero);
	} else if (string_equals_ignore_case(config_broker ->algoritmo_reemplazo, "LRU")){
    	particion_a_reemplazar = list_find(memoria_con_particiones, fue_accedida_hace_mas_tiempo);
	}

    return particion_victima;
}

uint32_t obtener_id(t_memoria_dinamica* particion){
    uint32_t id = 0;
    t_mensaje* mensaje = encontrar_mensaje(particion -> base, particion -> codigo_operacion);
    id = mensaje -> id_mensaje;
    return id;
}

t_memoria_dinamica* encontrar_mensaje(uint32_t base_de_la_particion_del_mensaje, op_code codigo){

    t_mensaje* un_mensaje;

    bool tiene_la_misma_base(void* un_mensaje){
        t_mensaje* msj = un_mensaje;
        t_memoria_dinamica* particion = msj -> payload;
        return particion -> base == base_de_la_particion_del_mensaje;
    }

    switch(codigo){
        case GET_POKEMON:
			un_mensaje = list_find(cola_get, tiene_la_misma_base);
			break;
		case CATCH_POKEMON:
			un_mensaje = list_find(cola_catch, tiene_la_misma_base);
			break;
		case LOCALIZED_POKEMON:
			un_mensaje = list_find(cola_localized, tiene_la_misma_base);
			break;
		case CAUGHT_POKEMON:
			un_mensaje = list_find(cola_caught, tiene_la_misma_base);
			break;
		case APPEARED_POKEMON:
			un_mensaje = list_find(cola_appeared, tiene_la_misma_base);
			break;
		case NEW_POKEMON:
			un_mensaje = list_find(cola_new, tiene_la_misma_base);
		    break;
		default:
            log_error(logger, "...No se puede reconocer el mensaje de esta partición.");
            break;
    }
    return un_mensaje;
}

void iniciar_memoria_particiones(t_list* memoria_de_particiones){
    /*tamanio_mensaje+payload+base+ocupado+base*/
    //Es la particion inicial, es decir, la memoria entera vacía.
    t_memoria_dinamica* particion_de_memoria = armar_particion((config_broker->size_memoria), 0, NULL, 0);
    list_add(memoria_de_particiones, particion_de_memoria);
}

//DEBERIA ARMAR UNA PARTICION PARA GUARDAR ANTES O CHEQUEARLO A PARTIR DEL MENSAJE?
void guardar_particion(t_mensaje* un_mensaje, void* contenido_mensaje){
    uint32_t posicion_a_ubicar = 0;
	t_memoria_dinamica* particion_a_ubicar;
	t_memoria_dinamica* nueva_particion;

    if(!chequear_espacio_memoria_particiones(un_mensaje -> tamanio_mensaje)){
			log_error(logger, "no hay memoria");
            reemplazar_particion_de_memoria(un_mensaje, contenido_mensaje);
            //La idea es que se llama recursivamente ya que se liberan particiones hasta que haya suficiente espacio
            //contiguo para almacenar.
	}

    if(string_equals_ignore_case(config_broker -> algoritmo_particion_libre, "FF")){
        posicion_a_ubicar = encontrar_primer_ajuste(un_mensaje -> tamanio_mensaje);

        if(posicion_a_ubicar == -1){
            log_error(logger, "No hay suficiente tamaño para ubicar el mensaje en memoria.");
            reemplazar_particion_de_memoria(un_mensaje, contenido_mensaje);
        }

		particion_a_ubicar = list_get(memoria_con_particiones, posicion_a_ubicar);

		nueva_particion = armar_particion(un_mensaje -> tamanio_mensaje, particion_a_ubicar -> base, un_mensaje, 1);
        ubicar_particion(posicion_a_ubicar, nueva_particion);
		guardar_contenido_de_mensaje(nueva_particion -> base, contenido_mensaje, nueva_particion -> tamanio);
    }

    if(string_equals_ignore_case(config_broker -> algoritmo_particion_libre,"BF")){
        posicion_a_ubicar = encontrar_mejor_ajuste(un_mensaje -> tamanio_mensaje);

        if(posicion_a_ubicar == -1){
            log_error(logger, "No hay suficiente tamaño para ubicar el mensaje en memoria.");
            reemplazar_particion_de_memoria(un_mensaje, contenido_mensaje);
        }

		particion_a_ubicar = list_get(memoria_con_particiones, posicion_a_ubicar);

		nueva_particion = armar_particion(un_mensaje -> tamanio_mensaje, particion_a_ubicar -> base, un_mensaje, 1);
		ubicar_particion(posicion_a_ubicar, nueva_particion);
		guardar_contenido_de_mensaje(nueva_particion -> base, contenido_mensaje, nueva_particion -> tamanio);


    }

}

void guardar_contenido_de_mensaje(uint32_t offset, void* contenido, uint32_t tamanio){
	memcpy(memoria + offset, contenido, tamanio);
	//Revisar el free.
	free(contenido);
}

void liberar_particion_dinamica(t_memoria_dinamica* particion_vacia){
    free(particion_vacia);
}

uint32_t chequear_espacio_memoria_particiones(uint32_t tamanio_mensaje){
	uint32_t tamanio_ocupado = 0;
	void sumar(void* particion){
		t_memoria_dinamica* una_particion = particion;
		if(una_particion->ocupado != 0)
		tamanio_ocupado += una_particion -> tamanio;
	}
	list_iterate(memoria_con_particiones, sumar);
	if(tamanio_ocupado == config_broker -> size_memoria || tamanio_ocupado + tamanio_mensaje >= config_broker -> size_memoria){
		return 0;
	}
	return tamanio_ocupado;

}
// EN ubicar_particion FALTA UBICAR EN LA MEMORIA DEL MALLOC ENORME EL MENSAJE COMO CORRESPONDE.
void ubicar_particion(uint32_t posicion_a_ubicar, t_memoria_dinamica* particion){

        t_memoria_dinamica* particion_reemplazada = list_replace(memoria_con_particiones, posicion_a_ubicar, particion);

        uint32_t total = particion_reemplazada -> tamanio;
        if(particion -> tamanio < total) {
        uint32_t nueva_base = (particion_reemplazada -> base) + particion->tamanio + 1;
        t_memoria_dinamica* nueva_particion_vacia = armar_particion(total - particion ->tamanio, nueva_base, NULL, 0);
        list_add_in_index(memoria_con_particiones, posicion_a_ubicar + 1, nueva_particion_vacia);
		memcpy((memoria + nueva_base), NULL, (total - particion -> tamanio));
        }
        log_info(logger, "Se guardo en la particion.");
        liberar_particion_dinamica(particion_reemplazada);
}

t_memoria_dinamica* armar_particion(uint32_t tamanio, uint32_t base, t_mensaje* mensaje, uint32_t ocupacion){

	t_memoria_dinamica* nueva_particion = malloc(sizeof(t_memoria_dinamica));

	if(mensaje != NULL){
		nueva_particion = (t_memoria_dinamica*) mensaje->payload;
		nueva_particion-> tamanio = tamanio;
		nueva_particion-> base = base;
		nueva_particion-> ocupado = ocupacion;
		nueva_particion-> codigo_operacion = mensaje->codigo_operacion;
    	nueva_particion = mensaje -> payload;
    } else {
		nueva_particion -> tamanio = tamanio;
		nueva_particion -> base = base;
		nueva_particion -> ocupado = 0;
		nueva_particion -> codigo_operacion = 0;
	}

    return nueva_particion;
}

uint32_t encontrar_primer_ajuste(uint32_t tamanio){
    uint32_t indice_seleccionado = 0;

    bool tiene_tamanio_suficiente(void* particion){
        t_memoria_dinamica* una_particion = particion;
        return tamanio <= (una_particion -> tamanio) && una_particion -> ocupado == 0;
    }

    t_memoria_dinamica* posible_particion = list_find(memoria_con_particiones, tiene_tamanio_suficiente);
    indice_seleccionado = encontrar_indice(posible_particion);

    return indice_seleccionado;
}

uint32_t encontrar_mejor_ajuste(uint32_t tamanio){
    uint32_t indice_seleccionado = 0;

    bool es_de_menor_tamanio(void* una_particion, void* otra_particion){
        t_memoria_dinamica* particion1 = una_particion;
        t_memoria_dinamica* particion2 = otra_particion;
        return (particion1 -> tamanio) < (particion2 -> tamanio);
    }

    bool tiene_tamanio_suficiente(void* particion){
        t_memoria_dinamica* una_particion = particion;
        return tamanio <= (una_particion -> tamanio) && una_particion -> ocupado == 0 ;
    }

    t_list* particiones_en_orden_creciente = list_duplicate(memoria_con_particiones);
    list_sort(particiones_en_orden_creciente, es_de_menor_tamanio);

    t_memoria_dinamica* posible_particion = list_find(particiones_en_orden_creciente, tiene_tamanio_suficiente);
    indice_seleccionado = encontrar_indice(posible_particion);
    //REVISAR ESTO
    list_destroy(particiones_en_orden_creciente);

    return indice_seleccionado;
}

void destruir_particion(void* una_particion){
   t_memoria_dinamica* particion = una_particion;
   free(particion);
   // free(una_particion);
}

uint32_t encontrar_indice(t_memoria_dinamica* posible_particion){
    uint32_t indice_disponible = 0;
    uint32_t indice_buscador = -1;
    void* obtener_indices(void* particion){
    	indice_buscador += 1;
        t_memoria_dinamica* particion_a_transformar = particion;
        t_indice* un_indice = malloc(sizeof(t_indice));
        un_indice -> indice = indice_buscador;
        un_indice -> tamanio = particion_a_transformar -> tamanio;
        return un_indice;
    }

    bool es_el_tamanio_necesario(void* indice){
        t_indice* otro_indice = indice;
        return (otro_indice -> tamanio) == (posible_particion -> tamanio);
    }

    t_list* indices = list_create();
    indices = list_map(memoria_con_particiones, obtener_indices);
    t_indice* indice_elegido = list_find(indices, es_el_tamanio_necesario);
    //FIJARSE SI ES UN DESTROY A LOS ELEMENTOS TAMBIEN.
    list_destroy(indices);
    indice_disponible = indice_elegido -> indice;
    //free(indice_elegido;)

    return indice_disponible;
}

//--------------------------------CONSOLIDACION_P--------------------------------//
//ESTO SIRVE PARA LA LISTA QUE SIMULA LA MEMORIA, PERO FALTA ADAPTARLO PARA QUE HAGA LO MISMO EN EL MALLOC
//ENORME.

void consolidar_particiones_dinamicas(){

    void consolidar_particiones_contiguas(void* particion){
        t_memoria_dinamica* una_particion = particion;
        uint32_t indice = encontrar_indice(una_particion);
        if(tiene_siguiente(indice) && ambas_estan_vacias(indice, indice + 1)){
            //Siempre asumo que es consolidable porque tiene un valor a la derecha que también es vacío.
            consolidar_particiones(indice, indice + 1);
        }
    }

    list_iterate(memoria_con_particiones, consolidar_particiones_contiguas);

    if(existen_particiones_contiguas_vacias(memoria_con_particiones)){
            consolidar_particiones_dinamicas();
    }
    //Tendría que llamar recursivamente? --> revisar
}

bool tiene_siguiente(uint32_t posicion){
    return list_get(memoria_con_particiones, posicion + 1) != NULL;
}

bool ambas_estan_vacias(uint32_t una_posicion, uint32_t posicion_siguiente){

    t_memoria_dinamica* una_particion       = list_get(memoria_con_particiones, una_posicion);
    t_memoria_dinamica* particion_siguiente = list_get(memoria_con_particiones, posicion_siguiente);
    return !((una_particion -> ocupado) && (particion_siguiente -> ocupado));
}

void consolidar_particiones(uint32_t primer_elemento, uint32_t elemento_siguiente){
    t_memoria_dinamica* una_particion = list_remove(memoria_con_particiones, primer_elemento);
    t_memoria_dinamica* particion_siguiente = list_remove(memoria_con_particiones, elemento_siguiente);

    uint32_t tamanio_particion_consolidada    = (una_particion -> tamanio) + (particion_siguiente -> tamanio);
    t_memoria_dinamica* particion_consolidada = armar_particion(tamanio_particion_consolidada, (una_particion -> base), NULL, 0);

    list_add_in_index(memoria_con_particiones, primer_elemento, particion_consolidada);

    //Esto consolida las particiones en la "memoria". Quizás se podría sacar ya que ahí no se distinguen particiones.
	guardar_contenido_de_mensaje((una_particion -> base), NULL, tamanio_particion_consolidada);
    destruir_particion(una_particion);
    destruir_particion(particion_siguiente);
}

bool existen_particiones_contiguas_vacias(t_list* memoria_cache){
    //Si el primer elemento de cada lista está vacío, hay una partición a consolidar.
    t_list* memoria_duplicada = list_duplicate(memoria_cache);

    bool la_posicion_siguiente_tambien_esta_vacia(void* una_particion){
        t_memoria_dinamica* particion = una_particion;

        uint32_t indice = encontrar_indice(particion);
        t_memoria_dinamica* sig_particion = list_get(memoria_duplicada, indice+1);;
        uint32_t sig_ocupado;

        if(sig_particion != NULL){
            sig_ocupado =  sig_particion -> ocupado;
        } else {
            sig_ocupado = 1;
        }

        return !((particion -> ocupado) && sig_ocupado);
    }

    return list_any_satisfy(memoria_duplicada, la_posicion_siguiente_tambien_esta_vacia);
}


//---------------------------------COMPACTACION_P---------------------------------//

void compactar_particiones_dinamicas(){

    t_list* particiones_vacias = list_create();
    t_list* particiones_ocupadas = list_create();

    bool es_particion_vacia(void* particion){
        t_memoria_dinamica* una_particion = particion;
        return (una_particion -> ocupado) == 0;
    }

    bool no_es_particion_vacia(void* particion){
        t_memoria_dinamica* una_particion = particion;
        return (una_particion -> ocupado) != 0;
    }

    particiones_vacias = list_filter(memoria_con_particiones, es_particion_vacia);


    void actualizar_base(void* particion_ocupada){
		t_memoria_dinamica* particion_actualizada = particion_ocupada;
		uint32_t base_de_particion;

	}

	particiones_ocupadas = list_filter(memoria_con_particiones, no_es_particion_vacia);
	list_iterate(particiones_ocupadas, actualizar_base);

    //list_clean(memoria_con_particiones);
    list_clean_and_destroy_elements(memoria_con_particiones, destruir_particion); //-->REVISAR
    //Se tiene que actualizar en cada particion la base.
    //Actualizar los mensajes con las particiones.
    list_add_all(memoria_con_particiones, particiones_vacias);
	list_add_all(memoria_con_particiones, particiones_ocupadas);

    //En esta función se hace la compactación realmente en el malloc enorme (o eso espero).
    compactar_memoria_cache(particiones_ocupadas);

    consolidar_particiones_dinamicas();

	uint32_t tamanio_vacio = 0;//obtener_tamanio_vacio(particiones_vacias);
	guardar_contenido_de_mensaje(0, NULL, tamanio_vacio);


    /*list_destroy(particiones_vacias);
    list_destroy(particiones_ocupadas);*/
    list_destroy_and_destroy_elements(particiones_vacias, destruir_particion);
    list_destroy_and_destroy_elements(particiones_ocupadas, destruir_particion);

    particiones_liberadas = 0;
}


void compactar_memoria_cache(t_list* lista_particiones_ocupadas){
    //En este momento el puntero tiene que estar en la primer posición de la memoria cache.
    void reescribir_memoria(void* particion){
        t_memoria_dinamica* una_particion = particion;
        //guardar_contenido_de_mensaje();
		memcpy(memoria + (una_particion -> base), NULL , (una_particion -> tamanio));
    }
    //Preguntar por la eficiencia de esto.
   // list_iterate(lista_particiones_ocupadas, reescribir_memoria);
}

//ELIMINAR MENSAJE --> JULI


void eliminar_mensaje(void* mensaje){
	t_mensaje* un_mensaje = mensaje;
    liberar_mensaje_de_memoria(un_mensaje);
	list_destroy(un_mensaje -> suscriptor_enviado);
	list_destroy(un_mensaje -> suscriptor_recibido);
	free(un_mensaje -> payload);
	free(un_mensaje);
	free((t_mensaje*) mensaje);
}

void liberar_mensaje_de_memoria(t_mensaje* mensaje){
	t_memoria_dinamica* particion_buscada = mensaje -> payload;

   	bool es_la_particion(void* particion){
		t_memoria_dinamica* una_particion = particion;
		return (una_particion -> base) == (particion_buscada -> base);
	}

    t_memoria_dinamica* particion_a_liberar = list_find(memoria_con_particiones, es_la_particion);

    uint32_t indice = encontrar_indice(particion_a_liberar);
    t_memoria_dinamica* particion_vacia = armar_particion(particion_a_liberar -> tamanio, particion_a_liberar -> base, NULL, 0);

    particion_a_liberar = list_replace(memoria_con_particiones, indice, particion_vacia);
    liberar_particion_en_cache(particion_a_liberar);

    particiones_liberadas++;
    //REVISAR --> JULI (semáforo)
	log_info(logger, "... Se libera una partición.");

    if(particiones_liberadas == config_broker -> frecuencia_compactacion){
        compactar_memoria();
    } else if (particiones_liberadas > config_broker -> frecuencia_compactacion) {
        log_error(logger, "...Debería haberse compactado la memoria :$.");
        exit(-90);
    }
}

void dump_info_particion(void* particion){
    t_memoria_dinamica* una_particion = particion;
    char* ocupado;

    if(una_particion -> ocupado != 0) {
        ocupado = "X";
    } else {
        ocupado = "L";
    }
    uint32_t base = memoria + (una_particion -> base);//Revisar que apunte al malloc
	uint32_t limite = base + (una_particion -> tamanio);
    uint32_t tamanio = una_particion -> tamanio;
    uint32_t valor_lru = una_particion -> ultima_referencia;
    //Relacionar al mensaje con la partición
    char* cola_del_mensaje = obtener_cola_del_mensaje(una_particion);
    uint32_t id_del_mensaje = obtener_id(una_particion);

    //Revisar el tema de la dirección de memoria para loggear-->(&base?).
    log_info(logger_memoria, "Particion %d: %d - %d.  [%s] Size: %d b LRU:%d Cola:%s ID:%d", numero_particion, base, limite, ocupado, tamanio, valor_lru, cola_del_mensaje, id_del_mensaje);
    numero_particion++;
    free(cola_del_mensaje);
    //Revisar si hay que liberar la partición!
}

char* obtener_cola_del_mensaje(t_memoria_dinamica* una_particion){
    char* una_cola;
    switch(una_particion -> codigo_operacion){
        case GET_POKEMON:
        una_cola = malloc(strlen("COLA_GET") + 1);
        una_cola = "COLA_GET";
        break;
        case CATCH_POKEMON:
        una_cola = malloc(strlen("COLA_CATCH") + 1);
        una_cola = "COLA_CATCH";
        break;
        case LOCALIZED_POKEMON:
        una_cola = malloc(strlen("COLA_LOCALIZED") + 1);
        una_cola = "COLA_LOCALIZED";
        break;
        case APPEARED_POKEMON:
        una_cola = malloc(strlen("COLA_APPEARED") + 1);
        una_cola = "COLA_APPEARED";
        break;
        case CAUGHT_POKEMON:
        una_cola = malloc(strlen("COLA_CAUGHT") + 1);
        una_cola = "COLA_CAUGHT";
        break;
        case NEW_POKEMON:
        una_cola = malloc(strlen("COLA_NEW") + 1);
        una_cola = "COLA_NEW";
        break;
        default:
        una_cola = malloc(strlen("No existe en ninguna cola de mensajes") + 1);
        una_cola = "No existe en ninguna cola de mensajes";
        break;
    }

    return una_cola;
}

uint64_t timestamp(void) {
	struct timeval valor;
	gettimeofday(&valor, NULL);
	unsigned long long result = (((unsigned long long )valor.tv_sec) * 1000 + ((unsigned long) valor.tv_usec));
	uint64_t tiempo = result;
	return tiempo;
}

//-------------BUDDY_SYSTEM-------------//

uint32_t obtenerPotenciaDe2(uint32_t tamanio_proceso)
{
	uint32_t contador = 0 ;
	while((2^contador) <= tamanio_proceso){
      contador ++;
	}
	return (2^contador) ;
}


t_node* crear_nodo(uint32_t tamanio)
{
  // Allocate memory for new node
  t_node* node = (t_node*)malloc(sizeof(t_node));

  // Assign data to this node
  node -> bloque = malloc(sizeof(t_memoria_buddy));
  node -> bloque -> tamanio = tamanio;
  node -> bloque -> libre = 1;

  // Initialize left and right children as NULL
  node -> izquierda = NULL;
  node -> derecha = NULL;
  return(node);
}


void arrancar_buddy(){
	memoria_cache = list_create();
	t_node* root = crear_nodo(config_broker -> size_memoria);
	list_add(memoria_cache, root);
}

void asignar_nodo(t_node* node,void* payload){
    node ->  bloque -> libre = 0;
    list_add(memoria_cache, node);
}

uint32_t recorrer_fifo(t_node* nodo, uint32_t exponente, void* payload){
    if(nodo == NULL || nodo->bloque->tamanio < config_broker -> size_min_memoria) {
        return 0;
    }
    asignado  = 0;
    if (nodo->bloque->tamanio >= exponente && nodo->bloque->libre == 1) {
        asignar_nodo(nodo, payload);
        return 1;
    }

    if (nodo -> izquierda == NULL) {
        nodo -> izquierda = crear_nodo(nodo -> bloque -> tamanio / 2);
    }

    if (nodo -> izquierda -> bloque -> tamanio > exponente) {
      recorrer_fifo(nodo->izquierda, exponente,payload);
    }

    asignado = recorrer_fifo(nodo -> izquierda, exponente, payload);
    if(asignado == 0) {
    	if (nodo -> derecha == NULL) {
    	        nodo -> derecha = crear_nodo(nodo -> bloque -> tamanio / 2);
    	    }
        asignado = recorrer_fifo(nodo -> derecha,exponente, payload);
    } else {
    	return 1;
    }
    log_info(logger,"paso por recorrer, varias veces");
    return asignado;
}

uint32_t recorrer_best_fit(t_node* nodo, uint32_t exponente, void* payload){
    if(nodo == NULL || nodo->bloque->tamanio < config_broker -> size_min_memoria) {
        return 0;
    }
    asignado  = 0;
    if (nodo->bloque->tamanio == exponente && nodo->bloque->libre == 1) {
        asignar_nodo(nodo, payload);
        return 1;
    }

    if (nodo -> izquierda == NULL) {
        nodo -> izquierda = crear_nodo(nodo -> bloque -> tamanio / 2);
    }

    if (nodo -> izquierda -> bloque -> tamanio > exponente) {
      recorrer_best_fit(nodo->izquierda, exponente,payload);
    }

    asignado = recorrer_best_fit(nodo -> izquierda, exponente, payload);
    if(asignado == 0) {
    	if (nodo -> derecha == NULL) {
    	        nodo -> derecha = crear_nodo(nodo -> bloque -> tamanio / 2);
    	    }
        asignado = recorrer_best_fit(nodo -> derecha,exponente, payload);
    } else {
    	return 1;
    }
    log_info(logger,"paso por recorrer, varias veces");
    return asignado;
}


void concatenacion_buddy_systeam(t_node*  nodo)
{

      if(nodo== NULL)
      {
          return;
      }
      if(nodo -> izquierda -> bloque-> libre && nodo -> derecha-> bloque-> libre && nodo)
      {
          nodo-> bloque -> libre = 1;
          nodo->izquierda =NULL;
          nodo->derecha =NULL;
          //concatenacion_buddy_systeam();///principop el de 4096
      }
      if(nodo -> izquierda)
      {
          concatenacion_buddy_systeam(nodo->izquierda);
      }
      if(nodo-> derecha)
      {
          concatenacion_buddy_systeam(nodo->derecha);
      }
}



/*void crear_hilo_segun_algoritmo() {
	if(string_equals_ignore_case(config_broker -> algoritmo_memoria, "BS") ||
			string_equals_ignore_case(config_broker -> algoritmo_memoria, "PARTICIONES")){
		uint32_t err = pthread_create(&hilo_algoritmo_memoria, NULL, reservar_memoria, NULL);
		if(err != 0) {
			log_error(logger, "El hilo no pudo ser creado!!"); // preguntar si estos logs se pueden hacer
		}
	} else {
		log_error(logger, "wtf?? Algoritmo de memoria recibido: %s", config_broker -> algoritmo_memoria);
	}
}
void crear_hilo_por_mensaje() {
	uint32_t err = pthread_create(&hilo_mensaje, NULL, gestionar_mensaje, NULL);
	if(err != 0) {
		log_error(logger, "El hilo no pudo ser creado!!");
	}
}*/

void gestionar_mensaje(){
    //ACA HAY QUE METER EL PROCESS REQUEST Y DEMÁS
}

void iniciar_semaforos_broker() {
	//REVISAR INICIALIZCIONES
	sem_init(&semaforo, 0, 1);
	sem_init(&mutex_id, 0, 1);
    sem_init(&mx_cola_get, 0, 1);
	sem_init(&mx_cola_catch, 0, 1);
	sem_init(&mx_cola_localized, 0, 1);
	sem_init(&mx_cola_caught, 0, 1);
    sem_init(&mx_cola_appeared, 0, 1);
    sem_init(&mx_cola_new, 0, 1);
    sem_init(&mx_suscrip_get, 0, 1);
	sem_init(&mx_suscrip_catch, 0, 1);
	sem_init(&mx_suscrip_localized, 0, 1);
	sem_init(&mx_suscrip_caught, 0, 1);
    sem_init(&mx_suscrip_appeared, 0, 1);
    sem_init(&mx_suscrip_new, 0, 1);
    sem_init(&mx_memoria_cache, 0, 1);
    sem_init(&mx_copia_memoria, 0, 1);


}

void terminar_hilos_broker(){
    pthread_detach(hilo_algoritmo_memoria);
    pthread_detach(hilo_mensaje);
}

void liberar_semaforos_broker(){
    sem_destroy(&mx_cola_get);
    sem_destroy(&mx_cola_catch);
    sem_destroy(&mx_cola_localized);
    sem_destroy(&mx_cola_caught);
    sem_destroy(&mx_cola_appeared);
    sem_destroy(&mx_cola_new);
    sem_destroy(&mx_suscrip_get);
    sem_destroy(&mx_suscrip_catch);
    sem_destroy(&mx_suscrip_localized);
    sem_destroy(&mx_suscrip_caught);
    sem_destroy(&mx_suscrip_appeared);
    sem_destroy(&mx_suscrip_new);
    sem_destroy(&mx_memoria_cache);
    sem_destroy(&mx_copia_memoria);
    sem_destroy(&semaforo);
    sem_destroy(&mutex_id);
}

