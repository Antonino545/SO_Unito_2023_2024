#include "lib.h"

int running = 0; // Flag che indica se il processo è in esecuzione
int msqid;       // ID della coda di messaggi

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
        waitForNInitMsg(msqid, 1);
    }
}

void handle_sigint(int sig)
{
    (void)sig; // Suppresses unused parameter warning
    printf("[INFO] Alimentazione (PID: %d): Ricevuto segnale di terminazione (SIGINT)\n", getpid());
    running = 0; // Imposta running a 0 per terminare il ciclo
}

void handle_createAtomo(int sig)
{
    printf("[INFO] Alimentazione (PID: %d): Ricevuto segnale di creazione di nuovi atomi (SIGUSR1)\n", getpid());
    running = 1; // Imposta running a 1 per iniziare a generare atomi

    // Inizio immediato del ciclo di generazione atomi
    while (running)
    {
        for (int i = 0; i < *N_NUOVI_ATOMI; i++)
        {
            createAtomo();
        }

        struct timespec step;
        step.tv_sec = 0;
        step.tv_nsec = *STEP;
        nanosleep(&step, NULL); // Ogni step nanosecondi crea nuovi atomi
    }
}

void setup_signal_handler()
{
    struct sigaction interrupt_sa;
    interrupt_sa.sa_handler = handle_sigint;
    sigemptyset(&interrupt_sa.sa_mask);
    sigaction(SIGINT, &interrupt_sa, NULL);

    struct sigaction sa2;
    sa2.sa_handler = handle_createAtomo;
    sigemptyset(&sa2.sa_mask);
    sigaction(SIGUSR1, &sa2, NULL);  // Assicurati che il segnale giusto sia utilizzato
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

    setup_signal_handler();

    key_t key = MESSAGE_QUEUE_KEY;
    if ((msqid = msgget(key, MES_PERM_RW_ALL)) < 0)
    {
        perror("msgget");
        exit(1);
    }

    send_message(msqid,ATTIVATORE_INIT_MSG , "Inizializzazione completata", getpid());
    
    // Aspetta un segnale prima di iniziare a generare
    while (1)
    {
        pause(); // Aspetta che arrivi un segnale
    }

    printf("[INFO] Alimentazione (PID: %d): Terminazione completata\n", getpid());
    exit(EXIT_SUCCESS);
}
