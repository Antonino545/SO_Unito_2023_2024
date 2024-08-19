#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>
#include "lib.h"

int numero_atomico = 0;
int *MIN_N_ATOMICO;
void atomdivision();
int energialiberata(int numero_atomico_padre, int numero_atomico_figlio);
void setup_ParametresMemory() {
    const char *shm_name = "/Parametres";
    const size_t shm_size = 8 * sizeof(int); // 8 interi

    int shm_fd = shm_open(shm_name, O_RDWR, 0666);// Apre la memoria condivisa in lettura e scrittura che e gia stata creata
    if (shm_fd == -1) {
        perror("Errore nella shm_open");
        exit(EXIT_FAILURE);
    }
    // Mappa la memoria condivisa in un puntatore 
    //PROT_READ | PROT_WRITE indica che la memoria condivisa può essere letta e scritta
    //MAP_SHARED indica che la memoria condivisa è condivisa tra più processi
    //shm_fd è il file descriptor della memoria condivisa
    //0 indica che la memoria condivisa è mappata dall'inizio
    void *shm_ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("Errore nella mmap");
        exit(EXIT_FAILURE);
    }

    // Puntatori alla variabile MIN_N_ATOMICO nella memoria condivisa
    MIN_N_ATOMICO = (int *)(shm_ptr + 2 * sizeof(int));//recupera il valore di MIN_N_ATOMICO dalla memoria condivisa
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <numero_atomico>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    setup_ParametresMemory(); // Setup della memoria condivisa

    numero_atomico = atoi(argv[1]);
    printf("Atomo %d: Sono un atomo con numero atomico %d\n", getpid(), numero_atomico);

    if (numero_atomico < *MIN_N_ATOMICO) { // Usa la variabile condivisa
        exit(EXIT_SUCCESS);
    }
    pause();
    exit(EXIT_SUCCESS);
}


void atomdivision() {
    int numero_atomico_figlio = generate_random(numero_atomico - 1); // Genera un numero atomico per il figlio
    numero_atomico = numero_atomico - numero_atomico_figlio; // Calcola il numero atomico rimanente per il padre

    int pipefd[2];
    if (pipe(pipefd) == -1) { // Crea una pipe
        perror("Errore nella creazione della pipe:");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid < 0) { // Processo non creato
        perror("Errore nella fork:");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Processo figlio
        close(pipefd[0]); // Chiudi il lato di lettura della pipe nel processo figlio

        char num_atomico_figlio_str[20];
        snprintf(num_atomico_figlio_str, sizeof(num_atomico_figlio_str), "%d", numero_atomico_figlio);

        printf("Figlio (PID: %d): Sto diventando un atomo con numero atomico %d\n", getpid(), numero_atomico_figlio);

        // Esegui `atomo` con il numero atomico del figlio come argomento
        if (execlp("./atomo", "atomo", num_atomico_figlio_str, NULL) == -1) { // execlp ritorna -1 se fallisce
            perror("Errore in execlp durante la creazione del processo atomo");
            exit(EXIT_FAILURE);
        }

        // Scrivi un segnale nella pipe per indicare che il figlio ha eseguito le istruzioni iniziali
        write(pipefd[1], "1", 1);
        close(pipefd[1]); // Chiudi il lato di scrittura della pipe nel processo figlio
    } else { // Processo padre
        close(pipefd[1]); // Chiudi il lato di scrittura della pipe nel processo padre

        printf("Atomo %d: Mi sto per scindere in due atomi con numeri atomici %d e %d\n", getpid(), numero_atomico, numero_atomico_figlio);

        // Aspetta che il figlio segnali di aver eseguito le sue istruzioni iniziali
        char buf;
        read(pipefd[0], &buf, 1);
        close(pipefd[0]); // Chiudi il lato di lettura della pipe nel processo padre

        // Ora libera l'energia
        energialiberata(numero_atomico, numero_atomico_figlio);

        // Attendi la terminazione del processo figlio
        wait(NULL);
    }
}
int energialiberata(int numero_atomico_padre, int numero_atomico_figlio){
    int energia= numero_atomico_figlio*numero_atomico_padre - max(numero_atomico_figlio, numero_atomico_padre);
    printf("Atomo %d: Ho liberato %d Joule di energia\n", getpid(), energia);
    return energia;
}