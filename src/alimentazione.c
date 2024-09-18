#include "lib.h"

int *PID_MANSTER;
int *MIN_N_ATOMICO;
int *N_ATOM_MAX; /**< Numero massimo di atomi */

/*
 * Crea un nuovo processo figlio per eseguire il programma `atomo` con un numero atomico casuale.
 */
void createAtomo() {
    pid_t pid = fork(); // Crea un nuovo processo
    int numero_atomico = generate_random(*N_ATOM_MAX); // Genera un numero atomico casuale

    if (pid < 0) { // Errore nella creazione del processo
        perror("[ERROR] Master: Fork fallita durante l'aggiunta di un nuovo atomo da parte dell'alimentazione");
        kill(*PID_MANSTER, SIGUSR1);

    } else if (pid == 0) { // Processo figlio
        char num_atomico_str[20];
        snprintf(num_atomico_str, sizeof(num_atomico_str), "%d", numero_atomico); // Converte il numero atomico in stringa

        printf("[INFO] Atomo (PID: %d): Avvio processo atomo con numero atomico %d\n", getpid(), numero_atomico);

        // Esegue `atomo` con il numero atomico come argomento
        if (execlp("./atomo", "atomo", num_atomico_str, NULL) == -1) {
            perror("[ERROR] Atomo: execlp fallito durante l'esecuzione del processo atomo");
            exit(EXIT_FAILURE);
        }
    } else { // Processo padre
        printf("[INFO] Master (PID: %d): Processo atomo creato con PID: %d\n", getpid(), pid);
    }
}

int main(int argc, char const *argv[]) {
    printf("[INFO] Alimentazione: Sono stato appena creato\n");
    // Invia un messaggio al master
    key_t key = MESSAGE_QUEUE_KEY;
    int msqid;
    if ((msqid = msgget(key, 0666)) < 0) {
        perror("msgget");
        exit(1);
    }

    msg_buffer sbuf;
    sbuf.mtype = 1;
    send_message_to_master( msqid,INIT_MSG, "[INFO] Alimentazione (PID: %d): Inizializzazione completata", getpid());
    wait(NULL); // Aspetta che il processo figlio si concluda, se necessario
    void* shm_ptr= allocateParametresMemory(); // Inizializza la memoria condivisa
    PID_MANSTER = (int *)(shm_ptr + 8 * sizeof(int)); // Recupera il PID del processo master dalla memoria condivisa
    MIN_N_ATOMICO = (int *)(shm_ptr + 2 * sizeof(int)); // Recupera il valore di MIN_N_ATOMICO dalla memoria condivisa
    N_ATOM_MAX = (int *)(shm_ptr + 3 * sizeof(int)); // Recupera il valore di N_ATOM_MAX dalla memoria condivisa
    exit(EXIT_SUCCESS);
}