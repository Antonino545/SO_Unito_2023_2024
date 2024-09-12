#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>     // Per O_CREAT e O_RDWR
#include<time.h>


#define MSGSZ 128
typedef struct {
    long mtype;
    char mtext[MSGSZ];
} msg_buffer;
/**
 * Genera un numero casuale tra 1 e `max`. 
 * @param max Il valore massimo che può essere generato.
 */
int generate_random(int max) {
    srand(time(NULL) ^ getpid());// serve per generare numeri casuali diversi per ogni processo se lanciato in contemporanea
    return rand() % max + 1;
}
/* macro per il massimo tra due numeri */
#define max(a, b) ((a) > (b) ? (a) : (b))

/**
 * Crea una memoria condivisa e restituisce un puntatore ad essa.
 * @param shm_name Nome della memoria condivisa.
 * @param shm_size Dimensione della memoria condivisa.
 * @return Puntatore alla memoria condivisa.
 */
void* create_shared_memory(const char *shm_name, size_t shm_size) {
    // Crea o apre una memoria condivisa
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
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

/**
 * Funzione che imposta la memoria condivisa per i parametri.
 */
void* allocateParametresMemory() {
    const char *shm_name = "/Parametres";
    const size_t shm_size = 8 * sizeof(int); // 8 interi

    int shm_fd = shm_open(shm_name, O_RDWR, 0666); // Apre la memoria condivisa in lettura e scrittura che è già stata creata
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