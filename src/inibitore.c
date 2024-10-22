#include "lib.h"
#define INIBITORE_SECONDO 1
int isRunning = 1; // Flag che indica se il processo è in esecuzione
int msqid;         // ID della coda di messaggi
int isBlocked = 0; // Flag to indicate if the inhibitor is blocked

void handle_sigint(int sig)
{
    (void)sig; // Sopprime l'avviso di parametro inutilizzato
    printf("[INFO] Inibitore (PID: %d): Ricevuto segnale di terminazione (SIGINT)\n", getpid());
    printf("[INFO] Inibitore (PID: %d): Terminazione completata\n", getpid());
    exit(EXIT_SUCCESS);
}

void handle_Block(int sig)
{
    (void)sig; // Sopprime l'avviso di parametro inutilizzato
    isBlocked = 1;
    printf("[INFO] Inibitore (PID: %d): Bloccato\n", getpid());
}

void handle_Unlock(int sig)
{
    (void)sig; // Sopprime l'avviso di parametro inutilizzato
    isBlocked = 0;
    printf("[INFO] Inibitore (PID: %d): Sbloccato\n", getpid());
}

void setup_signal_handler()
{
    sigaction(SIGINT, &(struct sigaction){.sa_handler = handle_sigint}, NULL);
    sigaction(SIGUSR1, &(struct sigaction){.sa_handler = handle_Block}, NULL);
    sigaction(SIGUSR2, &(struct sigaction){.sa_handler = handle_Unlock}, NULL);
}

int main(int argc, char const *argv[])
{
    printf("[INFO] Inibitore (PID: %d, GID: %d): Sono stato appena creato\n", getpid(), getpgrp());

    void *shm_ptr = allocateParametresMemory();
    if (shm_ptr == MAP_FAILED)
    {
        perror("[ERROR] Inibitore: Allocazione memoria condivisa fallita");
        exit(EXIT_FAILURE);
    }

    PID_MASTER = (int *)(shm_ptr + 8 * sizeof(int));
    MIN_N_ATOMICO = (int *)(shm_ptr + 2 * sizeof(int));
    N_ATOM_MAX = (int *)(shm_ptr + 3 * sizeof(int));
    N_NUOVI_ATOMI = (int *)(shm_ptr + 5 * sizeof(int));
    STEP = (int *)(shm_ptr + 4 * sizeof(int));
    PID_GROUP_ATOMO = (int *)(shm_ptr + 10 * sizeof(int));
    stats = accessStatisticsMemory();

    setup_signal_handler();

    key_t key = MESSAGE_QUEUE_KEY;
    if ((msqid = msgget(key, MES_PERM_RW_ALL)) < 0)
    {
        perror("[ERROR] Inibitore: Impossibile ottenere l'ID della coda di messaggi");
        exit(EXIT_FAILURE);
    }

    sem_inibitore = getSemaphoreInibitoreSet();
    if (sem_inibitore == -1)
    {
        perror("[ERROR] Inibitore: Impossibile ottenere il set di semafori per l'inibitore");
        exit(EXIT_FAILURE);
    }

    sem_start = getSemaphoreStartset();
    sem_stats = getSemaphoreStatsSets();
    if (sem_start == -1)
    {
        perror("[ERROR] Inibitore: Impossibile ottenere il set di semafori per l'avvio");
        exit(EXIT_FAILURE);
    }

    send_message(msqid, INIBITORE_INIT_MSG, "Inizializzazione completata", getpid());

    semwait(sem_start);
    semUnlock(sem_start);

    printf("[INFO] Inibitore (PID: %d): inizio simulazione\n", getpid());

    for (;;)
    {
       
        if (isBlocked)
        {
            printf("[INFO] Inibitore (PID: %d): Bloccato, in attesa di sblocco\n", getpid());
            pause(); // Wait for a signal
            continue; // Skip the rest of the loop if blocked
        }

       

        printf("[INFO] Inibitore (PID: %d): Possibilità di blocco o sblocco\n", getpid());
        if (rand() % 2 == 0)
        {
            semUnlock(sem_inibitore);
        }
        else
        {
            semwait(sem_inibitore);
        }
        semLock(sem_stats);
        int energy_assorbed = stats->energia_prodotta.ultimo_secondo / 10; //assorbe parte del energia prodotta 
        semUnlock(sem_stats);

        printf("[INFO] Inibitore (PID: %d): Assorbo %d di energia\n", getpid(), energy_assorbed);
        updateStats(0, 0, -energy_assorbed, 0, 0, energy_assorbed, 0);
         struct timespec step;
        step.tv_sec = INIBITORE_SECONDO;
        step.tv_nsec = 0;
        if (nanosleep(&step, NULL) < 0)
        {
            perror("[ERROR] Inibitore: nanosleep fallito");
            exit(EXIT_FAILURE);
        }

    }

    while (wait(NULL) > 0);
    printf("[INFO] Inibitore (PID: %d): Terminazione completata\n", getpid());
    exit(EXIT_SUCCESS);
}