#include "lib.h"



int main(int argc, char const *argv[]) {
    printf("[INFO] Attivatore (PID: %d): Sono stato appena creato\n", getpid());

    // Ottieni l'ID della coda di messaggi
    key_t key = MESSAGE_QUEUE_KEY;
    int msqid;
    if ((msqid = msgget(key, MES_PERM_RW_ALL)) < 0) {
        perror("msgget");
        exit(1);
    }
    void *shm_ptr = allocateParametresMemory();     
    ATOMO_GPID=(pid_t *)(shm_ptr + 9 * sizeof(int));
    send_message_to_master( msqid, INIT_MSG,"[INFO] Attivatore (PID: %d): Inizializzazione completata", getpid());
    printf("[INFO] Attivatore (PID: %d): Invio messaggio di scissione agli atomi\n", getpid());
    killpg(*ATOMO_GPID, SIGUSR2); 
    wait(NULL); // Aspetta che il processo figlio si concluda, se necessario
    exit(EXIT_SUCCESS);
}
