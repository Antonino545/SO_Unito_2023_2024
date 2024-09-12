
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
 * Funzione che attende un messaggio di scissione dall'attivatore.
 * 
 * @param msqid ID della coda di messaggi
 */
void wait_for_scission_message(int msqid) {
    msg_buffer rbuf;
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

    void* shm_ptr= allocateParametresMemory(); // Inizializza la memoria condivisa
    MIN_N_ATOMICO = (int *)(shm_ptr + 2 * sizeof(int)); // Recupera il valore di MIN_N_ATOMICO dalla memoria condivisa

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
        send_message_to_master(msqid,"[INFO] Atomo (PID: %d): Atomo terminato", getpid());
        exit(EXIT_SUCCESS);
    }

    // Invia messaggio di "Inizializzazione completata" al master
    send_message_to_master(msqid,"[INFO] Atomo (PID: %d): Inizializzazione completata", getpid());



    wait(NULL); // Aspetta che il processo figlio si concluda, se necessario
    exit(EXIT_SUCCESS);
}
