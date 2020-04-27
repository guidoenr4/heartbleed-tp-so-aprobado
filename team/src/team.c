#include "team.h"

int main(void) {

	iniciar_programa();
	int socket = crear_conexion(config -> ip_broker, config -> puerto_broker);
	enviar_mensaje(TE_GET_POKEMON_BR, "Get Pokemon", socket); // se va

	//t_buffer* recibido = recibir_mensaje(socket, strlen("Hola")+ 1);
	log_info(logger, "El ip es : %s", config -> ip_broker);
	log_info(logger, "El port es : %s ", config -> puerto_broker);
	terminar_programa(socket);

	return 0;
}

void iniciar_programa(){
	iniciar_logger("team.log", "team");
	leer_config(); // aca agregamos los elementos a los campos q corresponde
	//objetivo_global = obtener_objetivo_global();
	//crear_hilos_entrenadores(); // iniciar a los entrenadores
	//iniciar_conexion(); abrir socket con el gameBoy (pthread_create)

}

/*t_list* obtener_objetivo_global(){ // usar el iterate

// tener en cuenta que necesitamos saber la especie y cantidad de cada uno

	t_list* objetivos;
	t_list* aux_lista_lista = config -> objetivos_entrenadores;
	t_list* aux_lista = aux_lista_lista -> pokemons;

	while(aux_lista_lista -> next != NULL){
		while(aux_lista -> next != NULL){

			objetivos -> pokemon = aux_lista -> pokemon;
			objetivos -> next = aux_lista -> next;
			aux_lista = aux_lista -> next;
		}

		aux_lista_lista = aux_lista_lista -> next;
	}

	return objetivos;

}*/

void leer_config(void) {

	config = malloc(sizeof(t_config_team));

	t_config* config_team = config_create("Debug/team.config");

	char** posiciones = config_get_array_value(config_team, "POSICIONES_ENTRENADORES");
	char** pokemons = config_get_array_value(config_team, "POKEMON_ENTRENADORES");
	char** objetivos = config_get_array_value(config_team, "OBJETIVOS_ENTRENADORES");

	t_list* lista_posiciones = parsear(posiciones);
	t_list* lista_pokemons = parsear(pokemons);
	t_list* lista_objetivos = parsear(objetivos);



	config -> tiempo_reconexion = config_get_int_value(config_team, "TIEMPO_RECONEXION");
	config -> retardo_cpu = config_get_int_value(config_team, "RETARDO_CICLO_CPU");
	config -> algoritmo_planificacion = config_get_string_value(config_team, "ALGORITMO_PLANIFICACION");
	config -> ip_broker = config_get_string_value(config_team, "IP_BROKER");
	config -> puerto_broker = config_get_string_value(config_team, "PUERTO_BROKER");
	config -> estimacion_inicial = config_get_int_value(config_team, "ESTIMACION_INICIAL");
	config -> log_file = config_get_string_value(config_team, "LOG_FILE");

	config_destroy(config_team);

}
/*
void parsear_dato(char* cadena){
	char** palabra;
	palabra = string_split(cadena,"|");
	//agregar_a_entrenador(palabra);
}

void parsear(char** datos_a_parsear){
	string_iterate_lines(datos_a_parsear,parsear_dato);

}
*/

void* parsear(char** datos_de_config) { // no se si void o q retorne lo parseado y asignarlo al struct en leer_config
	t_list* lista = list_create();
	t_list* lista_lista = list_create();
	printf("%d", sizeof(datos_de_config));
	char e;
	char* palabra = "";
	for (char* c = *datos_de_config; c; c=*++datos_de_config) {
		for (char* d = c; d; d++) {
			e = *d;

			if(e != '|' && e) {
				palabra = append(palabra, e);
			} else {
				list_add(lista, palabra);
				palabra = ""; // limpiar char *
			}
			if(!e){
				break;
			}
		}
		t_list* lista_aux = list_duplicate(lista);
		list_add(lista_lista, lista_aux);
		list_clean(lista);
	}
	return lista_lista;
}

char* append(const char *s, char c) {
    int len = strlen(s);
    char buf[len+2];
    strcpy(buf, s);
    buf[len] = c;
    buf[len + 1] = 0;
    return strdup(buf);
}

void concatenar(char* palabra, char caracter) {
	int largo = string_length(palabra);
	palabra[largo] = caracter;
}
/*
 *
void agregar_a_entrenador(char ** palabra){

}
 *
 *
void crear_hilos_entrenadores(){

	while(posiciones_entrenadores != NULL){ // o iterate si se puede
		int err = pthread_create(hilo, NULL, iniciar_entrenador, entrenador);
		if(id =! 0){
			el hilo se creo mal
			quizas retornar err para tratar el error con lo de las commons
		}
	}
}*/

void load_entrenadores(t_list* entrenadores) {
	t_link_element *element = entrenadores -> head;
	t_link_element *aux = NULL;
	while (element != NULL) {

		aux = element -> next;

		element = aux;
	}
}

void liberar_config() {
	free(config -> algoritmo_planificacion);
	free(config -> log_file);
	free(config -> ip_broker);
	free(config -> puerto_broker);
	free(config);
}

void terminar_programa(int conexion) {
	liberar_config();
	liberar_logger();
	liberar_conexion(conexion);
}
