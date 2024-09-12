#include "lib.h"


int main(int argc, char const *argv[]) {
    printf("[INFO] Alimentazione: Sono stato appena creato\n");
    // Invia un messaggio al master
    key_t key = 1234;
    int msqid;
    if ((msqid = msgget(key, 0666)) < 0) {
        perror("msgget");
        exit(1);
    }

    msg_buffer sbuf;
    sbuf.mtype = 1;
    snprintf(sbuf.mtext, sizeof(sbuf.mtext), "[INFO] Alimentazione (PID: %d): Inizializzazione completata", getpid());

    if (msgsnd(msqid, &sbuf, sizeof(sbuf.mtext), IPC_NOWAIT) < 0) {
        perror("msgsnd");
        exit(1);
    }
    exit(EXIT_SUCCESS);
}