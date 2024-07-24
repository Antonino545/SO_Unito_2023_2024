#include <stdio.h>



// Dichiarazione della funzione genNumeroAtomico
int genNumeroAtomico(int max);

int main() {
    int maxNumero;
    printf("Inserisci il numero massimo: ");
    scanf("%d", &maxNumero);

    int numeroCasuale = genNumeroAtomico(500);
    printf("Il numero atomico generato è: %d\n", numeroCasuale);

    return 0;
}