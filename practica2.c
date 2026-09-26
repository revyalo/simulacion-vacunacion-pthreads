#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

#ifndef CENTROS
#define CENTROS 5  // Numero de centros de vacunacion
#endif
#ifndef FABRICAS
#define FABRICAS 3 // Numero de fabricas
#endif
#ifndef TANDAS
#define TANDAS 10  // Numero de tandas de vacunacion
#endif

#if CENTROS < 1 || FABRICAS < 1 || TANDAS < 1
#error "CENTROS, FABRICAS y TANDAS deben ser mayores que cero"
#endif

// Estructura para almacenar la configuracion
typedef struct{
    int habitantes_totales;
    int vacunas_iniciales;
    int minimo_vacunas;
    int max_vacunas;
    int tiempo_min_fabrica;
    int tiempo_max_fabrica;
    int tiempo_max_reparto;
    int tiempo_max_cita;
    int tiempo_max_desplazamiento;

}Configuracion_t;

// Estructura para representar un centro de vacunacion

typedef struct{
    int id;
    int stock_actual;
    int demanda_pendiente;
    int habitantes_asignados;
    int total_vacunas_recibidas;
    int total_vacunados;
    pthread_mutex_t mutexCentro;
    pthread_cond_t condicion_vacunas;
}Centro_t;

// Estructura para representar una fabrica

typedef struct{
    int id;
    int vacunas_fabricas;
    int vacunas_asignadas;
    int vacunas_entregadas_por_centro[CENTROS];

}Fabrica_t;

// Representacion de un habitante

typedef struct{
    int id;
    int centro_id;
}Habitante_t;

// Variables globales
Configuracion_t configuracion;
Centro_t centro[CENTROS];
Fabrica_t fabrica[FABRICAS];

const char *fichero_entrada = "entrada.txt";
const char *fichero_salida = "salida.txt";
FILE *salida = NULL;

pthread_mutex_t mutexSalida = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexAleatorio = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexReparto = PTHREAD_MUTEX_INITIALIZER;

// Declaracion de funciones
int inicio(int argc, char *argv[]);
void *fabricas(void *arg);
void *habitantes(void *arg);
void estadisticas(void);
void registrar_evento(const char *formato, ...);
int aleatorio_entre(int minimo, int maximo);
void dormir_aleatorio(int minimo, int maximo);
void repartir_vacunas(Fabrica_t *fabrica, int vacunas);
void comprobar_pthread(int resultado, const char *operacion);


int main(int argc, char *argv[]){
    int i;
    int j;
    int vacunasBase;
    int vacunasExtra;
    int habitantesBase;
    int habitantesExtra;
    int habitantesTandaActual;
    int maxHabitantesPorTanda;
    int habitantesCreados;
    int habitantesCreadosTanda;
    pthread_t *hilosFabricas;
    pthread_t *hilosHabitantes;
    Habitante_t *argsHabitantes;
    int tanda;
    int indice;
    int error;

    srand((unsigned int)time(NULL));

    if(inicio(argc, argv) != 0){
        return 1;
    }

    for(i = 0; i < CENTROS; i++){
        centro[i].id = i + 1;
        centro[i].stock_actual = configuracion.vacunas_iniciales;
        centro[i].demanda_pendiente = 0;
        centro[i].habitantes_asignados = 0;
        centro[i].total_vacunas_recibidas = 0;
        centro[i].total_vacunados = 0;

        comprobar_pthread(pthread_mutex_init(&centro[i].mutexCentro, NULL), "pthread_mutex_init");
        comprobar_pthread(pthread_cond_init(&centro[i].condicion_vacunas, NULL), "pthread_cond_init");
    }

    argsHabitantes = malloc(sizeof(Habitante_t) * configuracion.habitantes_totales);
    if(argsHabitantes == NULL){
        perror("Error reservando memoria para los datos de habitantes");
        fclose(salida);
        return 1;
    }

    for(i = 0; i < configuracion.habitantes_totales; i++){
        int centroAsignado = aleatorio_entre(0, CENTROS - 1);

        argsHabitantes[i].id = i + 1;
        argsHabitantes[i].centro_id = centroAsignado;
        centro[centroAsignado].habitantes_asignados++;
        centro[centroAsignado].demanda_pendiente++;
    }

    hilosFabricas = malloc(sizeof(pthread_t) * FABRICAS);
    if(hilosFabricas == NULL){
        perror("Error reservando memoria para los hilos de las fabricas");
        free(argsHabitantes);
        fclose(salida);
        return 1;
    }

    vacunasBase = configuracion.habitantes_totales / FABRICAS;
    vacunasExtra = configuracion.habitantes_totales % FABRICAS;

    for (i = 0; i < FABRICAS; i++)
    {
        fabrica[i].id = i + 1;
        fabrica[i].vacunas_asignadas = vacunasBase + (i < vacunasExtra ? 1 : 0);
        fabrica[i].vacunas_fabricas = fabrica[i].vacunas_asignadas;
        for ( j = 0; j < CENTROS; j++)
        {
            fabrica[i].vacunas_entregadas_por_centro[j] = 0;
        }
    }

    error = 0;
    for ( i = 0; i < FABRICAS; i++)
    {
        int resultado = pthread_create(&hilosFabricas[i], NULL, fabricas, (void*)&fabrica[i]);
        if(resultado != 0){
            fprintf(stderr, "Error creando el hilo de la fabrica %d: %s\n", i + 1, strerror(resultado));
            error = 1;
            break;
        }
    }

    if(error){
        for ( j = 0; j < i; j++)
        {
            comprobar_pthread(pthread_join(hilosFabricas[j], NULL), "pthread_join fabrica");
        }
        free(hilosFabricas);
        free(argsHabitantes);
        fclose(salida);
        return 1;
    }

    habitantesBase = configuracion.habitantes_totales / TANDAS;
    habitantesExtra = configuracion.habitantes_totales % TANDAS;
    maxHabitantesPorTanda = habitantesBase + (habitantesExtra > 0 ? 1 : 0);
    habitantesCreados = 0;

    hilosHabitantes = malloc(sizeof(pthread_t) * maxHabitantesPorTanda);

    if(hilosHabitantes == NULL){
        perror("Error reservando memoria para los hilos de habitantes");
        for ( i = 0; i < FABRICAS; i++)
        {
            comprobar_pthread(pthread_join(hilosFabricas[i], NULL), "pthread_join fabrica");
        }
        free(hilosFabricas);
        free(hilosHabitantes);
        free(argsHabitantes);
        fclose(salida);
        return 1;
    }

    for ( tanda = 0; tanda < TANDAS; tanda++)
    {
        habitantesTandaActual = habitantesBase + (tanda < habitantesExtra ? 1 : 0);
        habitantesCreadosTanda = 0;

        for ( i = 0; i < habitantesTandaActual; i++)
        {
            indice = habitantesCreados + i;

            {
                int resultado = pthread_create(&hilosHabitantes[i], NULL, habitantes, (void*)&argsHabitantes[indice]);
                if(resultado != 0){
                    fprintf(stderr, "Error creando el hilo del habitante %d: %s\n", argsHabitantes[indice].id, strerror(resultado));
                    error = 1;
                    break;
                }
            }

            habitantesCreadosTanda++;
        }

        for ( i = 0; i < habitantesCreadosTanda; i++)
        {
            comprobar_pthread(pthread_join(hilosHabitantes[i], NULL), "pthread_join habitante");
        }

        habitantesCreados += habitantesCreadosTanda;
        if(error){
            break;
        }
    }

    for ( i = 0; i < FABRICAS; i++)
    {
        comprobar_pthread(pthread_join(hilosFabricas[i], NULL), "pthread_join fabrica");
    }

    if(error){
        registrar_evento("SIMULACION INTERRUMPIDA POR ERROR\n");
    }else{
        registrar_evento("VACUNACION FINALIZADA\n");
    }

    estadisticas();

    free(hilosFabricas);
    free(hilosHabitantes);
    free(argsHabitantes);

    for ( i = 0; i < CENTROS; i++)
    {
        comprobar_pthread(pthread_mutex_destroy(&centro[i].mutexCentro), "pthread_mutex_destroy");
        comprobar_pthread(pthread_cond_destroy(&centro[i].condicion_vacunas), "pthread_cond_destroy");
    }

    if(fclose(salida) != 0){
        perror("Error cerrando el fichero de salida");
        error = 1;
    }

    comprobar_pthread(pthread_mutex_destroy(&mutexSalida), "pthread_mutex_destroy salida");
    comprobar_pthread(pthread_mutex_destroy(&mutexAleatorio), "pthread_mutex_destroy aleatorio");
    comprobar_pthread(pthread_mutex_destroy(&mutexReparto), "pthread_mutex_destroy reparto");
    return error ? 1 : 0;
}

//Iniciamos leyendo los ficheros de entrada y salida

int inicio(int argc, char *argv[]){
    FILE *entrada;
    int camposLeidos;
    char datoExtra;

    if(argc == 2){
        fichero_salida = argv[1];
    }else if(argc == 3){
        fichero_entrada = argv[1];
        fichero_salida = argv[2];
    }else if(argc > 3){
        fprintf(stderr, "Uso: %s [fichero_salida] o %s [fichero_entrada fichero_salida]\n", argv[0], argv[0]);
        return -1;
    }

    entrada = fopen(fichero_entrada, "r");

    if(entrada == NULL){
        perror("Error: el fichero de entrada no se puede abrir");
        return -1;
    }

    camposLeidos = fscanf(
        entrada,
        "%d %d %d %d %d %d %d %d %d",
        &configuracion.habitantes_totales,
        &configuracion.vacunas_iniciales,
        &configuracion.minimo_vacunas,
        &configuracion.max_vacunas,
        &configuracion.tiempo_min_fabrica,
        &configuracion.tiempo_max_fabrica,
        &configuracion.tiempo_max_reparto,
        &configuracion.tiempo_max_cita,
        &configuracion.tiempo_max_desplazamiento
    );

    if(camposLeidos == 9 && fscanf(entrada, " %c", &datoExtra) == 1){
        camposLeidos = 10;
    }

    if(fclose(entrada) != 0){
        perror("Error cerrando el fichero de entrada");
        return -1;
    }

    if(camposLeidos != 9){
        fprintf(stderr, "Error: el fichero de entrada debe contener exactamente 9 valores enteros\n");
        return -1;
    }

    if(configuracion.habitantes_totales <= 0){
        fprintf(stderr, "Error: el numero de habitantes debe ser mayor que 0\n");
        return -1;
    }

    if(configuracion.vacunas_iniciales < 0){
        fprintf(stderr, "Error: las vacunas iniciales no pueden ser negativas\n");
        return -1;
    }

    if(configuracion.minimo_vacunas <= 0 || configuracion.max_vacunas < configuracion.minimo_vacunas){
        fprintf(stderr, "Error: el rango de vacunas fabricadas debe ser positivo y coherente\n");
        return -1;
    }

    if(configuracion.tiempo_min_fabrica < 0 || configuracion.tiempo_max_fabrica < configuracion.tiempo_min_fabrica){
        fprintf(stderr, "Error: el rango de tiempo de fabricacion no es valido\n");
        return -1;
    }

    if(configuracion.tiempo_max_reparto < 0 || configuracion.tiempo_max_cita < 0 || configuracion.tiempo_max_desplazamiento < 0){
        fprintf(stderr, "Error: los tiempos maximos no pueden ser negativos\n");
        return -1;
    }

    salida = fopen(fichero_salida, "w");

    if(salida == NULL){
        perror("Error creando el fichero de salida");
        return -1;
    }

    registrar_evento("VACUNACION EN PANDEMIA: CONFIGURACION INICIAL\n");
    registrar_evento("Habitantes: %d\n", configuracion.habitantes_totales);
    registrar_evento("Centros de vacunacion: %d\n", CENTROS);
    registrar_evento("Fabricas: %d\n", FABRICAS);
    registrar_evento("Tandas de vacunacion: %d\n", TANDAS);
    registrar_evento("Vacunas iniciales en cada centro: %d\n", configuracion.vacunas_iniciales);
    registrar_evento("Vacunas totales que fabricaran las fabricas: %d\n", configuracion.habitantes_totales);
    registrar_evento("Minimo numero de vacunas fabricadas en cada tanda: %d\n", configuracion.minimo_vacunas);
    registrar_evento("Maximo numero de vacunas fabricadas en cada tanda: %d\n", configuracion.max_vacunas);
    registrar_evento("Tiempo minimo de fabricacion de una tanda de vacunas: %d\n", configuracion.tiempo_min_fabrica);
    registrar_evento("Tiempo maximo de fabricacion de una tanda de vacunas: %d\n", configuracion.tiempo_max_fabrica);
    registrar_evento("Tiempo maximo de reparto de vacunas a los centros: %d\n", configuracion.tiempo_max_reparto);
    registrar_evento("Tiempo maximo que un habitante tarda en ver que esta citado para vacunarse: %d\n", configuracion.tiempo_max_cita);
    registrar_evento("Tiempo maximo de desplazamiento del habitante al centro de vacunacion: %d\n", configuracion.tiempo_max_desplazamiento);
    registrar_evento("\nPROCESO DE VACUNACION\n");

    return 0;
}

void registrar_evento(const char *formato, ...){
    va_list argumentos;

    comprobar_pthread(pthread_mutex_lock(&mutexSalida), "pthread_mutex_lock salida");

    va_start(argumentos, formato);
    vprintf(formato, argumentos);
    va_end(argumentos);

    if(salida != NULL){
        va_start(argumentos, formato);
        vfprintf(salida, formato, argumentos);
        va_end(argumentos);
        fflush(salida);
    }

    comprobar_pthread(pthread_mutex_unlock(&mutexSalida), "pthread_mutex_unlock salida");
}

int aleatorio_entre(int minimo, int maximo){
    int valor;

    if(maximo <= minimo){
        return minimo;
    }

    comprobar_pthread(pthread_mutex_lock(&mutexAleatorio), "pthread_mutex_lock aleatorio");
    valor = minimo + rand() % (maximo - minimo + 1);
    comprobar_pthread(pthread_mutex_unlock(&mutexAleatorio), "pthread_mutex_unlock aleatorio");

    return valor;
}

void dormir_aleatorio(int minimo, int maximo){
    int segundos = aleatorio_entre(minimo, maximo);

    if(segundos > 0){
        sleep((unsigned int)segundos);
    }
}

void comprobar_pthread(int resultado, const char *operacion){
    if(resultado != 0){
        fprintf(stderr, "Error en %s: %s\n", operacion, strerror(resultado));
        abort();
    }
}

void repartir_vacunas(Fabrica_t *fabrica, int vacunas){
    int entregas[CENTROS] = {0};
    int pendientes = vacunas;
    int i;

    comprobar_pthread(pthread_mutex_lock(&mutexReparto), "pthread_mutex_lock reparto");

    while(pendientes > 0){
        int centroObjetivo = -1;
        int mayorDeficit = 0;

        for(i = 0; i < CENTROS; i++){
            int deficit;

            comprobar_pthread(pthread_mutex_lock(&centro[i].mutexCentro), "pthread_mutex_lock centro");
            deficit = centro[i].demanda_pendiente - centro[i].stock_actual - entregas[i];
            comprobar_pthread(pthread_mutex_unlock(&centro[i].mutexCentro), "pthread_mutex_unlock centro");

            if(deficit > mayorDeficit){
                mayorDeficit = deficit;
                centroObjetivo = i;
            }
        }

        if(centroObjetivo == -1){
            break;
        }

        entregas[centroObjetivo]++;
        pendientes--;
    }

    while(pendientes > 0){
        int centroObjetivo = 0;
        int menorStockProyectado;

        comprobar_pthread(pthread_mutex_lock(&centro[0].mutexCentro), "pthread_mutex_lock centro");
        menorStockProyectado = centro[0].stock_actual + entregas[0];
        comprobar_pthread(pthread_mutex_unlock(&centro[0].mutexCentro), "pthread_mutex_unlock centro");

        for(i = 1; i < CENTROS; i++){
            int stockProyectado;

            comprobar_pthread(pthread_mutex_lock(&centro[i].mutexCentro), "pthread_mutex_lock centro");
            stockProyectado = centro[i].stock_actual + entregas[i];
            comprobar_pthread(pthread_mutex_unlock(&centro[i].mutexCentro), "pthread_mutex_unlock centro");

            if(stockProyectado < menorStockProyectado){
                menorStockProyectado = stockProyectado;
                centroObjetivo = i;
            }
        }

        entregas[centroObjetivo]++;
        pendientes--;
    }

    for(i = 0; i < CENTROS; i++){
        if(entregas[i] <= 0){
            continue;
        }

        dormir_aleatorio(0, configuracion.tiempo_max_reparto);

        comprobar_pthread(pthread_mutex_lock(&centro[i].mutexCentro), "pthread_mutex_lock centro");
        centro[i].stock_actual += entregas[i];
        centro[i].total_vacunas_recibidas += entregas[i];
        fabrica -> vacunas_entregadas_por_centro[i] += entregas[i];

        registrar_evento("Fabrica %d entrega %d vacunas en el centro %d\n", fabrica -> id, entregas[i], i + 1);

        comprobar_pthread(pthread_cond_broadcast(&centro[i].condicion_vacunas), "pthread_cond_broadcast");
        comprobar_pthread(pthread_mutex_unlock(&centro[i].mutexCentro), "pthread_mutex_unlock centro");
    }

    comprobar_pthread(pthread_mutex_unlock(&mutexReparto), "pthread_mutex_unlock reparto");
}

// Funcion de fabricas
void *fabricas(void *arg){ 
    Fabrica_t* fabrica = (Fabrica_t*) arg;
    int vacunas;

    while (fabrica -> vacunas_fabricas > 0)
    {
        // Generamos un numero aleatorio de vacunas a producir en la tanda
        vacunas = aleatorio_entre(configuracion.minimo_vacunas, configuracion.max_vacunas);
        
        if(vacunas > fabrica -> vacunas_fabricas){
            vacunas = fabrica -> vacunas_fabricas;
        }

        registrar_evento("Fabrica %d prepara %d vacunas\n", fabrica -> id, vacunas);

        dormir_aleatorio(configuracion.tiempo_min_fabrica, configuracion.tiempo_max_fabrica);

        repartir_vacunas(fabrica, vacunas);

        fabrica -> vacunas_fabricas -= vacunas;
        
    }

    registrar_evento("Fabrica %d ha fabricado todas sus vacunas\n", fabrica -> id);

    return NULL;
}

// Funcion de los habitantes
void *habitantes(void *arg){
    Habitante_t* habitante = (Habitante_t*) arg;
    int centro_id = habitante -> centro_id;
    Centro_t *c;

    registrar_evento("Habitante %d elige el centro %d para vacunarse\n", habitante -> id, centro_id + 1);

    // Simulamos el tiempo de reaccion y desplazamiento

    dormir_aleatorio(0, configuracion.tiempo_max_cita);
    dormir_aleatorio(0, configuracion.tiempo_max_desplazamiento);

    c = &centro[centro_id];
    comprobar_pthread(pthread_mutex_lock(&c -> mutexCentro), "pthread_mutex_lock centro");

    // Esperamos si no hay vacunas disponibles
    while(c -> stock_actual <= 0){
        comprobar_pthread(pthread_cond_wait(&c -> condicion_vacunas, &c -> mutexCentro), "pthread_cond_wait");
    
    }

    // Vacunamos al habitante
    c -> stock_actual--;
    c -> demanda_pendiente--;
    c -> total_vacunados++;
    comprobar_pthread(pthread_mutex_unlock(&c -> mutexCentro), "pthread_mutex_unlock centro");

    registrar_evento("Habitante %d vacunado en el centro %d\n", habitante -> id, centro_id + 1);

    return NULL;
}

// Muestra las estadisticas finales de la vacunacion
void estadisticas(void){
    int i;
    int j;
    int fabricado;
    int totalFabricadas = 0;
    int totalRecibidas = 0;
    int totalVacunados = 0;
    int totalSobrantes = 0;

    registrar_evento("\nESTADISTICAS\n");

    for ( i = 0; i < FABRICAS; i++)
    {
        fabricado = fabrica[i].vacunas_asignadas - fabrica[i].vacunas_fabricas;
        totalFabricadas += fabricado;
        registrar_evento("Fabrica %d ha fabricado %d vacunas\n", fabrica[i].id, fabricado);

        for ( j = 0; j < CENTROS; j++)
        {
            registrar_evento("  entregadas al centro %d: %d\n", j + 1, fabrica[i].vacunas_entregadas_por_centro[j]);
            
        }
        
        
    }

    for ( i = 0; i < CENTROS; i++)
    {
        totalRecibidas += centro[i].total_vacunas_recibidas;
        totalVacunados += centro[i].total_vacunados;
        totalSobrantes += centro[i].stock_actual;

        registrar_evento(
            "Centro %d -> asignados: %d, pendientes: %d, recibidas: %d, vacunados: %d, sobran: %d\n",
            centro[i].id,
            centro[i].habitantes_asignados,
            centro[i].demanda_pendiente,
            centro[i].total_vacunas_recibidas,
            centro[i].total_vacunados,
            centro[i].stock_actual
        );
        
    }

    registrar_evento("\nResumen global\n");
    registrar_evento("Vacunas fabricadas: %d\n", totalFabricadas);
    registrar_evento("Vacunas recibidas por centros: %d\n", totalRecibidas);
    registrar_evento("Habitantes vacunados: %d de %d\n", totalVacunados, configuracion.habitantes_totales);
    registrar_evento("Vacunas sobrantes en centros: %d\n", totalSobrantes);

}
