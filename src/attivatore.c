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

    send_message_to_master( msqid, INIT_MSG,"[INFO] Attivatore (PID: %d): Inizializzazione completata", getpid());
    printf("[INFO] Attivatore (PID: %d): Invio messaggio di scissione agli atomi\n", getpid());
    killpg(getppid(), SIGUSR2); 
    wait(NULL); // Aspetta che il processo figlio si concluda, se necessario
    exit(EXIT_SUCCESS);
}
