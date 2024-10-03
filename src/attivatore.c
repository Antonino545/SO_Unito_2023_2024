#include "lib.h"

int running = 1; // Flag che indica se il processo è in esecuzione
int msqid;       // ID della coda di messaggi


void handle_sigint(int sig)
{
    (void)sig; // Suppresses unused parameter warning
    printf("[INFO] Attivatore (PID: %d): Ricevuto segnale di terminazione (SIGINT)\n", getpid());
    running = 0; // Imposta running a 0 per terminare il ciclo
}

void handle_dividiatomo(int sig)
{
    (void)sig; // Suppresses unused parameter warning
    printf("[INFO] Attivatore (PID: %d): Ricevuto segnale di divisione di un atomo (SIGUSR1)\n", getpid());
}

void setup_signal_handler()
{
    struct sigaction interrupt_sa;
    interrupt_sa.sa_handler = handle_sigint;
    sigemptyset(&interrupt_sa.sa_mask);
    sigaction(SIGINT, &interrupt_sa, NULL);
    struct sigaction sa2;
    sa2.sa_handler = handle_dividiatomo;
    sigemptyset(&sa2.sa_mask);
    sigaction(SIGUSR1, &sa2, NULL);  // Assicurati che il segnale giusto sia utilizzato
}

int main(int argc, char const *argv[])
{
    printf("[INFO] Attivatore: Sono stato appena creato\n");

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

    send_message(msqid,ATTIVATORE_INIT_MSG , "Inizializzazione completata", getpid());
    
    pause();
    for (;;)
    {
        killpg(*PID_MASTER, SIGUSR2);

        struct timespec step;
        step.tv_sec = 0;
        step.tv_nsec = *STEP;

        if (nanosleep(&step, NULL) < 0)
        {
            perror("[ERROR] Attivatore: nanosleep fallito");
            exit(EXIT_FAILURE);
        }
    }


    printf("[INFO] Attivatore (PID: %d): Terminazione completata\n", getpid());
    exit(EXIT_SUCCESS);
}
