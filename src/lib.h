#include <stdlib.h>
#include <time.h>
#include <unistd.h>
/**
 * Genera un numero casuale tra 1 e `max`. 
 * @param max Il valore massimo che può essere generato.
 */
int generate_random(int max) {
    srand(time(NULL) ^ getpid());// serve per generare numeri casuali diversi per ogni processo se lanciato in contemporanea
    return rand() % max + 1;
}

#define max(a, b) ((a) > (b) ? (a) : (b))

/**
 * Configura la memoria condivisa.
 * @param shm_name Nome della memoria condivisa.
 * @param shm_size Dimensione della memoria condivisa.
 * @return Puntatore alla memoria condivisa.
 */
void* setup_shared_memory(const char *shm_name, size_t shm_size) {
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