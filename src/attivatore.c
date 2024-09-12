#include "lib.h"

/**
 * Invia un messaggio di scissione all'atomo con il PID specificato.
 */
void invia_messaggio_scissione(int msqid, pid_t atomo_pid) {
    msg_buffer sbuf;
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
    printf("[INFO] Attivatore (PID: %d): Sono stato appena creato\n", getpid());

    // Ottieni l'ID della coda di messaggi
    key_t key = MESSAGE_QUEUE_KEY;
    int msqid;
    if ((msqid = msgget(key, 0666)) < 0) {
        perror("msgget");
        exit(1);
    }

    send_message_to_master( msqid, "[INFO] Attivatore (PID: %d): Inizializzazione completata", getpid());
    exit(EXIT_SUCCESS);
}
