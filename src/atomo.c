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
#include <sys/msg.h>  // Include questo header per msgget e msgsnd
#include "lib.h"

int numero_atomico = 0;
int *MIN_N_ATOMICO;


/**
 * Funzione che gestisce la divisione dell'atomo.
 * (Implementazione non fornita nel codice originale)
 */
void atomdivision() {
    // Implementa la logica di divisione dell'atomo
}

/**
 * Funzione che calcola l'energia liberata durante la divisione dell'atomo.
 * (Implementazione non fornita nel codice originale)
 * 
 * @param numero_atomico_padre Numero atomico del padre
 * @param numero_atomico_figlio Numero atomico del figlio
 * @return Energia liberata
 */
int energialiberata(int numero_atomico_padre, int numero_atomico_figlio) {
    // Implementa il calcolo dell'energia liberata
    return 0; // Sostituisci con la logica corretta
}

/**
 * Funzione che imposta la memoria condivisa per i parametri.
 * Recupera il valore di MIN_N_ATOMICO dalla memoria condivisa.
 */
void setup_ParametresMemory() {
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

    MIN_N_ATOMICO = (int *)(shm_ptr + 2 * sizeof(int)); // Recupera il valore di MIN_N_ATOMICO dalla memoria condivisa
}

/**
 * Funzione che invia un messaggio al processo master.
 * 
 * @param msqid ID della coda di messaggi
 * @param atom_pid PID dell'atomo
 * @param status Stato da inviare al master
 */
void send_message_to_master(int msqid, int atom_pid, const char *status) {
    message_buf sbuf;
    sbuf.mtype = 1;  // Tipo di messaggio (lo stesso per tutti i messaggi inviati dall'atomo)
    snprintf(sbuf.mtext, sizeof(sbuf.mtext), "Atomo %d: %s", atom_pid, status);

    if (msgsnd(msqid, &sbuf, sizeof(sbuf.mtext), IPC_NOWAIT) < 0) {
        perror("Errore msgsnd: impossibile inviare il messaggio");
        // Si può gestire ulteriormente l'errore qui se necessario
        exit(1);
    }
}

/**
 * Funzione che attende un messaggio di scissione dall'attivatore.
 * 
 * @param msqid ID della coda di messaggi
 */
void wait_for_scission_message(int msqid) {
    message_buf rbuf;
    ssize_t msg_length;
    
    // Attende un messaggio dalla coda di messaggi
    printf("[INFO] Atomo (PID: %d): In attesa di un messaggio di scissione...\n", getpid());
    if ((msg_length = msgrcv(msqid, &rbuf, sizeof(rbuf.mtext), getpid(), 0)) < 0) {
        perror("Errore msgrcv: impossibile ricevere il messaggio");
        exit(1);
    }

    // Verifica il contenuto del messaggio
    printf("[INFO] Atomo (PID: %d): Messaggio ricevuto: %s\n", getpid(), rbuf.mtext);

    // Esegui la divisione dell'atomo
    atomdivision();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[ERROR] Atomo (PID: %d): Numero di argomenti errato\n", getpid());
        exit(EXIT_FAILURE);
    }

    setup_ParametresMemory(); // Setup della memoria condivisa

    numero_atomico = atoi(argv[1]);
    printf("[INFO] Atomo (PID: %d): Sono stato appena creato con numero atomico %d\n", getpid(), numero_atomico);

    // Ottieni l'ID della coda di messaggi
    key_t key = 1234;
    int msqid;
    if ((msqid = msgget(key, 0666)) < 0) {
        perror("Errore msgget: impossibile ottenere l'ID della coda di messaggi");
        exit(1);
    }

    // Verifica se l'atomo deve terminare immediatamente
    if (numero_atomico < *MIN_N_ATOMICO) {
        printf("[INFO] Atomo (PID: %d): Numero atomico minore di MIN_N_ATOMICO. Atomo terminato\n", getpid());
        send_message_to_master(msqid, getpid(), "Terminazione con successo");
        exit(EXIT_SUCCESS);
    }

    // Invia messaggio di "Inizializzazione completata" al master
    send_message_to_master(msqid, getpid(), "Inizializzazione completata");



    wait(NULL); // Aspetta che il processo figlio si concluda, se necessario
    exit(EXIT_SUCCESS);
}
