#include "lib.h"

int isRunning = 1; // Flag che indica se il processo è in esecuzione
int msqid;         // ID della coda di messaggi

/**
 * Funzione gestore del segnale SIGINT. Viene chiamata quando il processo attivatore riceve il segnale SIGINT,
 * stampando un messaggio di terminazione e chiudendo il processo.
 * @param sig Il segnale ricevuto (SIGINT).
 */
void handle_sigint(int sig)
{
    (void)sig; // Sopprime l'avviso di parametro inutilizzato
    printf("[INFO] Attivatore (PID: %d): Ricevuto segnale di terminazione (SIGINT)\n", getpid());
    printf("[INFO] Attivatore (PID: %d): Terminazione completata\n", getpid());
    exit(EXIT_SUCCESS);
}

/**
 * Funzione che imposta il gestore del segnale SIGINT per il processo attivatore.
 * Associa il segnale SIGINT alla funzione handle_sigint.
 */
void setup_signal_handler()
{
    sigaction(SIGINT, &(struct sigaction){.sa_handler = handle_sigint}, NULL);
}

/**
 * Funzione principale del processo "Attivatore".
 * - Inizializza la memoria condivisa per accedere ai parametri della simulazione.
 * - Imposta i gestori dei segnali.
 * - Invia un messaggio di inizializzazione completata e attende il segnale di avvio della simulazione.
 * - Una volta avviata la simulazione, invia periodicamente il segnale SIGUSR2 al gruppo di processi master,
 *   ordinando l'inizio della divisione degli atomi.
 * - Utilizza nanosleep per inserire una pausa di 1 secondo tra le iterazioni.
 * - Alla terminazione, attende i processi figli e chiude correttamente il processo.
 */
int main(int argc, char const *argv[])
{
    printf("[INFO] Attivatore(PID: %d, GID: %d): Inizializzazione\n", getpid(), getpgrp());

    void *shm_ptr = allocateParametresMemory();
    if (shm_ptr == MAP_FAILED)
    {
        perror("[ERROR] Attivatore: Allocazione memoria condivisa fallita");
        exit(EXIT_FAILURE);
    }

    PID_MASTER = (int *)(shm_ptr + 8 * sizeof(int));
    MIN_N_ATOMICO = (int *)(shm_ptr + 2 * sizeof(int));
    N_ATOM_MAX = (int *)(shm_ptr + 3 * sizeof(int));
    N_NUOVI_ATOMI = (int *)(shm_ptr + 5 * sizeof(int));
    STEP = (int *)(shm_ptr + 4 * sizeof(int));

    setup_signal_handler();

    key_t key = MESSAGE_QUEUE_KEY;
    if ((msqid = msgget(key, MES_PERM_RW_ALL)) < 0)
    {
        perror("msgget");
        exit(1);
    }

    sem_start = getSemaphoreStartset();
    send_message(msqid, ATTIVATORE_INIT_MSG, "Inizializzazione completata", getpid());
    semwait(sem_start);
    semUnlock(sem_start);

    printf("[INFO] Attivatore (PID: %d): inizio simulazione\n", getpid());
    for (;;)
    {
        killpg(*PID_MASTER, SIGUSR2);
        struct timespec step;
        step.tv_sec = 0;
        step.tv_nsec =* STEP;

        if (nanosleep(&step, NULL) < 0)
        {
            perror("[ERROR] Attivatore: nanosleep fallito");
            exit(EXIT_FAILURE);
        }
        printf("[INFO] Attivatore (PID: %d): Ordino agli atomi di scindersi\n", getpid());
    }

    while (wait(NULL) > 0)
        ;
    printf("[INFO] Attivatore (PID: %d): Terminazione completata\n", getpid());
    exit(EXIT_SUCCESS);
}
