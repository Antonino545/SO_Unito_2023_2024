#include "lib.h"
#include <errno.h>

int *N_ATOMI_INIT; /** Numero iniziale di atomi */
int *N_ATOM_MAX; /** Numero massimo di atomi */
int *MIN_N_ATOMICO; /** Numero atomico minimo */
int *ENERGY_DEMAND; /** Domanda di energia */
int *STEP; /** Passo per la variazione dell'energia */
int *N_NUOVI_ATOMI; /** Numero di nuovi atomi */
int *SIM_DURATION; /** Durata della simulazione */
int *ENERGY_EXPLODE_THRESHOLD; /** Soglia di esplosione dell'energia */
int *PID_MASTER; /** PID del processo master */
int *ATOMO_GPID; /** Gruppo di processi degli atomi */
int generate_random(int max) {
    return rand() % max + 1; // Restituisce un numero tra 1 e max
}

void* create_shared_memory(const char *shm_name, size_t shm_size) {
    // Crea o apre una memoria condivisa
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, MES_PERM_RW_ALL);
    if (shm_fd == -1) {
        perror("[ERROR] Master: Errore durante la creazione della memoria condivisa (shm_open fallita)");
        exit(EXIT_FAILURE);
    }

    // Imposta la dimensione della memoria condivisa
    if (ftruncate(shm_fd, shm_size) == -1) {
        perror("[ERROR] Master: Impossibile impostare la dimensione della memoria condivisa (ftruncate fallita)");
        exit(EXIT_FAILURE);
    }

    // Mappa la memoria condivisa nel proprio spazio degli indirizzi
    void *shm_ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("[ERROR] Master: Errore durante la mappatura della memoria condivisa (mmap fallita)");
        exit(EXIT_FAILURE);
    }

    return shm_ptr;
}

void* allocateParametresMemory() {
    const char *shm_name = "/Parametres";
    const size_t shm_size = 8 * sizeof(int); // 8 interi

    int shm_fd = shm_open(shm_name, O_RDWR, MES_PERM_RW_ALL); // Apre la memoria condivisa in lettura e scrittura che è già stata creata
    if (shm_fd == -1) {
        perror("Errore nella shm_open");
        exit(EXIT_FAILURE);
    }

    void *shm_ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("Errore nella mmap");
        exit(EXIT_FAILURE);
    }
    return shm_ptr;
}

void send_message(int msqid, long type, const char *format, ...) {
    msg_buffer message;
    message.mtype = type;
    snprintf(message.mtext, sizeof(message.mtext), "%s", format);

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


void waitForNInitMsg(int msqid, int n) {
    msg_buffer rbuf;
    for (int i = 0; i < n; i++) {
        if (msgrcv(msqid, &rbuf, sizeof(rbuf.mtext), INIT_MSG, 0) < 0) {
            perror("Errore msgrcv: impossibile ricevere il messaggio");
            exit(EXIT_FAILURE);
        }
        printf("[MESSRIC] Master (PID: %d) - Message: %s\n", getpid(), rbuf.mtext);
    }
}

/**
 * Invia un segnale di inizio simulazione ai processi attivatore e alimentatore.
 */
void sendStartSimulationSignal(pid_t attivatore_pid, pid_t alimentazione_pid) {
    if (kill(attivatore_pid, SIGUSR2) == -1) {
        perror("[ERROR] Master: Impossibile inviare il segnale di inizio simulazione all'attivatore");
        exit(EXIT_FAILURE);
    }
    if (kill(alimentazione_pid, SIGUSR2) == -1) {
        perror("[ERROR] Master: Impossibile inviare il segnale di inizio simulazione all'alimentazione");
        exit(EXIT_FAILURE);
    }
    printf("[INFO] Master (PID: %d): Segnale di inizio simulazione inviato a attivatore e alimentazione\n", getpid());
}