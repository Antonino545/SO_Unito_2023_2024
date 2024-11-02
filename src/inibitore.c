#include "lib.h"
#define MEZZOSECONDO 500000000
#define DEFAULT_ABSORBER_RATE 30
int isRunning = 1; // Flag che indica se il processo è in esecuzione
int msqid;         // ID della coda di messaggi
int isBlocked = 0; // Flag to indicate if the inhibitor is blocked
int Scioniblock = 0;
/**
 * Funzione gestore del segnale SIGINT. Viene chiamata quando il processo inibitore riceve il segnale SIGINT,
 * stampando un messaggio di terminazione e chiudendo il processo.
 * @param sig Il segnale ricevuto (SIGINT).
 */
void handle_termination(int sig)
{
    printf("[INFO] Inibitore (PID: %d): Ricevuto segnale di terminazione (SIGINT)\n", getpid());
    printf("[INFO] Inibitore (PID: %d): Riumozione del set di semafori\n", getpid());
    removeSemaphoreSet(sem_inibitore);
    printf("[INFO] Inibitore (PID: %d): Rimozione del set di semafori completata\n", getpid());
    
    (void)sig; // Sopprime l'avviso di parametro inutilizzato
    printf("[INFO] Inibitore (PID: %d): Ricevuto segnale di terminazione (SIGINT)\n", getpid());
    printf("[INFO] Inibitore (PID: %d): Terminazione completata\n", getpid());
    exit(EXIT_SUCCESS);
}
/**
 * Funzione gestore del segnale SIGUSR1. Viene chiamata quando il processo inibitore riceve il segnale SIGUSR1,
 * BLocca il processo inibitore.
 */
void handle_Block(int sig)
{
    (void)sig; // Sopprime l'avviso di parametro inutilizzato
    isBlocked = 1;
    printf("[INFO] Inibitore (PID: %d): Bloccato\n", getpid());
}
/**
 * Funzione gestore del segnale SIGUSR2. Viene chiamata quando il processo inibitore riceve il segnale SIGUSR2,
 * Sblocca il processo inibitore.
 */
void handle_Unlock(int sig)
{
    (void)sig; // Sopprime l'avviso di parametro inutilizzato
    isBlocked = 0;
    printf("[INFO] Inibitore (PID: %d): Sbloccato\n", getpid());
}
/**
 * Funzione che imposta il gestore del segnale SIGINT per il processo inibitore.
 * Associa il segnale SIGINT alla funzione handle_termination.
 * Associa il segnale SIGUSR1 alla funzione handle_Block.
 * Associa il segnale SIGUSR2 alla funzione handle_Unlock.
 */
void setup_signal_handler()
{
    sigaction(SIGTERM, &(struct sigaction){.sa_handler = handle_termination}, NULL);
    sigaction(SIGUSR1, &(struct sigaction){.sa_handler = handle_Block}, NULL);
    sigaction(SIGUSR2, &(struct sigaction){.sa_handler = handle_Unlock}, NULL);
}
/**
 * Funzione principale del processo "Inibitore".
 * - Inizializza la memoria condivisa per accedere ai parametri della simulazione.
 * - Imposta i gestori dei segnali.
 * - Invia un messaggio di inizializzazione completata e attende il segnale di avvio della simulazione.
 * - Una volta avviata la simulazione, se la soglia di esplosione è quasi raggiunta, assorbe energia e al 50% decide se bloccare o sbloccare le scissioni.
 */
int main(int argc, char const *argv[])
{
    printf("[INFO] Inibitore (PID: %d): Inizio inizializzazione\n", getpid());

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
    ENERGY_EXPLODE_THRESHOLD = (int *)(shm_ptr + 7 * sizeof(int));
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
        exit(EXIT_FAILURE);}
    sem_start = getSemaphoreStartset();
    if (sem_start == -1)
    {
        perror("[ERROR] Inibitore: Impossibile ottenere il set di semafori per l'avvio");
        exit(EXIT_FAILURE);
    }
       sem_stats = getSemaphoreStatsSets();
    if (sem_stats == -1)
    {
        perror("[ERROR] Inibitore: Impossibile ottenere il set di semafori per le statistiche");
        exit(EXIT_FAILURE);
    }
    semUnlock(sem_inibitore);

    send_message(msqid, INIBITORE_INIT_MSG, "Inizializzazione completata");
    printf("[INFO] Inibitore (PID: %d): inizializzazione completata\n", getpid());
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
        semwait(sem_stats);
        if(stats->energia_prodotta.totale> (*ENERGY_EXPLODE_THRESHOLD / 2))
        {
            int energy_absorbed = stats->energia_prodotta.ultimo_secondo * DEFAULT_ABSORBER_RATE / 100;
            updateStats(0, 0, -energy_absorbed, 0, 0, energy_absorbed, 0);
            if(rand()%2==0)//ogni mezzo secondo al 50% di probabilità decide se bloccare o sbloccare le scissioni
            {
                if(Scioniblock==0)
                {
                semwait(sem_inibitore);
                Scioniblock=1;
                }
            }else{
                if(Scioniblock==1)
                {
                    semUnlock(sem_inibitore);
                    Scioniblock=0;
                }
            }
        }else{
           if(Scioniblock==1)
           {
               semUnlock(sem_inibitore);
               Scioniblock=0;
               printf("[INFO] Inibitore (PID: %d): Sblocco le scissioni\n", getpid());
           }

        }
        semUnlock(sem_stats);
    
        struct timespec step;
        step.tv_sec = 0;
        step.tv_nsec = MEZZOSECONDO;
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