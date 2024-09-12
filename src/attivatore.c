#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "lib.h"


void invia_messaggio_scissione(int msqid, pid_t atomo_pid) {
    message_buf sbuf;
    sbuf.mtype = atomo_pid;  // Imposta il mtype al PID dell'atomo destinatario

    // Crea un messaggio di richiesta di scissione
    snprintf(sbuf.mtext, sizeof(sbuf.mtext), "[INFO] Attivatore (PID: %d): Scissione richiesta", getpid());

    // Invia il messaggio alla coda
    if (msgsnd(msqid, &sbuf, sizeof(sbuf.mtext), IPC_NOWAIT) < 0) {
        perror("msgsnd");
        exit(1);
    }

    printf("[INFO] Attivatore (PID: %d): Messaggio di scissione inviato all'atomo (PID: %d)\n", getpid(), atomo_pid);
}

int main(int argc, char const *argv[]) {
    printf("[INFO] Attivatore: Sono stato appena creato\n");

    // Ottieni l'ID della coda di messaggi
    key_t key = 1234;
    int msqid;
    if ((msqid = msgget(key, 0666)) < 0) {
        perror("msgget");
        exit(1);
    }

    message_buf sbuf;
    sbuf.mtype = 1;
    snprintf(sbuf.mtext, sizeof(sbuf.mtext), "[INFO] Attivatore (PID: %d): Inizializzazione completata", getpid());

    if (msgsnd(msqid, &sbuf, sizeof(sbuf.mtext), IPC_NOWAIT) < 0) {
        perror("msgsnd");
        exit(1);
    }
    exit(EXIT_SUCCESS);
}
