#include "lib.h"
#include <errno.h>

int sem_id;                    // ID del semaforo
Statistiche *stats;            // Statistiche della simulazione in memoria condivisa
int *N_ATOMI_INIT;             /** Numero iniziale di atomi */
int *N_ATOM_MAX;               /** Numero massimo del numero atomico dell'atomo */
int *MIN_N_ATOMICO;            /** Numero atomico minimo */
int *ENERGY_DEMAND;            /** Domanda di energia */
int *STEP;                     /** Passo per la variazione dell'energia */
int *N_NUOVI_ATOMI;            /** Numero di nuovi atomi */
int *SIM_DURATION;             /** Durata della simulazione */
int *ENERGY_EXPLODE_THRESHOLD; /** Soglia di esplosione dell'energia */
int *PID_MASTER;               /** PID del processo master */
int *ATOMO_GPID;               /** Gruppo di processi degli atomi */
int generate_random(int max)
{
    return rand() % max + 1; // Restituisce un numero tra 1 e max
}

void *create_shared_memory(const char *shm_name, size_t shm_size)
{
    // Crea o apre una memoria condivisa
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, MES_PERM_RW_ALL);
    if (shm_fd == -1)
    {
        perror("[ERROR] Master: Errore durante la creazione della memoria condivisa (shm_open fallita)");
        exit(EXIT_FAILURE);
    }

    // Imposta la dimensione della memoria condivisa
    if (ftruncate(shm_fd, shm_size) == -1)
    {
        perror("[ERROR] Master: Impossibile impostare la dimensione della memoria condivisa (ftruncate fallita)");
        exit(EXIT_FAILURE);
    }

    // Mappa la memoria condivisa nel proprio spazio degli indirizzi
    void *shm_ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
    {
        perror("[ERROR] Master: Errore durante la mappatura della memoria condivisa (mmap fallita)");
        exit(EXIT_FAILURE);
    }

    return shm_ptr;
}

void *allocateParametresMemory()
{
    const char *shm_name = "/Parametres";
    const size_t shm_size = 8 * sizeof(int); // 8 interi

    int shm_fd = shm_open(shm_name, O_RDWR, MES_PERM_RW_ALL); // Apre la memoria condivisa in lettura e scrittura che è già stata creata
    if (shm_fd == -1)
    {
        perror("Errore nella shm_open");
        exit(EXIT_FAILURE);
    }

    void *shm_ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
    {
        perror("Errore nella mmap");
        exit(EXIT_FAILURE);
    }
    return shm_ptr;
}

void *allocateStatisticsMemory()
{
    const char *shm_name = "/Statistics";        // Name for statistics shared memory
    const size_t shm_size = sizeof(Statistiche); // Size based on structure

    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("Error in shm_open for statistics");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, shm_size) == -1)
    {
        perror("Error in ftruncate for statistics");
        exit(EXIT_FAILURE);
    }

    void *shm_ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
    {
        perror("Error in mmap for statistics");
        exit(EXIT_FAILURE);
    }

    // Initialize statistics
    Statistiche *stats = (Statistiche *)shm_ptr;
    memset(stats, 0, shm_size); // Zero out the statistics structure

    return stats; // Return pointer to the initialized structure
}

/**
 * Funzione per bloccare il semaforo.
 */
void semLock(int sem_id)
{
    struct sembuf sb = {0, -1, 0}; // Operazione di lock
    if (semop(sem_id, &sb, 1) == -1)
    {
        perror("semop lock");
        exit(EXIT_FAILURE);
    }
}

/**
 * Funzione per sbloccare il semaforo.
 */
void semUnlock(int sem_id)
{
    struct sembuf sb = {0, 1, 0}; // Operazione di unlock
    if (semop(sem_id, &sb, 1) == -1)
    {
        perror("semop unlock");
        exit(EXIT_FAILURE);
    }
}

/**
 * Funzione per aggiornare le statistiche, protetta da semafori.
 * Gli altri processi chiameranno questa funzione per aggiornare i dati.
 */
void updateStats(int attivazioni, int scissioni, int energia_prod, int energia_cons, int scorie)
{
    semLock(sem_id); // Blocco del semaforo

    stats->Nattivazioni.totale += attivazioni;
    stats->Nattivazioni.ultimo_secondo = attivazioni;
    stats->Nscissioni.totale += scissioni;
    stats->Nscissioni.ultimo_secondo = scissioni;
    stats->energia_prodotta.totale += energia_prod;
    stats->energia_prodotta.ultimo_secondo = energia_prod;
    stats->energia_consumata.totale += energia_cons;
    stats->energia_consumata.ultimo_secondo = energia_cons;
    stats->scorie_prodotte.totale += scorie;
    stats->scorie_prodotte.ultimo_secondo = scorie;

    semUnlock(sem_id); // Sblocco del semaforo
}

void send_message(int msqid, long type, const char *format, ...)
{
    msg_buffer message;
    message.mtype = type;

    // Inizializza gli argomenti variabili
    va_list args;
    va_start(args, format);

    // Usa vsnprintf per formattare il messaggio
    vsnprintf(message.mtext, sizeof(message.mtext), format, args);

    // Termina l'uso degli argomenti variabili
    va_end(args);

    int attempts = 0;
    while (msgsnd(msqid, &message, sizeof(message.mtext), IPC_NOWAIT) == -1)
    {
        if (errno == EAGAIN)
        {
            if (attempts < 5)
            {
                attempts++;
                usleep(100000); // Attende 100ms prima di ritentare
            }
            else
            {
                perror("Errore msgsnd: impossibile inviare il messaggio");
                break;
            }
        }
        else
        {
            perror("Errore msgsnd: impossibile inviare il messaggio");
            break;
        }
    }
}

void waitForNInitMsg(int msqid, int n)
{
    msg_buffer rbuf;
    for (int i = 0; i < n; i++)
    {
        // Receive any message
        if (msgrcv(msqid, &rbuf, sizeof(rbuf.mtext), 0, 0) < 0)
        {
            perror("[Error] PID: %d - Errore durante la ricezione del messaggio di inizializzazione");
            exit(EXIT_FAILURE);
        }

        // Display the correct message based on the type
        if (rbuf.mtype == ATOMO_INIT_MSG)
        {
            printf("[MESSRIC] Master (PID: %d) - Message from Atomo: %s\n", getpid(), rbuf.mtext);
        }
        else if (rbuf.mtype == ATTIVATORE_INIT_MSG)
        {
            printf("[MESSRIC] Master (PID: %d) - Message from Attivatore: %s\n", getpid(), rbuf.mtext);
        }
        else if (rbuf.mtype == ALIMENTAZIONE_INIT_MSG)
        {
            printf("[MESSRIC] Master (PID: %d) - Message from Alimentazione: %s\n", getpid(), rbuf.mtext);
        }
        else
        {
            printf("[MESSRIC] Master (PID: %d) - Message from Unknown Sender: %s\n", getpid(), rbuf.mtext);
        }
    }
}

/**
 * Invia un segnale di inizio simulazione ai processi attivatore e alimentatore.
 */
void sendStartSimulationSignal(pid_t attivatore_pid, pid_t alimentazione_pid)
{
    if (kill(attivatore_pid, SIGUSR2) == -1)
    {
        perror("[ERROR] Master: Impossibile inviare il segnale di inizio simulazione all'attivatore");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("[INFO] Master (PID: %d): Segnale di inizio simulazione inviato a attivatore\n", getpid());
    }
    if (kill(alimentazione_pid, SIGUSR2) == -1)
    {
        perror("[ERROR] Master: Impossibile inviare il segnale di inizio simulazione all'alimentazione");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("[INFO] Master (PID: %d): Segnale di inizio simulazione inviato a alimentazione\n", getpid());
    }
}

int getSemaphoreSet()
{

    int semid = semget(SEMAPHORE_KEY, 1, IPC_CREAT | 0666); // Create a semaphore set with 1 semaphore
    if (semid == -1)
    {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    // Inizializza il semaforo a 1 (semaforo binario)
    if (semctl(semid, 0, SETVAL, 1) == -1)
    {
        perror("semctl");
        exit(EXIT_FAILURE);
    }

    return semid;
}

void removeSemaphoreSet(int semid)
{
    if (semctl(semid, 0, IPC_RMID) == -1)
    {
        perror("semctl");
        exit(EXIT_FAILURE);
    }
}

void P(int semid)
{
    struct sembuf p_op = {0, -1, 0}; // Operazione di wait
    if (semop(semid, &p_op, 1) == -1)
    {
        perror("P operation");
        exit(EXIT_FAILURE);
    }
}

void V(int semid)
{
    struct sembuf v_op = {0, 1, 0}; // Operazione di signal
    if (semop(semid, &v_op, 1) == -1)
    {
        perror("V operation");
        exit(EXIT_FAILURE);
    }
}