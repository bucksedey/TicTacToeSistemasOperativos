/*
Flores Anzurez Marco Antonio

Planteamiento: Realizar un juego de gato (tic tac toe) solamente usando como base el 
lenguaje C de programación. Permitiendo dos modos de juego: Juego contra la computadora y un 
modo de jugador contra jugador implementando el uso de Tuberías, Hilos, semáforos, algoritmos de planificación y memoria compartida
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>

#define BOARD_SIZE 3

char (*tablero)[BOARD_SIZE];  // Tablero del juego
sem_t *sem_turno;  // Semáforo para controlar los turnos de los jugadores
sem_t *sem_jugada;  // Semáforo para controlar las jugadas

const char JUGADOR1 = 'X';  // Símbolo del jugador 1
const char JUGADOR2 = 'O';  // Símbolo del jugador 2

typedef struct {
    int jugador;  // Número de jugador (1 o 2)
    pthread_t thread_id;  // Identificador del hilo del jugador
    int pipefd;  // Descriptor de la tubería del jugador
} JugadorInfo;

void* turnoJugador(void* arg);  // Función ejecutada por el hilo de un jugador humano
void* turnoComputadora(void* arg);  // Función ejecutada por el hilo de la computadora
void reiniciarTablero();  // Reinicia el tablero del juego
void imprimirTablero();  // Imprime el tablero del juego
int verificarEspaciosLibres();  // Verifica cuántos espacios vacíos hay en el tablero
char verificarGanador();  // Verifica si hay un ganador y devuelve el símbolo del ganador
void imprimirGanador(char);  // Imprime el mensaje de felicitaciones al ganador o empate
void jugarModoUnJugador();  // Función para jugar en modo un jugador (jugador humano contra la computadora)
void jugarModoDosJugadores();  // Función para jugar en modo dos jugadores (jugador humano contra jugador humano)
void limpiarRecursos();  // Libera los recursos utilizados por el juego

int shmid;  // Identificador del segmento de memoria compartida

int minimax(char tablero[BOARD_SIZE][BOARD_SIZE], int profundidad, bool esMax, char jugador);
int encontrarMejorMovimiento(char tablero[BOARD_SIZE][BOARD_SIZE], char jugador);

int main()
{
    srand(time(NULL));

    // Obtener el segmento de memoria compartida para el tablero del juego
    shmid = shmget(IPC_PRIVATE, BOARD_SIZE * BOARD_SIZE * sizeof(char), IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("Error obteniendo el segmento de memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Adjuntar el segmento de memoria compartida al espacio de direcciones del proceso
    tablero = shmat(shmid, NULL, 0);
    if (tablero == (void*)-1)
    {
        perror("Error adjuntando el segmento de memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Crear semáforos para controlar los turnos y las jugadas
    sem_turno = sem_open("/sem_turno", O_CREAT, 0666, 1);
    sem_jugada = sem_open("/sem_jugada", O_CREAT, 0666, 0);

    // Registrar la función para limpiar los recursos al finalizar el programa
    atexit(limpiarRecursos);

    int opcion;

    do
    {
        printf("Elige el modo de juego:\n");
        printf("1. Un Jugador\n");
        printf("2. Dos Jugadores\n");
        printf("3. Salir\n");
        printf("Opción: ");
        scanf("%d", &opcion);

        switch (opcion)
        {
            case 1:
                jugarModoUnJugador();
                break;
            case 2:
                jugarModoDosJugadores();
                break;
            case 3:
                printf("Saliendo del juego.\n");
                break;
            default:
                printf("Opción no válida. Por favor, elige una opción entre 1 y 3.\n");
        }
    } while (opcion != 3);

    return 0;
}

void reiniciarTablero()
{
    // Reiniciar el tablero, colocando espacios en todas las posiciones
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            tablero[i][j] = ' ';
        }
    }
}

void imprimirTablero()
{
    // Imprimir el tablero del juego en su estado actual
    printf(" %c | %c | %c \n", tablero[0][0], tablero[0][1], tablero[0][2]);
    printf("---|---|---\n");
    printf(" %c | %c | %c \n", tablero[1][0], tablero[1][1], tablero[1][2]);
    printf("---|---|---\n");
    printf(" %c | %c | %c \n", tablero[2][0], tablero[2][1], tablero[2][2]);
    printf("\n");
}

int verificarEspaciosLibres()
{
    // Contar el número de espacios vacíos en el tablero
    int espaciosLibres = BOARD_SIZE * BOARD_SIZE;

    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            if (tablero[i][j] != ' ')
            {
                espaciosLibres--;
            }
        }
    }
    return espaciosLibres;
}

void* turnoJugador(void* arg)
{
    JugadorInfo* jugadorInfo = (JugadorInfo*)arg;
    int jugador = jugadorInfo->jugador;
    sem_wait(sem_turno);
    int x, y;

    do
    {
        printf("Jugador %d: Ingresa el número de fila (1-%d): ", jugador, BOARD_SIZE);
        scanf("%d", &x);
        x--;
        printf("Jugador %d: Ingresa el número de columna (1-%d): ", jugador, BOARD_SIZE);
        scanf("%d", &y);
        y--;

        if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE)
        {
            printf("¡Posición inválida! Por favor, ingresa valores dentro del rango válido.\n");
        }
        else if (tablero[x][y] != ' ')
        {
            printf("¡Movimiento inválido! La posición ya está ocupada.\n");
        }
        else
        {
            // Escribe las coordenadas en la tubería para enviarlas al otro jugador
            int coords[2] = {x, y};
            write(jugadorInfo->pipefd, coords, sizeof(coords));
            sem_post(sem_jugada);
            break;
        }
    } while (1);

    sem_post(sem_turno);
    return NULL;
}

void* turnoComputadora(void* arg)
{
    JugadorInfo* jugadorInfo = (JugadorInfo*)arg;
    sem_wait(sem_turno);

    if (verificarEspaciosLibres() > 0)
    {
        // Encontrar el mejor movimiento utilizando el algoritmo minimax
        int mejorMovimiento = encontrarMejorMovimiento(tablero, JUGADOR2);
        int x = mejorMovimiento / BOARD_SIZE;
        int y = mejorMovimiento % BOARD_SIZE;

        // Escribe las coordenadas en la tubería para enviarlas al otro jugador
        int coords[2] = {x, y};
        write(jugadorInfo->pipefd, coords, sizeof(coords));
        sem_post(sem_jugada);
    }
    else
    {
        // No hay más espacios libres, el juego ha terminado en empate
        imprimirGanador(' ');
        sem_post(sem_jugada);
    }

    sem_post(sem_turno);
    return NULL;
}

char verificarGanador()
{
    // Verificar si hay un ganador en el tablero
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        if (tablero[i][0] == tablero[i][1] && tablero[i][0] == tablero[i][2])
        {
            return tablero[i][0];
        }
    }

    for (int i = 0; i < BOARD_SIZE; i++)
    {
        if (tablero[0][i] == tablero[1][i] && tablero[0][i] == tablero[2][i])
        {
            return tablero[0][i];
        }
    }

    if (tablero[0][0] == tablero[1][1] && tablero[0][0] == tablero[2][2])
    {
        return tablero[0][0];
    }

    if (tablero[0][2] == tablero[1][1] && tablero[0][2] == tablero[2][0])
    {
        return tablero[0][2];
    }

    return ' ';
}

void imprimirGanador(char ganador)
{
    // Imprimir el mensaje de felicitaciones al ganador o mensaje de empate
    if (ganador == JUGADOR1)
    {
        printf("¡El jugador X (Jugador 1) gana!\n");
    }
    else if (ganador == JUGADOR2)
    {
        printf("¡El jugador O (Jugador 2) gana!\n");
    }
    else
    {
        printf("¡Empate!\n");
    }
}

void jugarModoUnJugador()
{
    printf("Modo de juego: Un Jugador\n");
    printf("El jugador X juega contra la computadora O.\n");

    JugadorInfo jugador1, jugador2;
    jugador1.jugador = 1;
    jugador2.jugador = 2;

    int pipefd_jugador1[2];
    int pipefd_jugador2[2];

    if (pipe(pipefd_jugador1) == -1)
    {
        perror("Error creando la tubería del jugador 1");
        exit(EXIT_FAILURE);
    }

    if (pipe(pipefd_jugador2) == -1)
    {
        perror("Error creando la tubería del jugador 2");
        exit(EXIT_FAILURE);
    }

    jugador1.pipefd = pipefd_jugador1[1];
    jugador2.pipefd = pipefd_jugador2[1];

    reiniciarTablero();

    while (verificarEspaciosLibres() > 0 && verificarGanador() == ' ')
    {
        imprimirTablero();

        pthread_create(&jugador1.thread_id, NULL, turnoJugador, &jugador1);
        pthread_join(jugador1.thread_id, NULL);  // esperamos a que el jugador termine su turno

        sem_wait(sem_jugada);
        // Leemos las coordenadas del jugador 1 de la tubería y actualizamos el tablero
        int coords[2];
        read(pipefd_jugador1[0], coords, sizeof(coords));
        tablero[coords[0]][coords[1]] = JUGADOR1;
        sem_post(sem_jugada);

        // Verificamos si el juego ya terminó
        char ganador = verificarGanador();
        if (ganador != ' ' || verificarEspaciosLibres() == 0) {
            imprimirTablero();
            imprimirGanador(ganador);
            break;
        }

        pthread_create(&jugador2.thread_id, NULL, turnoComputadora, &jugador2);
        pthread_join(jugador2.thread_id, NULL); // esperamos a que la computadora termine su turno

        sem_wait(sem_jugada);
        // Leemos las coordenadas de la computadora de la tubería y actualizamos el tablero
        read(pipefd_jugador2[0], coords, sizeof(coords));
        tablero[coords[0]][coords[1]] = JUGADOR2;
        sem_post(sem_jugada);

        // Verificamos si el juego ya terminó después de la jugada de la computadora
        ganador = verificarGanador();
        if (ganador != ' ' || verificarEspaciosLibres() == 0) {
            imprimirTablero();
            imprimirGanador(ganador);
            break;
        }
    }

    close(pipefd_jugador1[0]);
    close(pipefd_jugador1[1]);
    close(pipefd_jugador2[0]);
    close(pipefd_jugador2[1]);
}

void jugarModoDosJugadores()
{
    printf("Modo de juego: Dos Jugadores\n");
    printf("El jugador X (Jugador 1) juega contra el jugador O (Jugador 2).\n");

    JugadorInfo jugador1, jugador2;
    jugador1.jugador = 1;
    jugador2.jugador = 2;

    int pipefd_jugador1[2];
    int pipefd_jugador2[2];

    if (pipe(pipefd_jugador1) == -1)
    {
        perror("Error creando la tubería del jugador 1");
        exit(EXIT_FAILURE);
    }

    if (pipe(pipefd_jugador2) == -1)
    {
        perror("Error creando la tubería del jugador 2");
        exit(EXIT_FAILURE);
    }

    jugador1.pipefd = pipefd_jugador1[1];
    jugador2.pipefd = pipefd_jugador2[1];

    reiniciarTablero();

    while (verificarEspaciosLibres() > 0 && verificarGanador() == ' ')
    {
        pthread_create(&jugador1.thread_id, NULL, turnoJugador, &jugador1);
        sem_wait(sem_jugada);
        // Leemos las coordenadas del jugador 1 de la tubería y actualizamos el tablero
        int coords[2];
        read(pipefd_jugador1[0], coords, sizeof(coords));
        tablero[coords[0]][coords[1]] = JUGADOR1;

        imprimirTablero();
        sem_post(sem_jugada);

        // Verificamos si el juego ya terminó después de la jugada del jugador 1
        char ganador = verificarGanador();
        if (ganador != ' ' || verificarEspaciosLibres() == 0) {
            imprimirGanador(ganador);
            break;
        }

        pthread_create(&jugador2.thread_id, NULL, turnoJugador, &jugador2);
        sem_wait(sem_jugada);
        // Leemos las coordenadas del jugador 2 de la tubería y actualizamos el tablero
        read(pipefd_jugador2[0], coords, sizeof(coords));
        tablero[coords[0]][coords[1]] = JUGADOR2;

        imprimirTablero();
        sem_post(sem_jugada);

        // Verificamos si el juego ya terminó después de la jugada del jugador 2
        ganador = verificarGanador();
        if (ganador != ' ' || verificarEspaciosLibres() == 0) {
            imprimirGanador(ganador);
            break;
        }
    }

    close(pipefd_jugador1[0]);
    close(pipefd_jugador1[1]);
    close(pipefd_jugador2[0]);
    close(pipefd_jugador2[1]);
}

void limpiarRecursos()
{
    // Liberar el segmento de memoria compartida
    if (shmdt(tablero) == -1)
    {
        perror("Error liberando el segmento de memoria compartida");
    }

    if (shmctl(shmid, IPC_RMID, 0) == -1)
    {
        perror("Error eliminando el segmento de memoria compartida");
    }

    // Cerrar y eliminar los semáforos
    sem_close(sem_turno);
    sem_close(sem_jugada);
    sem_unlink("/sem_turno");
    sem_unlink("/sem_jugada");
}

int minimax(char tablero[BOARD_SIZE][BOARD_SIZE], int profundidad, bool esMax, char jugador) {
    char ganador = verificarGanador();
    
    if (ganador != ' ') {
        if (ganador == JUGADOR1)
            return -10 + profundidad;
        else if (ganador == JUGADOR2)
            return 10 - profundidad;
    }

    if (verificarEspaciosLibres() == 0)
        return 0;

    if (esMax) {
        int maxEval = INT_MIN;
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                if (tablero[i][j] == ' ') {
                    tablero[i][j] = jugador;
                    int currEval = minimax(tablero, profundidad + 1, false, JUGADOR1);
                    tablero[i][j] = ' ';
                    maxEval = (currEval > maxEval) ? currEval : maxEval;
                }
            }
        }
        return maxEval;
    } else {
        int minEval = INT_MAX;
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                if (tablero[i][j] == ' ') {
                    tablero[i][j] = jugador;
                    int currEval = minimax(tablero, profundidad + 1, true, JUGADOR2);
                    tablero[i][j] = ' ';
                    minEval = (currEval < minEval) ? currEval : minEval;
                }
            }
        }
        return minEval;
    }
}

int encontrarMejorMovimiento(char tablero[BOARD_SIZE][BOARD_SIZE], char jugador) {
    int bestMoveValue = INT_MIN;
    int bestMove = -1;

    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (tablero[i][j] == ' ') {
                tablero[i][j] = jugador;
                int moveValue = minimax(tablero, 0, false, JUGADOR1);
                tablero[i][j] = ' ';
                
                if (moveValue > bestMoveValue) {
                    bestMoveValue = moveValue;
                    bestMove = i * BOARD_SIZE + j;
                }
            }
        }
    }
    return bestMove;
}
