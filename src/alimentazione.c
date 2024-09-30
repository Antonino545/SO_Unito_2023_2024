#include "lib.h"

int running = 0; // Flag che indica se il processo è in esecuzione
int msqid; // ID della coda di messaggi
/**
 * Funzione che gestisce il segnale SIGINT per fermare l'esecuzione del processo atomo.
 * Cambia lo stato della variabile "running" per uscire dal ciclo di attesa.
 */
void handle_sigint(int sig)
{
     (void)sig;  // Suppresses unused parameter warning
    printf("[INFO] Alimentazione (PID: %d): Ricevuto segnale di terminazione (SIGINT)\n", getpid());
    running = 0; // Imposta running a 0 per terminare il ciclo di attesa
}
/**
 * Funzione che gestisce il segnale SIGUSR2 per iniziare la simulazione.
 */
void handle_start_simulation(int sig) {
    printf("[INFO] Alimentazione (PID: %d): Ricevuto segnale di inizio simulazione (SIGUSR2)\n", getpid());
    running = 1;
    // Codice per iniziare la simulazione
}

/*
 * Crea un nuovo processo figlio per eseguire il programma `atomo` con un numero atomico casuale.
 */
void createAtomo() {
    int numero_atomico = generate_random(*N_ATOM_MAX);
    char num_atomico_str[20];
    snprintf(num_atomico_str, sizeof(num_atomico_str), "%d", numero_atomico);
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("[ERROR] Alimentatore: Fork fallita durante la creazione di un atomo");
        kill(*PID_MASTER, SIGUSR1);
    } else if (pid == 0) {
        printf("[INFO] Atomo (PID: %d): Avvio processo atomo con numero atomico %d\n", getpid(), numero_atomico);
        if (execlp("./atomo", "atomo", num_atomico_str, NULL) == -1) {
            perror("[ERROR] Atomo: execlp fallito durante l'esecuzione del processo atomo");
            exit(EXIT_FAILURE);
        }
    } else {
        //il padre aspetta il messaggio di inizializzazione del figlio
        waitForNInitMsg(msqid, 1);
    }

}
void setup_signal_handler(){

    struct sigaction sa_int;
    bzero(&sa_int, sizeof(sa_int));
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sigaction(SIGINT, &sa_int, NULL);
        struct sigaction sa_start_simulation;
    bzero(&sa_start_simulation, sizeof(sa_start_simulation));
    sa_start_simulation.sa_handler = handle_start_simulation;
    sigemptyset(&sa_start_simulation.sa_mask);
    sigaction(SIGUSR2, &sa_start_simulation, NULL);
}

int main(int argc, char const *argv[]) {
    printf("[INFO] Alimentazione: Sono stato appena creato\n");
    void* shm_ptr= allocateParametresMemory(); // Inizializza la memoria condivisa
    PID_MASTER = (int *)(shm_ptr + 8 * sizeof(int)); // Recupera il PID del processo master dalla memoria condivisa
    MIN_N_ATOMICO = (int *)(shm_ptr + 2 * sizeof(int)); // Recupera il valore di MIN_N_ATOMICO dalla memoria condivisa
    N_ATOM_MAX = (int *)(shm_ptr + 3 * sizeof(int)); // Recupera il valore di N_ATOM_MAX dalla memoria condivisa
    N_NUOVI_ATOMI = (int *)(shm_ptr + 5 * sizeof(int)); // Recupera il numero di nuovi atomi dalla memoria condivisa
    setup_signal_handler(); // Imposta il gestore del segnale per il SIGINT
    // Invia un messaggio al master
    key_t key = MESSAGE_QUEUE_KEY;
    if ((msqid = msgget(key, MES_PERM_RW_ALL)) < 0) {
        perror("msgget");
        exit(1);
    }

    send_message( msqid,INIT_MSG, "[INFO] Alimentazione (PID: %d): Inizializzazione completata", getpid());

    pause(); // Attendi l'arrivo di un segnale
    while (running)
    {
        for(int i=0; i<*N_NUOVI_ATOMI; i++){
        printf("[INFO] Alimentazione (PID: %d): Creazione di un nuovo atomo\n", getpid());
        createAtomo(); // Crea un nuovo atomo
        }
        nanosleep((const struct timespec[]){{1, 0}}, NULL); // Aspetta 1 secondo
    }
    
    wait(NULL); // Aspetta che il processo figlio si concluda, se necessario
    exit(EXIT_SUCCESS);
}