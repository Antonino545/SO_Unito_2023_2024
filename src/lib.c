#include "lib.h"
#include <stdbool.h>

int *N_ATOMI_INIT;             // Numero iniziale di atomi
int *N_ATOM_MAX;               // Numero atomico massimo
int *MIN_N_ATOMICO;            // Numero atomico minimo
int *ENERGY_DEMAND;            // Domanda di energia
int *STEP;                     // Passo per la variazione dell'energia
int *N_NUOVI_ATOMI;            // Numero di nuovi atomi
int *SIM_DURATION;             // Durata della simulazione
int *ENERGY_EXPLODE_THRESHOLD; // Soglia di esplosione dell'energia
int *PID_MASTER;               // PID del processo master
int *ATOMO_GPID;               // Gruppo di processi degli atomi
int *isCleaning;               // flag che indica se la pulizia è in corso
Statistiche *stats;            // Statistiche della simulazione
int sem_stats;                 // ID del semaforo per le statistiche
int sem_start;                 // ID del semaforo per l'avvio della simulazione
int sem_inibitore;             // ID del semaforo per l'inibitore
int *PID_GROUP_ATOMO;          // PID del gruppo di processi degli atomi
int *isinibitoreactive;

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
    const size_t shm_size = 8 * sizeof(int);

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

void *initializeStatisticsMemory()
{
    const char *shm_name = "/Statistics";
    const size_t shm_size = sizeof(Statistiche);

    // Apre la memoria condivisa
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("Error in shm_open for statistics");
        exit(EXIT_FAILURE);
    }

    // Imposta la dimensione della memoria condivisa
    if (ftruncate(shm_fd, shm_size) == -1)
    {
        perror("Error in ftruncate for statistics");
        exit(EXIT_FAILURE);
    }

    // Mappa la memoria condivisa
    void *shm_ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
    {
        perror("Error in mmap for statistics");
        exit(EXIT_FAILURE);
    }

    Statistiche *stats = (Statistiche *)shm_ptr;
    memset(stats, 0, shm_size); // Azzeramento della struttura delle statistiche

    return stats;
}

Statistiche *accessStatisticsMemory()
{
    const char *shm_name = "/Statistics";

    // Apre la memoria condivisa esistente
    int shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("Error in shm_open for accessing statistics");
        exit(EXIT_FAILURE);
    }

    // Mappa la memoria condivisa
    size_t shm_size = sizeof(Statistiche);
    void *shm_ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
    {
        perror("Error in mmap for accessing statistics");
        exit(EXIT_FAILURE);
    }

    return (Statistiche *)shm_ptr;
}

int getSemaphoreStatsSets()
{
    int semid = semget(SEMAPHORE_STATS_KEY, 1, IPC_CREAT | 0666); // crea un set di semafori con un semaforo
    if (semid == -1)
    {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    // Inizializza il semaforo a 1
    if (semctl(semid, 0, SETVAL, 1) == -1)
    {
        perror("Semctl erro in SemaphoreStatsSets");
        exit(EXIT_FAILURE);
    }

    return semid;
}

int getSemaphoreStartset()
{
    int semid = semget(SEMAPHORE_START_KEY, 1, IPC_CREAT | 0666);
    if (semid == -1)
    {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    if (semctl(semid, 0, SETVAL, 0) == -1)
    {
        perror("semctl");
        exit(EXIT_FAILURE);
    }

    return semid;
}

int getSemaphoreInibitoreSet()
{
    int semid = semget(SEMAPHORE_INIBITORE_KEY, 1, IPC_CREAT | 0666);
    if (semid == -1)
    {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    if (semctl(semid, 0, SETVAL, 1) == -1)
    {
        perror("semctl in getSemaphoreInibitoreSet");
        exit(EXIT_FAILURE);
    }
    return semid;
}

void removeSemaphoreSet(int semid)
{
    if (semctl(semid, 0, IPC_RMID) == -1)
    {
        perror("Error nel rimuovere il set di semafori ");
        exit(EXIT_FAILURE);
    }
}



void semUnlock(int semid)
{
    struct sembuf sb = {0, 1, 0}; // Operazione di unlock
    if (semop(semid, &sb, 1) == -1)
    {
        perror("semop unlock");
        exit(EXIT_FAILURE);
    }
}

void semwait(int semid)
{
    struct sembuf sb = {0, -1, 0};
    if (semop(semid, &sb, 1) == -1)
    {
        perror("semop wait");
        exit(EXIT_FAILURE);
    }
}

void updateStats(int attivazioni, int scissioni, int energia_prod, int energia_cons, int scorie, int energia_assorbita, int bilanciamento)
{
    sem_stats = getSemaphoreStatsSets();

    semwait(sem_stats);

    stats->Nattivazioni.totale += attivazioni;
    stats->Nattivazioni.ultimo_secondo += attivazioni;
    stats->Nscissioni.totale += scissioni;
    stats->Nscissioni.ultimo_secondo += scissioni;
    stats->energia_prodotta.totale += energia_prod;
    stats->energia_prodotta.ultimo_secondo += energia_prod;
    stats->energia_consumata.totale += energia_cons;
    stats->energia_consumata.ultimo_secondo += energia_cons;
    stats->scorie_prodotte.totale += scorie;
    stats->scorie_prodotte.ultimo_secondo += scorie;
    stats->energia_assorbita.totale += energia_assorbita;
    stats->energia_assorbita.ultimo_secondo += energia_assorbita;
    stats->bilanciamento.totale += bilanciamento;
    stats->bilanciamento.ultimo_secondo += bilanciamento;

    semUnlock(sem_stats);
}
void send_message(int msqid, long type, char *messagetext) {
    msg_buffer message;
    message.mtype = type;
    strncpy(message.mtext, messagetext, sizeof(message.mtext) - 1); // Ensure null termination
    message.mtext[sizeof(message.mtext) - 1] = '\0';

    int attempts = 0;
    while (msgsnd(msqid, &message, sizeof(message.mtext), IPC_NOWAIT) == -1) {
        if (errno == EAGAIN) {
            if (attempts < 5) {
                attempts++;
                usleep(100000); // 100 ms
            } else {
                perror("Errore msgsnd: impossibile inviare il messaggio");
                exit(EXIT_FAILURE); // End process
            }
        } else {
            perror("Errore msgsnd: impossibile inviare il messaggio");
            exit(EXIT_FAILURE); // End process
        }
    }
}

int isSemaphoreUnlocked(int semid)
{
    return semctl(semid, 0, GETVAL);
}

void waitForNInitMsg(int msqid, int n)
{
    msg_buffer rbuf;
    for (int i = 0; i < n; i++)
    {
        if (msgrcv(msqid, &rbuf, sizeof(rbuf.mtext), 0, 0) < 0)
        {
            perror("[Error] PID: %d - Errore durante la ricezione del messaggio di inizializzazione");
            exit(EXIT_FAILURE);
        }
    }
}