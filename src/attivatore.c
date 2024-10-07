#include "lib.h"

int running = 1; // Flag che indica se il processo è in esecuzione
int msqid;       // ID della coda di messaggi


void handle_sigint(int sig)
{
    (void)sig; // Suppresses unused parameter warning
    printf("[INFO] Attivatore (PID: %d): Ricevuto segnale di terminazione (SIGINT)\n", getpid());
    while(wait(NULL) > 0);
    printf("[INFO] Attivatore (PID: %d): Terminazione completata\n", getpid());
    exit(EXIT_SUCCESS);

}

// Configurazione dei gestori di segnali
void setup_signal_handler()
{
    sigaction(SIGINT, &(struct sigaction){.sa_handler = handle_sigint}, NULL);
    signal(SIGUSR2, SIG_IGN);
}

int main(int argc, char const *argv[])
{
       setpgid(0, 0);
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

    send_message(msqid,ATTIVATORE_INIT_MSG , "Inizializzazione completata", getpid());
    msg_buffer rbuf;
    //aspetta un messaggio di tipo 5
    if(msgrcv(msqid, &rbuf, sizeof(rbuf.mtext), 5, 0) < 0){
        perror("[ERROR] Attivatore: Errore durante la ricezione del messaggio di inzio divisione");
        exit(EXIT_FAILURE);
    }else{
        printf("[INFO] Attivatore (PID: %d): Ricevuto messaggio di inzio divisione\n", getpid());
    }
    for (;;)
    {
        printf("[INFO] Attivatore (PID: %d): Ordino agli atomi di dividersi\n", getpid());
        killpg(*PID_MASTER, SIGUSR2);
        struct timespec step;
        step.tv_sec = 3;
        step.tv_nsec = 0;

        if (nanosleep(&step, NULL) < 0)
        {
            perror("[ERROR] Attivatore: nanosleep fallito");
            exit(EXIT_FAILURE);
        }
    }

    while(wait(NULL) > 0);
    printf("[INFO] Attivatore (PID: %d): Terminazione completata\n", getpid());
    exit(EXIT_SUCCESS);
}
