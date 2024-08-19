#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>  // Include this header for msgget and msgsnd

int numero_atomico = 0;
int *MIN_N_ATOMICO;

typedef struct msgbuf {
    long mtype;
    char mtext[128];
} message_buf;

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

    void *shm_ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("Errore nella mmap");
        exit(EXIT_FAILURE);
    }

    MIN_N_ATOMICO = (int *)(shm_ptr + 2 * sizeof(int)); // Recupera il valore di MIN_N_ATOMICO dalla memoria condivisa
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <numero_atomico>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    setup_ParametresMemory(); // Setup della memoria condivisa

    numero_atomico = atoi(argv[1]);
    printf("Atomo %d: Sono un atomo con numero atomico %d\n", getpid(), numero_atomico);

    if (numero_atomico < *MIN_N_ATOMICO) {
        exit(EXIT_SUCCESS);
    }

    // Invia un messaggio al master
    key_t key = 1234;
    int msqid;
    if ((msqid = msgget(key, 0666)) < 0) {
        perror("msgget");
        exit(1);
    }

    message_buf sbuf;
    sbuf.mtype = 1;
    snprintf(sbuf.mtext, sizeof(sbuf.mtext), "Atomo %d: Inizializzazione completata", getpid());

    if (msgsnd(msqid, &sbuf, sizeof(sbuf.mtext), IPC_NOWAIT) < 0) {
        perror("msgsnd");
        exit(1);
    }

    pause();
    exit(EXIT_SUCCESS);
}
