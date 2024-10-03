#include "lib.h"

int msqid; // ID della coda di messaggi

void createAtomo()
{
    int numero_atomico = generate_random(*N_ATOM_MAX);
    char num_atomico_str[20];
    snprintf(num_atomico_str, sizeof(num_atomico_str), "%d", numero_atomico);

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("[ERROR] Alimentatore: Fork fallita durante la creazione di un atomo");
        kill(*PID_MASTER, SIGUSR1);
    }
    else if (pid == 0)
    {
        printf("[INFO] Atomo (PID: %d): Avvio processo atomo con numero atomico %d\n", getpid(), numero_atomico);
        if (execlp("./atomo", "atomo", num_atomico_str, NULL) == -1)
        {
            perror("[ERROR] Atomo: execlp fallito durante l'esecuzione del processo atomo");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        msg_buffer rbuf;

        if (msgrcv(msqid, &rbuf, sizeof(rbuf.mtext), 0, 0) < 0)
        {
            perror("[Error] PID: %d - Errore durante la ricezione del messaggio di inizializzazione");
            exit(EXIT_FAILURE);
        }
        printf("[MESSRIC] Alimentatore (PID: %d) - Message from Atomo: %s\n", getpid(), rbuf.mtext);
    }
}

void handle_sigint(int sig)
{
    (void)sig; // Suppresses unused parameter warning
    
    printf("[INFO] Alimentazione (PID: %d): Ricevuto segnale di terminazione (SIGINT)\n", getpid());
    printf("[INFO] Alimentazione (PID: %d): Terminazione completata\n", getpid());
    exit(EXIT_SUCCESS);
}
void handle_sigusr1(int sig)
{
    (void)sig; // Suppresses unused parameter warning
    printf("[INFO] Alimentazione (PID: %d): Ricevuto segnale di start (SIGUSR1)\n", getpid());
}
void setup_signal_handler()
{
    struct sigaction interrupt_sa;
    interrupt_sa.sa_handler = handle_sigint;
    sigemptyset(&interrupt_sa.sa_mask);
    sigaction(SIGINT, &interrupt_sa, NULL);
    struct sigaction sa_start;
    sa_start.sa_handler = handle_sigusr1;
    sigemptyset(&sa_start.sa_mask);
    sigaction(SIGUSR1, &sa_start, NULL);
    
}

int main(int argc, char const *argv[])
{
    printf("[INFO] Alimentazione: Sono stato appena creato\n");

    void *shm_ptr = allocateParametresMemory();
    if (shm_ptr == MAP_FAILED)
    {
        perror("[ERROR] Alimentazione: Allocazione memoria condivisa fallita");
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

    send_message(msqid, ALIMENTAZIONE_INIT_MSG, "Inizializzazione completata", getpid());
    pause();
    for (;;)
    {
        for (int i = 0; i < *N_NUOVI_ATOMI; i++)
        {
            createAtomo();
        }

        struct timespec step;
        step.tv_sec = 0;
        step.tv_nsec = *STEP;

        if (nanosleep(&step, NULL) < 0)
        {
            perror("[ERROR] Alimentazione: nanosleep fallito");
            exit(EXIT_FAILURE);
        }
    }
                while (wait(NULL) > 0); // Aspetta che tutti i processi figli terminino

    exit(EXIT_FAILURE);
}