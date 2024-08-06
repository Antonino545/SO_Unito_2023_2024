#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "lib.h"

#define _GNU_SOURCE

// Definizione delle variabili come puntatori che punteranno a memoria condivisa
int *N_ATOMI_INIT;
int *N_ATOM_MAX;
int *MIN_N_ATOMICO;
int *ENERGY_DEMAND;
int *STEP;
int *N_NUOVI_ATOMI;
int *SIM_DURATION;
int *ENERGY_EXPLODE_THRESHOLD;

/**
 * Crea un nuovo processo figlio che esegue il programma `atomo` con un numero atomico casuale come argomento.
 */
void createAtomo() {
    pid_t pid = fork(); // Crea un nuovo processo

    if (pid < 0) { // Processo non creato
        perror("Errore nella fork:");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Processo figlio
        int numero_atomico = generate_random(*N_ATOM_MAX); // Genera un numero atomico casuale tra 1 e N_ATOM_MAX
        char num_atomico_str[20];
        
        snprintf(num_atomico_str, sizeof(num_atomico_str), "%d", numero_atomico); // Converti il numero atomico in stringa

        printf("Figlio (PID: %d): Sto diventando un atomo con numero atomico %d\n", getpid(), numero_atomico);

        // Esegui `atomo` con il numero atomico come argomento
        if (execlp("./atomo", "atomo", num_atomico_str, NULL) == -1) { // execlp ritorna -1 se fallisce
            perror("Errore in execlp durante la creazione del processo atomo");
            exit(EXIT_FAILURE);
        }
    } else { // Processo padre
        printf("Master: Creazione di un atomo con PID: %d\n", pid);
    }
}

/**
 * Crea un nuovo processo figlio che esegue il programma `attivatore`.
 */
void createAttivatore(){
    pid_t pid = fork(); // Crea un nuovo processo

    if (pid < 0) { // Processo non creato
        perror("Errore nella fork:");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Processo figlio
        printf("Figlio (PID: %d): Sto diventando un attivatore\n", getpid());

        // Esegui `attivatore`
        if (execlp("./attivatore", "attivatore", NULL) == -1) { // execlp ritorna -1 se fallisce
            perror("Errore in execlp durante la creazione del processo attivatore");
            exit(EXIT_FAILURE);
        }
    } else { // Processo padre
        printf("Master: Creazione di un attivatore con PID: %d\n", pid);
    }
}

/**
 * Crea un nuovo processo figlio che esegue il programma `alimentazione`.
 */
void createAlimentazione(){
    pid_t pid = fork(); // Crea un nuovo processo

    if (pid < 0) { // Processo non creato
        perror("Errore nella fork:");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Processo figlio
        printf("Figlio (PID: %d): Sto diventando il processo alimentazione\n", getpid());

        // Esegui `alimentazione`
        if (execlp("./alimentazione", "alimentazione", NULL) == -1) { // execlp ritorna -1 se fallisce
            perror("Errore in execlp durante la creazione del processo alimentazione");
            exit(EXIT_FAILURE);
        }
    } else { // Processo padre
        printf("Master: Creazione di un alimentatore con PID: %d\n", pid);
    }
}

/**
 * Funzione per leggere i parametri dal file di configurazione
 */
void readparameters(FILE *file) {
    if (file == NULL) { // Verifica che il file sia stato aperto correttamente
        perror("Errore nell'apertura del file di configurazione");
        exit(EXIT_FAILURE);
    }

    char line[256]; // Buffer per leggere ogni linea del file
    while (fgets(line, sizeof(line), file)) { // Legge il file riga per riga
        line[strcspn(line, "\r\n")] = 0; // Rimuove il carattere di newline
        
        char key[128];
        int value;

        // Parsea la linea in formato chiave=valore
        if (sscanf(line, "%127[^=]=%d", key, &value) == 2) {
            // Assegna il valore alla variabile corrispondente
            if (strcmp(key, "N_ATOMI_INIT") == 0) {
                *N_ATOMI_INIT = value;
            } else if (strcmp(key, "N_ATOM_MAX") == 0) {
                *N_ATOM_MAX = value;
            } else if (strcmp(key, "MIN_N_ATOMICO") == 0) {
                *MIN_N_ATOMICO = value;
            } else if (strcmp(key, "ENERGY_DEMAND") == 0) {
                *ENERGY_DEMAND = value;
            } else if (strcmp(key, "STEP") == 0) {
                *STEP = value;
            } else if (strcmp(key, "N_NUOVI_ATOMI") == 0) {
                *N_NUOVI_ATOMI = value;
            } else if (strcmp(key, "SIM_DURATION") == 0) {
                *SIM_DURATION = value;
            } else if (strcmp(key, "ENERGY_EXPLODE_THRESHOLD") == 0) {
                *ENERGY_EXPLODE_THRESHOLD = value;
            } else {
                fprintf(stderr, "Chiave sconosciuta: %s\n", key);
            }
        } else {
            fprintf(stderr, "Formato di riga non valido: %s\n", line);
        }
    }

    fclose(file); // Chiude il file dopo aver letto tutti i parametri
}

int main() {
    // Creare e mappare la memoria condivisa
    const char *shm_name = "/shared_mem";
    const size_t shm_size = 8 * sizeof(int); // 8 interi

    // Crea o apre una memoria condivisa
    //shm_fd serve per identificare la memoria condivisa 
    //sgm_name é il nome della memoria condivisa
    //O_CREAT indica che se la memoria condivisa non esiste, deve essere creata
    //O_RDWR indica che la memoria condivisa è sia leggibile che scrivibile
    //0666 indica che la memoria condivisa è accessibile a tutti gli utenti 
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Errore nella shm_open");
        exit(EXIT_FAILURE);
    }

    // Imposta la dimensione della memoria condivisa
    if (ftruncate(shm_fd, shm_size) == -1) {//ftruncate serve per impostare la dimensione della memoria condivisa
        perror("Errore nella ftruncate");
        exit(EXIT_FAILURE);
    }

    // Mappa la memoria condivisa nel proprio spazio degli indirizzi
    //shm_size è la dimensione della memoria condivisa
    //PROT_READ | PROT_WRITE indica che la memoria condivisa è sia leggibile che scrivibile
    //MAP_SHARED indica che la memoria condivisa è condivisa tra più processi
    //shm_fd è il file descriptor della memoria condivisa che serve per identificarla 
    void *shm_ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("Errore nella mmap");
        exit(EXIT_FAILURE);
    }

    // Puntatori alle variabili nella memoria condivisa
    //DA 0 a 7 * sizeof(int) ci sono 8 interi
    N_ATOMI_INIT = (int *)shm_ptr;
    N_ATOM_MAX = (int *)(shm_ptr + sizeof(int));
    MIN_N_ATOMICO = (int *)(shm_ptr + 2 * sizeof(int));
    ENERGY_DEMAND = (int *)(shm_ptr + 3 * sizeof(int));
    STEP = (int *)(shm_ptr + 4 * sizeof(int));
    N_NUOVI_ATOMI = (int *)(shm_ptr + 5 * sizeof(int));
    SIM_DURATION = (int *)(shm_ptr + 6 * sizeof(int));
    ENERGY_EXPLODE_THRESHOLD = (int *)(shm_ptr + 7 * sizeof(int));

    // Apri il file di configurazione e leggi i parametri
    FILE *file = fopen("../Data/parameters.txt", "r");
    if (file == NULL) {
        perror("Errore nell'apertura del file di configurazione");
        exit(EXIT_FAILURE);
    } else {
        readparameters(file);
    }

    // Stampa informazioni di inizio simulazione
    printf("Master: Inizio simulazione ho pid %d\n", getpid());
    printf("Master: Parametri letti dal file di configurazione:\n");

    // Creazione dei processi necessari
    printf("Master:Creazione del processo alimentatore\n");
    createAlimentazione();

    printf("Master:Creazione del processo attivatore\n");
    createAttivatore();

    printf("Master: Inizio creazione atomi iniziali\n");
    for (int i = 0; i < *N_ATOMI_INIT; i++) {
        createAtomo();
    }
    printf("Master: Fine creazione atomi iniziali\n");

    // Attende la terminazione di tutti i figli
    while(wait(NULL) != -1); 

    // Pulizia della memoria condivisa
    shm_unlink(shm_name);// alla chiusur del processo la memoria condivisa viene rimossa

    exit(EXIT_SUCCESS);
}
