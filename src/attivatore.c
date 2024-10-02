#include "lib.h"

int running = 1; // Flag che indica se il processo è in esecuzione

/**
 * Funzione che gestisce il segnale SIGINT per fermare l'esecuzione del processo atomo.
 */
void handle_sigint(int sig)
{
    printf("[INFO] Attivatore (PID: %d): Ricevuto segnale di terminazione (SIGINT)\n", getpid());
    running = 0; // Imposta running a 0 per terminare il ciclo di attesa
}

/**
 * Funzione che gestisce il segnale SIGUSR1 per iniziare la scissione.
 */
void handle_inizia_Messaggi_scissione(int sig)
{
    printf("[INFO] Attivatore (PID: %d): Ricevuto segnale di inizio scissione (SIGUSR1)\n", getpid());
    killpg(*PID_MASTER, SIGUSR2); // Invia SIGUSR2 a tutti i processi nel gruppo
    
}

/**
 * Configura il gestore per i segnali specificati.
 */
void setup_signal_handlers()
{
    struct sigaction sa;
    sa.sa_flags = 0; // Nessun flag
    sigemptyset(&sa.sa_mask);

    // Gestione SIGINT
    sa.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa, NULL);

    // Gestione SIGUSR1
    sa.sa_handler = handle_inizia_Messaggi_scissione;
    sigaction(SIGUSR1, &sa, NULL);
}

int main(int argc, char const *argv[])
{
    // Ottieni l'ID della coda di messaggi
    key_t key = MESSAGE_QUEUE_KEY;
    int msqid = msgget(key, MES_PERM_RW_ALL);
    if (msqid < 0)
    {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    
    void *shm_ptr = allocateParametresMemory();
    if (shm_ptr == MAP_FAILED)
    {
        perror("Failed to allocate shared memory");
        exit(EXIT_FAILURE);
    }

    PID_MASTER = (int *)(shm_ptr + 8 * sizeof(int));
    
    send_message(msqid, ATTIVATORE_INIT_MSG, "[INFO] Attivatore (PID: %d): Inizializzazione completata", getpid());



    // Ciclo di attesa per i segnali
    while (running)
    {
        pause(); // Aspetta un segnale
    }
    printf("[INFO] Attivatore (PID: %d): Terminazione completata\n", getpid());
    exit(EXIT_SUCCESS);
}
