// genNumeroAtomico.c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Dichiarazione della funzione
int genNumeroAtomico(int max);

int genNumeroAtomico(int max) {
    // Inizializza il generatore di numeri casuali
    srand(time(NULL));
    // Genera un numero casuale compreso tra 1 e max
    return rand() % max + 1;
}
