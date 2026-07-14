# Simulacion de vacunacion con pthreads

Proyecto academico en C que simula una campana de vacunacion usando hilos POSIX. El programa modela fabricas que producen vacunas, centros que reciben stock y habitantes que acuden a vacunarse por tandas.

El objetivo principal es practicar programacion concurrente de bajo nivel: creacion de hilos, mutex, variables de condicion, sincronizacion de recursos compartidos y escritura coordinada de logs.

## Por que encaja en un portfolio de ciberseguridad

- Demuestra base de programacion de sistemas en C.
- Usa sincronizacion explicita con `pthread_mutex_t` y `pthread_cond_t`.
- Trata problemas habituales en software concurrente: carreras de datos, espera por recursos, validacion de entrada y logs desde varios hilos.
- Es un buen punto de partida para hablar de robustez, comportamiento indefinido y programacion defensiva.

## Compilacion

```bash
make
```

Tambien se puede compilar directamente:

```bash
cc -Wall -Wextra -Wpedantic -O2 -pthread practica2.c -o practica2
```

## Ejecucion

```bash
./practica2
```

O con `make`:

```bash
make run
```

El programa escribe el progreso por pantalla y tambien en el fichero de salida indicado.

Argumentos admitidos:

```bash
./practica2                         # usa entrada.txt y salida.txt
./practica2 resultado.txt           # usa entrada.txt y escribe en resultado.txt
./practica2 config.txt resultado.txt # usa ambos ficheros explicitamente
```

## Formato del fichero de entrada

El fichero de entrada debe contener 9 enteros, en este orden:

```text
habitantes_totales
vacunas_iniciales_por_centro
minimo_vacunas_fabricadas_por_tanda
maximo_vacunas_fabricadas_por_tanda
tiempo_minimo_fabricacion
tiempo_maximo_fabricacion
tiempo_maximo_reparto
tiempo_maximo_cita
tiempo_maximo_desplazamiento
```

Los tiempos estan expresados en segundos.

## Mejoras incluidas

- Validacion completa del fichero de entrada.
- Reparto de habitantes aunque el total no sea multiplo de las tandas.
- Reparto de vacunas aunque el total no sea multiplo de las fabricas.
- Reparto de vacunas basado en la demanda pendiente de cada centro para evitar inanicion.
- Generacion aleatoria protegida por mutex para evitar carreras de datos.
- Escritura de eventos protegida por mutex para evitar logs intercalados.
- Estadisticas finales agregadas.

## Estructura

```text
.
├── practica2.c
├── entrada.txt
├── entrada.example.txt
├── Makefile
├── README.md
└── LICENSE
```

La memoria PDF original se conserva en local, pero no se versiona por defecto para evitar publicar datos personales o academicos sin una revision previa.
