#include "lib.h"

int *PID_MASTER;
int *MIN_N_ATOMICO;
int *N_ATOM_MAX; /**< Numero massimo di atomi */

/*
 * Crea un nuovo processo figlio per eseguire il programma `atomo` con un numero atomico casuale.
 */
void createAtomo() {
    int numero_atomico = generate_random(*N_ATOM_MAX);
    char num_atomico_str[20];
    snprintf(num_atomico_str, sizeof(num_atomico_str), "%d", numero_atomico);
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("[ERROR] Master: Fork fallita durante la creazione di un atomo");
        kill(*PID_MASTER, SIGUSR1);
    } else if (pid == 0) {
        printf("[INFO] Atomo (PID: %d): Avvio processo atomo con numero atomico %d\n", getpid(), numero_atomico);
        if (execlp("./atomo", "atomo", num_atomico_str, NULL) == -1) {
            perror("[ERROR] Atomo: execlp fallito durante l'esecuzione del processo atomo");
            exit(EXIT_FAILURE);
        }
    } else {
        if (*ATOMO_GPID == -1) {
            *ATOMO_GPID = pid; // Imposta il GPID del primo atomo
            printf("[INFO] Master (PID: %d): GPID del primo atomo impostato a %d\n", getpid(), *ATOMO_GPID);
        }
        if (setpgid(pid, *ATOMO_GPID) == -1) {
            perror("[ERROR] Master: Impossibile impostare il gruppo di processi del figlio");
        } else {
            printf("[INFO] Master (PID: %d): Processo atomo con PID: %d, gruppo impostato a %d\n", getpid(), pid, *ATOMO_GPID);
        }
    }
}

int main(int argc, char const *argv[]) {
    printf("[INFO] Alimentazione: Sono stato appena creato\n");
    // Invia un messaggio al master
    key_t key = MESSAGE_QUEUE_KEY;
    int msqid;
    if ((msqid = msgget(key, MES_PERM_RW_ALL)) < 0) {
        perror("msgget");
        exit(1);
    }

    msg_buffer sbuf;
    sbuf.mtype = 1;
    send_message_to_master( msqid,INIT_MSG, "[INFO] Alimentazione (PID: %d): Inizializzazione completata", getpid());
    wait(NULL); // Aspetta che il processo figlio si concluda, se necessario
    void* shm_ptr= allocateParametresMemory(); // Inizializza la memoria condivisa
    PID_MASTER = (int *)(shm_ptr + 8 * sizeof(int)); // Recupera il PID del processo master dalla memoria condivisa
    MIN_N_ATOMICO = (int *)(shm_ptr + 2 * sizeof(int)); // Recupera il valore di MIN_N_ATOMICO dalla memoria condivisa
    N_ATOM_MAX = (int *)(shm_ptr + 3 * sizeof(int)); // Recupera il valore di N_ATOM_MAX dalla memoria condivisa
    ATOMO_GPID=( pid_t *)(shm_ptr + 9 * sizeof(int)); // Recupera il GPID degli atomi dalla memoria condivisa
 
    exit(EXIT_SUCCESS);
}