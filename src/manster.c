#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>

#define _GNU_SOURCE

int N_ATOMI_INIT, N_ATOM_MAX, MIN_N_ATOMICO, ENERGY_DEMAND, STEP, N_NUOVI_ATOMI, SIM_DURATION, ENERGY_EXPLODE_THRESHOLD;


/**
 * Crea un nuovo processo figlio che esegue il programma `atomo` con un numero atomico casuale come argomento.
 */
void createAtomo() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        int numero_atomico = N_ATOM_MAX;
        char num_atomico_str[10];
        snprintf(num_atomico_str, sizeof(num_atomico_str), "%d", numero_atomico);

        printf("Figlio %d: Sto diventando un atomo con numero atomico %d\n", getpid(), numero_atomico);

        // Esegui `atomo` con il numero atomico come argomento
        if (execlp("./atomo", "atomo", num_atomico_str, NULL) == -1) {
            perror("execlp failed in creating atomo");
            exit(EXIT_FAILURE);
        }
    } else {
        // Processo padre
        printf("Manster: Creazione di un atomo pid: %d\n", pid);
    }
}

/**
 * Funzione per leggere i parametri dal file di configurazione
 */
void readparameters() {
    FILE *file = fopen("../Data/parameters.txt", "r");
    if (file == NULL) {
        perror("Errore nell'apertura del file di configurazione");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
        
        char key[128];
        int value;

        if (sscanf(line, "%127[^=]=%d", key, &value) == 2) {
            if (strcmp(key, "N_ATOMI_INIT") == 0) {
                N_ATOMI_INIT = value;
            } else if (strcmp(key, "N_ATOM_MAX") == 0) {
                N_ATOM_MAX = value;
            } else if (strcmp(key, "MIN_N_ATOMICO") == 0) {
                MIN_N_ATOMICO = value;
            } else if (strcmp(key, "ENERGY_DEMAND") == 0) {
                ENERGY_DEMAND = value;
            } else if (strcmp(key, "STEP") == 0) {
                STEP = value;
            } else if (strcmp(key, "N_NUOVI_ATOMI") == 0) {
                N_NUOVI_ATOMI = value;
            } else if (strcmp(key, "SIM_DURATION") == 0) {
                SIM_DURATION = value;
            } else if (strcmp(key, "ENERGY_EXPLODE_THRESHOLD") == 0) {
                ENERGY_EXPLODE_THRESHOLD = value;
            } else {
                fprintf(stderr, "Chiave sconosciuta: %s\n", key);
            }
        } else {
            fprintf(stderr, "Formato di riga non valido: %s\n", line);
        }
    }

    fclose(file);
}

int main() {
    srand(time(NULL));  // Inizializza il generatore di numeri casuali

    readparameters();
    printf("Manster: Inizio simulazione ho pid %d\n", getpid());
    printf("Manster: Parametri letti dal file di configurazione:\n");
    printf("Manster: Inizio creazione atomi iniziali\n");
    for (int i = 0; i < N_ATOMI_INIT; i++) {
        createAtomo();
    }
    printf("Manster: Fine creazione atomi iniziali\n");

    // Attende la terminazione di tutti i figli
    while (wait(NULL) > 0);

    return 0;
}
