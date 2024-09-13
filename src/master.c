#include "lib.h"
#include <errno.h>

// Definizione della struttura del messaggio
typedef struct {
    int n_atom;
    pid_t pid;
} atom;

// Definizione delle variabili come puntatori che punteranno a memoria condivisa
int *N_ATOMI_INIT;
int *N_ATOM_MAX; // Numero massimo di atomi
int *MIN_N_ATOMICO;
int *ENERGY_DEMAND;
int *STEP;
int *N_NUOVI_ATOMI;
int *SIM_DURATION;
int *ENERGY_EXPLODE_THRESHOLD;
int energy = 0;
int msqid;
int cleanup_flag = 0; // Global flag for cleanup

void cleanup() {
    if (cleanup_flag) {
        printf("[CLEANUP] Master (PID: %d): Avvio della pulizia\n", getpid());

        // Attende la terminazione di tutti i processi figli
        while (wait(NULL) != -1);
        const char  *shm_name="/Parametres";
        // Pulizia della memoria condivisa
        printf("[INFO] Master (PID: %d): Inizio pulizia della memoria condivisa\n", getpid());
        if (shm_unlink(shm_name) == -1) {
            perror("Errore durante shm_unlink");
        } else {
            printf("[INFO] Master (PID: %d): Memoria condivisa pulita con successo\n", getpid());
        }

        // Rimuove la coda di messaggi
        if (msgctl(msqid, IPC_RMID, NULL) < 0) {
            perror("[ERROR] Master: Errore durante la rimozione della coda di messaggi (msgctl fallita)");
        }
    }
}

/**
 * Crea un nuovo processo figlio che esegue il programma `atomo` con un numero atomico casuale come argomento.
 */
void createAtomo() {
    pid_t pid = fork(); // Crea un nuovo processo
    int numero_atomico = generate_random(*N_ATOM_MAX); // Genera un numero atomico casuale

    if (pid < 0) { // Errore nella creazione del processo
        perror("[ERROR] Master: Fork fallita durante la creazione di un atomo");
        cleanup(); // Call cleanup function
        printf("[TERMINATION] Master (PID: %d): Terminazione della simulazione  per meltdown\n", getpid());
        exit(EXIT_SUCCESS);
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
        printf("[INFO] Master (PID: %d): Processo atomo creato con PID: %d\n", getpid(), pid);
    }
}

/**
 * Crea un nuovo processo figlio che esegue il programma `attivatore`.
 */
void createAttivatore() {
    pid_t pid = fork(); // Crea un nuovo processo

    if (pid < 0) { // Errore nella creazione del processo
          perror("[ERROR] Master: Fork fallita durante la creazione di un atomo");
        cleanup(); // Call cleanup function
        printf("[TERMINATION] Master (PID: %d): Terminazione della simulazione  per meltdown\n", getpid());
        exit(EXIT_SUCCESS);
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
  perror("[ERROR] Master: Fork fallita durante la creazione di un atomo");
        cleanup(); // Call cleanup function
        printf("[TERMINATION] Master (PID: %d): Terminazione della simulazione  per meltdown\n", getpid());
        exit(EXIT_SUCCESS);
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

void checkForMeltdownMessage(int msqid) {
    msg_buffer rbuf;
    ssize_t result;

    // Attempt to receive a message from the queue
    result = msgrcv(msqid, &rbuf, sizeof(rbuf.mtext), 2, IPC_NOWAIT);

    if (result == -1) {
        if (errno != ENOMSG) { // Check if the error is not "No message of the desired type"
            perror("Errore msgrcv: impossibile ricevere il messaggio");
        }
        // If errno is ENOMSG, simply return as there are no messages to process
        return;
    }

    printf("[TERMINATION] per meltdown: %s\n", rbuf.mtext);

    // Set cleanup flag and perform cleanup
    cleanup_flag = 1; // Set the flag for cleanup
    cleanup(); // Call cleanup function

    // Optionally exit if a critical meltdown message is received
    exit(EXIT_SUCCESS);
}

int main() {
    printf("[INFO] Master (PID: %d): Inizio esecuzione del programma principale\n", getpid());

    // Configura la memoria condivisa
    const char *shm_name = "/Parametres";
    const size_t shm_size = 8 * sizeof(int); // Dimensione della memoria condivisa (8 interi)
    void *shmParamsPtr = create_shared_memory(shm_name, shm_size);

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
    printf("[INFO] Master (PID: %d): Parametri letti dal file di configurazione. Inizio creazione Proccessi iniziali\n", getpid());

    // Creazione della coda di messaggi
    key_t key = MESSAGE_QUEUE_KEY;
    if ((msqid = msgget(key, IPC_CREAT | 0666)) < 0) {
        perror("[ERROR] Master: Errore durante la creazione della coda di messaggi (msgget fallita)");
        exit(EXIT_FAILURE);
    }

    printf("[INFO] Master (PID: %d): Inizio creazione atomi iniziali\n", getpid());
    for (int i = 0; i < *N_ATOMI_INIT; i++) {
        createAtomo();
    }

    // Ricezione dei messaggi dai processi
    waitForNInitMsg(msqid, *N_ATOMI_INIT);

    printf("[INFO] Master (PID: %d): Fine creazione atomi iniziali\n", getpid());
    // Creazione dei processi necessari
    printf("[INFO] Master (PID: %d): Creazione del processo alimentatore\n", getpid());
    createAlimentazione();
    waitForNInitMsg(msqid, 1);
    printf("[INFO] Master (PID: %d): Creazione del processo attivatore\n", getpid());
    createAttivatore();
    waitForNInitMsg(msqid, 1);

    // Avvio della simulazione principale
    printf("[IMPORTANT] Master (PID: %d): Processi creati con successo. Inizio simulazione principale\n", getpid());

    while (*SIM_DURATION > 0) {
        printf("[INFO] SIM_DURATION attuale: %d\n", *SIM_DURATION);

        // Check for meltdown messages
        checkForMeltdownMessage(msqid);

        // Esegui l'azione desiderata qui, ad esempio una pausa di 1 secondo
        (*SIM_DURATION)--;
        sleep(1);
    }

    // Attende la terminazione di tutti i processi figli
    while (wait(NULL) != -1);

    // Pulizia della memoria condivisa
    printf("[INFO] Master (PID: %d): Inizio pulizia della memoria condivisa\n", getpid());
    if (shm_unlink(shm_name) == -1) {
        perror("Errore durante shm_unlink");
    } else {
        printf("[INFO] Master (PID: %d): Memoria condivisa pulita con successo\n", getpid());
    }

    // Rimuove la coda di messaggi
    if (msgctl(msqid, IPC_RMID, NULL) < 0) {
        perror("[ERROR] Master: Errore durante la rimozione della coda di messaggi (msgctl fallita)");
    }

    printf("[TERMINATION] Master (PID: %d): Simulazione terminata con successo. Chiusura programma.\n", getpid());

    exit(EXIT_SUCCESS);
}
