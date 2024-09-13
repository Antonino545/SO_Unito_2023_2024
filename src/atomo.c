
#include "lib.h"

int numero_atomico = 0;
int *MIN_N_ATOMICO;


/**
 * Funzione che gestisce la divisione dell'atomo.
 * (Implementazione non fornita nel codice originale)
 */
void atomdivision() {

    pid_t pid = fork(); // Crea un nuovo processo
    int numero_atomico_figlio = generate_random(numero_atomico-1); // Genera un numero atomico casuale
    numero_atomico = numero_atomico - numero_atomico_figlio; // Aggiorna il numero atomico dell'atomo
    if (pid < 0) { // Errore nella creazione del processo
        perror("[ERROR] Master: Fork fallita durante la creazione di un atomo");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Processo figlio
        char num_atomico_str[20];
        snprintf(num_atomico_str, sizeof(num_atomico_str), "%d", numero_atomico_figlio); // Converti il numero atomico in stringa

        printf("[INFO] Atomo (PID: %d): Avvio processo atomo con numero atomico %d\n", getpid(), numero_atomico);

        // Esegui `atomo` con il numero atomico come argomento
        if (execlp("./atomo", "atomo", num_atomico_str, NULL) == -1) {
            perror("[ERROR] Atomo: execlp fallito durante l'esecuzione del processo atomo");
            exit(EXIT_FAILURE);
        }
    } else { // Processo padre
        
        printf("[INFO] Master (PID: %d): Processo atomo creato con PID: %d e numero atomico: %d\n", getpid(), pid, numero_atomico);
    

}}

/**
 * Funzione che calcola l'energia liberata durante la divisione dell'atomo.
 * (Implementazione non fornita nel codice originale)
 * 
 * @param n1 Numero atomico del padre
 * @param n2 Numero atomico del figlio
 * @return Energia liberata
 */
int energialiberata(int n1, int n2) {
    // Definisci n1 e n2 come i numeri atomici dell'atomo padre e figlio


    // Calcola l'energia liberata usando la formula data
    int energia_liberata = (n1 * n2) - (n1 > n2 ? n1 : n2);

    return energia_liberata;
}





void ricevi_messaggio_scissione(int msqid, pid_t mio_pid) {
    msg_buffer rbuf;

    // Il processo entra in attesa bloccante finché non arriva un messaggio
    if (msgrcv(msqid, &rbuf, sizeof(rbuf.mtext), mio_pid, 0) < 0) {
        perror("msgrcv");
        exit(1);
    }

    printf("[INFO] Atomo (PID: %d): Messaggio ricevuto: %s\n", getpid(), rbuf.mtext);

    // Controlla se il messaggio contiene la parola "Scissione"
    if (strstr(rbuf.mtext, "Scissione") != NULL) {
        printf("[INFO] Atomo (PID: %d): Scissione rilevata! Procedura di scissione avviata.\n", getpid());
    }
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
    key_t key = MESSAGE_QUEUE_KEY;
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
