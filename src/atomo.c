#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include "lib.h"
  int numero_atomico=0;
  void atomdivision();
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <numero_atomico>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
      numero_atomico = atoi(argv[1]);
    printf("Atomo %d: Sono un atomo con numero atomico %d\n", getpid(), numero_atomico);
    if(numero_atomico<40){
        return 0;
    }
    atomdivision();
    return 0;
}

void atomdivision( ) {

    int numero_atomico_figlio = generate_random(numero_atomico); // Genera un numero atomico per il figlio
     numero_atomico = numero_atomico - numero_atomico_figlio; // Calcola il numero atomico rimanente per il padre

    pid_t pid = fork();

    if (pid < 0) { // Processo non creato
        perror("Errore nella fork:");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Processo figlio
   
        char num_atomico_figlio_str[3];
        snprintf(num_atomico_figlio_str, sizeof(num_atomico_figlio_str), "%d", numero_atomico_figlio);

        printf("Figlio (PID: %d): Sto diventando un atomo con numero atomico %d\n", getpid(), numero_atomico_figlio);

        // Esegui `atomo` con il numero atomico del figlio come argomento
        if (execlp("./atomo", "atomo", num_atomico_figlio_str, NULL) == -1) { // execlp ritorna -1 se fallisce
            perror("Errore in execlp durante la creazione del processo atomo");
            exit(EXIT_FAILURE);
        }
    } else { // Processo padre
        printf("Atomo %d: Mi sto scindendo in due atomi con numeri atomici %d e %d\n", getpid(), numero_atomico, numero_atomico_figlio);

        // Attendi la terminazione del processo figlio
        wait(NULL);
    }
}