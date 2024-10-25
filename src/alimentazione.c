#include "lib.h"

int msqid; // ID della coda di messaggi

/**
 * Funzione che crea un nuovo processo atomo.
 * Il processo atomo viene creato con un numero atomico casuale.
 */
void createAtomo()
{
    if (*isCleaning == 1)
    {
        printf("[INFO] Alimentazione (PID: %d): Impossibile creare nuovi processi. La fase di cleanup è in corso.\n", getpid());
        return; // Esce dalla funzione senza creare il nuovo processo
    }
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
       // printf("[INFO] Atomo (PID: %d): Avvio processo atomo con numero atomico %d\n", getpid(), numero_atomico);
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
    //    printf("[MESSRIC] Alimentatore (PID: %d) - Message from Atomo: %s\n", getpid(), rbuf.mtext);
    }
}


void handle_sigint(int sig)
{
    (void)sig; // Sopprime l'avviso di parametro inutilizzato

    printf("[INFO] Alimentazione (PID: %d): Ricevuto segnale di terminazione (SIGINT)\n", getpid());


    while (wait(NULL) > 0);
    printf("[INFO] Alimentazione (PID: %d): Terminazione completata\n", getpid());

    exit(EXIT_SUCCESS);
}


/**
 * Funzione per impostare il gestore dei segnali.
 */
void setup_signal_handler()
{
    if (sigaction(SIGINT, &(struct sigaction){.sa_handler = handle_sigint}, NULL) == -1)
    {
        perror("[ERROR] Alimentazione: Errore durante la gestione del segnale di terminazione");
        exit(EXIT_FAILURE);
    }
}

/**
 * Funzione principale del processo "Alimentazione".
 * - Inizializza la memoria condivisa mappando i parametri necessari.
 * - Imposta i gestori dei segnali.
 * - Crea la coda di messaggi per la comunicazione tra i processi e invia un messaggio di inizializzazione completata.
 * - Attende il messaggio di avvio della simulazione tramite la coda di messaggi.
 * - All'avvio della simulazione, entra in un ciclo infinito in cui crea nuovi atomi a ogni ciclo.
 * - Utilizza la funzione `nanosleep` per inserire una pausa (di durata definita da `STEP`) tra le creazioni di atomi.
 * - Se si verifica un errore, il processo termina con un codice di uscita di errore.
 */

int main(int argc, char const *argv[])
{

    printf("[INFO] Alimentatore(PID: %d, GID: %d):Inizio Inizializzazione\n", getpid(), getpgrp());

    void *shm_ptr = allocateParametresMemory();
    if (shm_ptr == MAP_FAILED)
    {
        perror("[ERROR] Alimentazione: Allocazione memoria condivisa fallita");
        exit(EXIT_FAILURE);
    }

    MIN_N_ATOMICO = (int *)(shm_ptr + 2 * sizeof(int));
    N_ATOM_MAX = (int *)(shm_ptr + 3 * sizeof(int));
    STEP = (int *)(shm_ptr + 4 * sizeof(int));
    N_NUOVI_ATOMI = (int *)(shm_ptr + 5 * sizeof(int));
    PID_MASTER = (int *)(shm_ptr + 8 * sizeof(int));
    isCleaning = (int *)(shm_ptr + 9 * sizeof(int));

    setup_signal_handler();

    key_t key = MESSAGE_QUEUE_KEY;
    if ((msqid = msgget(key, MES_PERM_RW_ALL)) < 0)
    {
        perror("msgget");
        exit(1);
    }
    msg_buffer rbuf;
    sem_start = getSemaphoreStartset();
    send_message(msqid, ALIMENTAZIONE_INIT_MSG, "Inizializzazione completata", getpid());
    printf("[INFO] Alimentazione (PID: %d): inizializzazione completata\n", getpid());
    semwait(sem_start);
    semUnlock(sem_start);
    printf("[INFO] Alimentazione (PID: %d): inizio simulazione\n", getpid());
    
    for (;;)
    {
        //printf("[INFO] Alimentazione: Creazione di nuovi atomi\n");
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

    exit(EXIT_FAILURE);
}