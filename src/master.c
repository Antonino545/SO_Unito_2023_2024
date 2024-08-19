#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "lib.h"

#define MSGSZ 128

// Definizione della struttura del messaggio
typedef struct msgbuf {
    long mtype;
    char mtext[MSGSZ];
} message_buf;

// Definizione delle variabili come puntatori che punteranno a memoria condivisa
int *N_ATOMI_INIT;
int *N_ATOM_MAX;
int *MIN_N_ATOMICO;
int *ENERGY_DEMAND;
int *STEP;
int *N_NUOVI_ATOMI;
int *SIM_DURATION;
int *ENERGY_EXPLODE_THRESHOLD;
int msqid;

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
        printf("Master (PID: %d): Creazione di un atomo con PID: %d\n", getpid(), pid);
    }
}

/**
 * Crea un nuovo processo figlio che esegue il programma `attivatore`.
 */
void createAttivatore() {
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
        printf("Master (PID: %d): Creazione di un attivatore con PID: %d\n", getpid(), pid);
    }
}

/**
 * Crea un nuovo processo figlio che esegue il programma `alimentazione`.
 */
void createAlimentazione() {
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
        printf("Master (PID: %d): Creazione di un alimentatore con PID: %d\n", getpid(), pid);
    }
}

/**
 * Funzione per leggere i parametri dal file di configurazione
 */
void readparameters(FILE *file) {
    if (file == NULL) { // Verifica che il file sia stato aperto correttamente
        perror("Errore nell'apertura del file di configurazione");
        exit(EXIT_FAILURE); // Sono Costanti definite in stdlib.h e indicano rispettivamente il successo e il fallimento di un programma
    }

    char line[256]; // Buffer per leggere ogni linea del file
    while (fgets(line, sizeof(line), file)) { // Legge il file riga per riga
        line[strcspn(line, "\r\n")] = 0; // Rimuove il carattere di newline

        char key[128];
        int value;

        // Parsea la linea in formato chiave=valore
        if (sscanf(line, "%127[^=]=%d", key, &value) == 2) { // Parsea la linea nel formato chiave=valore
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
            }
        }
    }

    fclose(file); // Chiude il file dopo aver letto tutti i parametri
}

int main() {
    printf("Master (PID: %d): Inizio esecuzione\n", getpid());

    // Creare e mappare la memoria condivisa
    const char *shm_name = "/Parametres";
    const size_t shm_size = 8 * sizeof(int); // 8 interi

    // Crea o apre una memoria condivisa
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Errore nella shm_open");
        exit(EXIT_FAILURE);
    }

    // Imposta la dimensione della memoria condivisa
    if (ftruncate(shm_fd, shm_size) == -1) {
        perror("Errore nella ftruncate");
        exit(EXIT_FAILURE);
    }

    // Mappa la memoria condivisa nel proprio spazio degli indirizzi
    void *shm_ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("Errore nella mmap");
        exit(EXIT_FAILURE);
    }

    // Puntatori alle variabili nella memoria condivisa
    N_ATOMI_INIT = (int *)shm_ptr;
    N_ATOM_MAX = (int *)(shm_ptr + sizeof(int));
    MIN_N_ATOMICO = (int *)(shm_ptr + 2 * sizeof(int));
    ENERGY_DEMAND = (int *)(shm_ptr + 3 * sizeof(int));
    STEP = (int *)(shm_ptr + 4 * sizeof(int));
    N_NUOVI_ATOMI = (int *)(shm_ptr + 5 * sizeof(int));
    SIM_DURATION = (int *)(shm_ptr + 6 * sizeof(int));
    ENERGY_EXPLODE_THRESHOLD = (int *)(shm_ptr + 7 * sizeof(int));

    printf("Master (PID: %d): Parametri inizializzati. Inizio lettura del file di configurazione\n", getpid());

    // Apri il file di configurazione e leggi i parametri
    FILE *file = fopen("../Data/parameters.txt", "r");
    if (file == NULL) {
        perror("Errore nell'apertura del file di configurazione");
        exit(EXIT_FAILURE);
    } else {
        readparameters(file);
    }

    // Stampa informazioni di inizio simulazione
    printf("Master (PID: %d): Parametri letti dal file di configurazione\n", getpid());

    // Creazione della coda di messaggi
    key_t key = 1234;
    if ((msqid = msgget(key, IPC_CREAT | 0666)) < 0) {
        perror("msgget");
        exit(1);
    }

    printf("Master (PID: %d): Inizio creazione atomi iniziali\n", getpid());
    for (int i = 0; i < *N_ATOMI_INIT; i++) {
        createAtomo();
    }
    printf("Master (PID: %d): Fine creazione atomi iniziali\n", getpid());

    // Creazione dei processi necessari
    printf("Master (PID: %d): Creazione del processo alimentatore\n", getpid());
    createAlimentazione();

    printf("Master (PID: %d): Creazione del processo attivatore\n", getpid());
    createAttivatore();

    // Ricezione dei messaggi dai processi
    message_buf rbuf;
    for (int i = 0; i < *N_ATOMI_INIT + 2; i++) { // +2 per attivatore e alimentatore
        if (msgrcv(msqid, &rbuf, sizeof(rbuf.mtext), 1, 0) < 0) {
            perror("msgrcv");
            exit(1);
        }
        printf("Master ha ricevuto: %s\n", rbuf.mtext);
    }

    // Avvia la simulazione
    printf("Master (PID: %d): Inizio simulazione\n", getpid());

    // Attende la terminazione di tutti i figli
    while (wait(NULL) != -1);

    // Pulizia della memoria condivisa
    shm_unlink(shm_name);

    // Rimuove la coda di messaggi
    if (msgctl(msqid, IPC_RMID, NULL) < 0) {
        perror("msgctl");
        exit(1);
    }

    exit(EXIT_SUCCESS);
}