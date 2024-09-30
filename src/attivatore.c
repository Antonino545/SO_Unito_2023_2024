#include "lib.h"

int running = 0; // Flag che indica se il processo è in esecuzione


/**
 * Funzione che gestisce il segnale SIGINT per fermare l'esecuzione del processo atomo.
 * Cambia lo stato della variabile "running" per uscire dal ciclo di attesa.
 */
void handle_sigint(int sig)
{
    printf("[INFO] Attivatore (PID: %d): Ricevuto segnale di terminazione (SIGINT)\n", getpid());
    running = 0; // Imposta running a 0 per terminare il ciclo di attesa
}
/**
 * Funzione che gestisce il segnale SIGUSR2 per iniziare la simulazione.
 */
void handle_start_simulation(int sig) {
    printf("[INFO] Attivatore (PID: %d): Ricevuto segnale di inizio simulazione (SIGUSR2)\n", getpid());
    running = 1;
    // Codice per iniziare la simulazione
}

int main(int argc, char const *argv[]) {
    printf("[INFO] Attivatore (PID: %d): Sono stato appena creato\n", getpid());

    // Ottieni l'ID della coda di messaggi
    key_t key = MESSAGE_QUEUE_KEY;
    int msqid;
    if ((msqid = msgget(key, MES_PERM_RW_ALL)) < 0) {
        perror("msgget");
        exit(1);
    }
    void *shm_ptr = allocateParametresMemory();     
    PID_MASTER = (int *)(shm_ptr + 8 * sizeof(int));
    send_message_to_master( msqid, INIT_MSG,"[INFO] Attivatore (PID: %d): Inizializzazione completata", getpid());
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
    pause(); // Attendi l'arrivo di un segnale
    while (running)
    {
    printf("[INFO] Attivatore (PID: %d): Invio messaggio di scissione agli atomi\n", getpid());
    killpg(*PID_MASTER, SIGUSR2);
    sleep(5);
    }
    wait(NULL); // Aspetta che il processo figlio si concluda, se necessario
    exit(EXIT_SUCCESS);
}
