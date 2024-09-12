#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
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
int *N_ATOM_MAX; // Numero massimo di atomi
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
    int numero_atomico = generate_random(*N_ATOM_MAX); // Genera un numero atomico casuale

    if (pid < 0) { // Errore nella creazione del processo
        perror("[ERROR] Master: Fork fallita durante la creazione di un atomo");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Processo figlio
        char num_atomico_str[20];
        snprintf(num_atomico_str, sizeof(num_atomico_str), "%d", numero_atomico); // Converti il numero atomico in stringa

        printf("[INFO] Atomo (PID: %d): Avvio processo atomo con numero atomico %d\n", getpid(), numero_atomico);

        // Esegui `atomo` con il numero atomico come argomento
        if (execlp("./atomo", "atomo", num_atomico_str, NULL) == -1) {
            perror("[ERROR] Atomo: execlp fallito durante l'esecuzione del processo atomo");
            exit(EXIT_FAILURE);
        }
    } else { // Processo padre
        printf("[INFO] Master (PID: %d): Processo atomo creato con PID: %d e numero atomico: %d\n", getpid(), pid, numero_atomico);
    }
}

/**
 * Crea un nuovo processo figlio che esegue il programma `attivatore`.
 */
void createAttivatore() {
    pid_t pid = fork(); // Crea un nuovo processo

    if (pid < 0) { // Errore nella creazione del processo
        perror("[ERROR] Master: Fork fallita durante la creazione di un attivatore");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Processo figlio
        printf("[INFO] Attivatore (PID: %d): Avvio processo attivatore\n", getpid());

        // Esegui `attivatore`
        if (execlp("./attivatore", "attivatore", NULL) == -1) {
            perror("[ERROR] Attivatore: execlp fallito durante l'esecuzione del processo attivatore");
            exit(EXIT_FAILURE);
        }
    } else { // Processo padre
        printf("[INFO] Master (PID: %d): Processo attivatore creato con PID: %d\n", getpid(), pid);
    }
}

/**
 * Crea un nuovo processo figlio che esegue il programma `alimentazione`.
 */
void createAlimentazione() {
    pid_t pid = fork(); // Crea un nuovo processo

    if (pid < 0) { // Errore nella creazione del processo
        perror("[ERROR] Master: Fork fallita durante la creazione del processo alimentazione");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Processo figlio
        printf("[INFO] Alimentazione (PID: %d): Avvio processo alimentazione\n", getpid());

        // Esegui `alimentazione`
        if (execlp("./alimentazione", "alimentazione", NULL) == -1) {
            perror("[ERROR] Alimentazione: execlp fallito durante l'esecuzione del processo alimentazione");
            exit(EXIT_FAILURE);
        }
    } else { // Processo padre
        printf("[INFO] Master (PID: %d): Processo alimentazione creato con PID: %d\n", getpid(), pid);
    }
}

/**
 * Legge i parametri dal file di configurazione e li memorizza nella memoria condivisa.
 * @param file Puntatore al file di configurazione aperto.
 */
void readparameters(FILE *file) {
    if (file == NULL) { // Verifica che il file sia stato aperto correttamente
        perror("[ERROR] Master: Impossibile aprire il file di configurazione");
        exit(EXIT_FAILURE); 
    }

    char line[256]; // Buffer per leggere ogni linea del file
    while (fgets(line, sizeof(line), file)) { // Legge il file riga per riga
        line[strcspn(line, "\r\n")] = 0; // Rimuove il carattere di newline

        char key[128];
        int value;

        // Parsea la linea nel formato chiave=valore
        if (sscanf(line, "%127[^=]=%d", key, &value) == 2) {
            if (strcmp(key, "N_ATOMI_INIT") == 0) {
                *N_ATOMI_INIT = value;
                printf("[DEBUG] Master: N_ATOMI_INIT impostato a %d\n", value);
            } else if (strcmp(key, "N_ATOM_MAX") == 0) {
                *N_ATOM_MAX = value;
                printf("[DEBUG] Master: N_ATOM_MAX impostato a %d\n", value);
            } else if (strcmp(key, "MIN_N_ATOMICO") == 0) {
                *MIN_N_ATOMICO = value;
                printf("[DEBUG] Master: MIN_N_ATOMICO impostato a %d\n", value);
            } else if (strcmp(key, "ENERGY_DEMAND") == 0) {
                *ENERGY_DEMAND = value;
                printf("[DEBUG] Master: ENERGY_DEMAND impostato a %d\n", value);
            } else if (strcmp(key, "STEP") == 0) {
                *STEP = value;
                printf("[DEBUG] Master: STEP impostato a %d\n", value);
            } else if (strcmp(key, "N_NUOVI_ATOMI") == 0) {
                *N_NUOVI_ATOMI = value;
                printf("[DEBUG] Master: N_NUOVI_ATOMI impostato a %d\n", value);
            } else if (strcmp(key, "SIM_DURATION") == 0) {
                *SIM_DURATION = value;
                printf("[DEBUG] Master: SIM_DURATION impostato a %d\n", value);
            } else if (strcmp(key, "ENERGY_EXPLODE_THRESHOLD") == 0) {
                *ENERGY_EXPLODE_THRESHOLD = value;
                printf("[DEBUG] Master: ENERGY_EXPLODE_THRESHOLD impostato a %d\n", value);
            }
        }
    }

    fclose(file); // Chiude il file dopo aver letto tutti i parametri
}

int main() {
    printf("[INFO] Master (PID: %d): Inizio esecuzione del programma principale\n", getpid());

    // Configura la memoria condivisa
    const char *shm_name = "/Parametres";
    const size_t shm_size = 8 * sizeof(int); // Dimensione della memoria condivisa (8 interi)
    void *shmParamsPtr = setup_shared_memory(shm_name, shm_size);

    // Puntatori alle variabili nella memoria condivisa
    N_ATOMI_INIT = (int *)shmParamsPtr;
    N_ATOM_MAX = (int *)(shmParamsPtr + sizeof(int));
    MIN_N_ATOMICO = (int *)(shmParamsPtr + 2 * sizeof(int));
    ENERGY_DEMAND = (int *)(shmParamsPtr + 3 * sizeof(int));
    STEP = (int *)(shmParamsPtr + 4 * sizeof(int));
    N_NUOVI_ATOMI = (int *)(shmParamsPtr + 5 * sizeof(int));
    SIM_DURATION = (int *)(shmParamsPtr + 6 * sizeof(int));
    ENERGY_EXPLODE_THRESHOLD = (int *)(shmParamsPtr + 7 * sizeof(int));

    printf("[INFO] Master (PID: %d): Memoria condivisa mappata con successo. Inizio lettura del file di configurazione\n", getpid());

    // Apri il file di configurazione e leggi i parametri
    FILE *file = fopen("../Data/parameters.txt", "r");
    if (file == NULL) {
        perror("[ERROR] Master: Impossibile aprire il file di configurazione");
        exit(EXIT_FAILURE);
    } else {
        readparameters(file);
    }

    // Stampa informazioni di inizio simulazione
    printf("[INFO] Master (PID: %d): Parametri letti dal file di configurazione. Avvio simulazione\n", getpid());

    // Creazione della coda di messaggi
    key_t key = 1234;
    if ((msqid = msgget(key, IPC_CREAT | 0666)) < 0) {
        perror("[ERROR] Master: Errore durante la creazione della coda di messaggi (msgget fallita)");
        exit(1);
    }

    printf("[INFO] Master (PID: %d): Inizio creazione atomi iniziali\n", getpid());
    for (int i = 0; i < *N_ATOMI_INIT; i++) {
        createAtomo();
    }
      // Ricezione dei messaggi dai processi
    message_buf rbuf;
    for (int i = 0; i < *N_ATOMI_INIT ; i++) { // +2 per attivatore e alimentatore
        if (msgrcv(msqid, &rbuf, sizeof(rbuf.mtext), 1, 0) < 0) {
            perror("[ERROR] Master: Errore durante la ricezione del messaggio (msgrcv fallita)");
            exit(1);
        }
        printf("[INFO] Master (PID: %d): Mess Ric: %s\n", getpid(), rbuf.mtext);
    }
    printf("[INFO] Master (PID: %d): Fine creazione atomi iniziali\n", getpid());

    // Creazione dei processi necessari
    printf("[INFO] Master (PID: %d): Creazione del processo alimentatore\n", getpid());
    createAlimentazione();

    printf("[INFO] Master (PID: %d): Creazione del processo attivatore\n", getpid());
    createAttivatore();

  

    // Avvio della simulazione principale
    printf("[INFO] Master (PID: %d): Inizio simulazione principale\n", getpid());

    // Attende la terminazione di tutti i processi figli
    while (wait(NULL) != -1);

    // Pulizia della memoria condivisa
    printf("[INFO] Master (PID: %d): Inizio pulizia della memoria condivisa\n", getpid());
    shm_unlink(shm_name);

    // Rimuove la coda di messaggi
    if (msgctl(msqid, IPC_RMID, NULL) < 0) {
        perror("[ERROR] Master: Errore durante la rimozione della coda di messaggi (msgctl fallita)");
        exit(1);
    }

    printf("[INFO] Master (PID: %d): Simulazione terminata con successo. Chiusura programma.\n", getpid());

    exit(EXIT_SUCCESS);
}
