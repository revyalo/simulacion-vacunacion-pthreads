# Simulación de vacunación con pthreads

Proyecto académico en C que simula una campaña de vacunación usando hilos POSIX. El programa modela fábricas que producen vacunas, centros que reciben stock y habitantes que acuden a vacunarse por tandas.

El objetivo principal es practicar programación concurrente de bajo nivel: creación de hilos, mutex, variables de condición, sincronización de recursos compartidos y escritura coordinada de logs.

## Conceptos trabajados

- Creación y gestión de hilos mediante POSIX Threads.
- Sincronización con `pthread_mutex_t`.
- Coordinación mediante `pthread_cond_t`.
- Gestión de recursos compartidos.
- Prevención de condiciones de carrera.
- Espera y señalización entre hilos.
- Coordinación entre productores, centros y habitantes.
- Validación de datos de entrada.
- Escritura sincronizada de logs.
- Cálculo de estadísticas al finalizar la simulación.

El proyecto permite experimentar con problemas habituales de programación concurrente, como el acceso simultáneo a datos compartidos, la espera por recursos y la coordinación entre tareas que se ejecutan en paralelo.

## Compilación

```bash
make
```

También se puede compilar directamente:

```bash
cc -Wall -Wextra -Wpedantic -O2 -pthread practica2.c -o practica2
```

## Ejecución

```bash
./practica2
```

O con `make`:

```bash
make run
```

El programa escribe el progreso por pantalla y también en el fichero de salida indicado.

Argumentos admitidos:

```bash
./practica2                          # usa entrada.txt y salida.txt
./practica2 resultado.txt            # usa entrada.txt y escribe en resultado.txt
./practica2 config.txt resultado.txt # usa ambos ficheros explícitamente
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

Los tiempos están expresados en segundos.

## Mejoras incluidas

- Validación completa del fichero de entrada.
- Reparto de habitantes aunque el total no sea múltiplo de las tandas.
- Reparto de vacunas aunque el total no sea múltiplo de las fábricas.
- Reparto de vacunas basado en la demanda pendiente de cada centro para evitar inanición.
- Generación aleatoria protegida por mutex para evitar carreras de datos.
- Escritura de eventos protegida por mutex para evitar logs intercalados.
- Estadísticas finales agregadas.

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

La memoria PDF original se conserva en local, pero no se versiona por defecto para evitar publicar datos personales o académicos sin una revisión previa.
