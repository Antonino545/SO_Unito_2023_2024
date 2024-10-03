#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/sem.h>

int msqid;              // ID della coda di messaggi
int numero_atomico = 0; // Numero atomico del processo atomo
int running = 1;        // Flag che indica se il processo è in esecuzione
int semid;              // ID del set di semafori

// Struttura per le operazioni sui semafori
struct sembuf sem_lock = {0, -1, 0};  // Operazione di decremento (lock)
struct sembuf sem_unlock = {0, 1, 0}; // Operazione di incremento (unlock)

int *energia_totale; // Puntatore alla variabile dell'energia totale liberata

/**
 * Funzione che gestisce la divisione dell'atomo.
 * Viene invocata quando il processo riceve il segnale SIGUSR2.
 */
void handle_scissione(int sig)
{
    srand(time(NULL) ^ getpid());   // Inizializza il seed del numero casuale con il tempo e il PID
    int probabilita = rand() % 100; // Genera un numero tra 0 e 99

    if (probabilita < 60)
    {
        int numero_atomico_figlio = generate_random(numero_atomico);
        numero_atomico -= numero_atomico_figlio; // Riduce il numero atomico del padre
        printf("[INFO] Atomo (PID: %d): Ricevuto segnale di scissione (SIGUSR2)\n", getpid());

        // Verifica se il numero atomico è inferiore al minimo consentito
        if (numero_atomico < *MIN_N_ATOMICO)
        {
            printf("[INFO] Atomo (PID: %d): Numero atomico minore di MIN_N_ATOMICO. Atomo terminato\n", getpid());
            send_message(msqid, TERMINATION_MSG, "[INFO] Atomo (PID: %d): Atomo terminato", getpid());
            exit(EXIT_SUCCESS);
        }

        printf("[INFO] Atomo (PID: %d): Scissione avviata \n", getpid());

        // Crea un nuovo processo figlio per rappresentare la scissione
        pid_t pid = fork();
        if (pid == 0)
        {
            // Processo figlio: rappresenta il nuovo atomo creato dalla scissione
            printf("[INFO] Atomo (PID: %d): Creato da scissione del PID %d\n", getpid(), getppid());

            // Converte il numero atomico in stringa e avvia il nuovo processo atomo
            char num_atomico_str[20];
            snprintf(num_atomico_str, sizeof(num_atomico_str), "%d", numero_atomico_figlio);
            printf("[INFO] Atomo (PID: %d): Avvio processo atomo con numero atomico %d\n", getpid(), numero_atomico_figlio);

            if (execlp("./atomo", "atomo", num_atomico_str, NULL) == -1)
            {
                perror("[ERROR] Atomo: execlp fallito durante l'esecuzione del processo atomo");
                exit(EXIT_FAILURE);
            }
        }
        if (pid < 0)
        {
            perror("[ERROR] Atomo: Fork fallita durante la divisione di un atomo");
            kill(*PID_MASTER, SIGUSR1);
        }
        else
        {
            // il padre aspetta il messaggio di inizializzazione del figlio
            waitForNInitMsg(msqid, 1);
        }
    }
}

/**
 * Funzione che gestisce il segnale SIGINT per fermare l'esecuzione del processo atomo.
 * Cambia lo stato della variabile "running" per uscire dal ciclo di attesa.
 */
void handle_sigint(int sig)
{
    printf("[INFO] Atomo (PID: %d): Ricevuto segnale di terminazione (SIGTERM)\n", getpid());
    running = 0; // Imposta running a 0 per terminare il ciclo di attesa
}

void setup_signal_handler()
{

if(sigaction(SIGTERM, &(struct sigaction){.sa_handler = handle_sigint}, NULL)==-1){
        perror("[ERROR] Atomo: Errore durante la gestione del segnale di terminazione");
        exit(EXIT_FAILURE);
    }
    if(sigaction(SIGUSR2, &(struct sigaction){.sa_handler = handle_scissione}, NULL)==-1){
        perror("[ERROR] Atomo: Errore durante la gestione del segnale di start");
        exit(EXIT_FAILURE);
    }
}
/**
 * Funzione che calcola l'energia liberata durante la divisione dell'atomo
 * e aggiorna la memoria condivisa delle statistiche in modo sicuro con i semafori.
 *
 * @param n1 Numero atomico del padre
 * @param n2 Numero atomico del figlio
 * @return Energia liberata
 */
int energialiberata(int n1, int n2)
{
    int energia_liberata = (n1 * n2) - (n1 > n2 ? n1 : n2); // Formula per calcolare l'energia

    // Blocca l'accesso alla memoria condivisa con il semaforo
    if (semop(semid, &sem_lock, 1) == -1)
    {
        perror("[ERROR] Atomo: semop lock fallito");
        exit(EXIT_FAILURE);
    }

    // Aggiorna l'energia totale liberata nella memoria condivisa
    *energia_totale += energia_liberata;
    printf("[INFO] Atomo (PID: %d): Energia liberata = %d, Energia totale aggiornata = %d\n", getpid(), energia_liberata, *energia_totale);

    // Rilascia il semaforo
    if (semop(semid, &sem_unlock, 1) == -1)
    {
        perror("[ERROR] Atomo: semop unlock fallito");
        exit(EXIT_FAILURE);
    }

    return energia_liberata;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "[ERROR] Atomo (PID: %d): Numero di argomenti errato\n", getpid());
        exit(EXIT_FAILURE);
    }

    // Inizializza memoria condivisa e recupera parametri
    void *shm_ptr = allocateParametresMemory();
    MIN_N_ATOMICO = (int *)(shm_ptr + 2 * sizeof(int));
    PID_MASTER = (int *)(shm_ptr + 8 * sizeof(int));
    numero_atomico = atoi(argv[1]);

    // Inizializza memoria condivisa per le statistiche
    Statistiche *stats = (Statistiche *)allocateStatisticsMemory(); // Ottieni puntatore alla struttura Statistiche
    int *energia_totale = &(stats->energia_prodotta.totale);        // Supponiamo che energia_prodotta.totale sia la variabile di interesse

    // Inizializza i semafori
    semid = getSemaphoreSet(); // Funzione per ottenere l'ID del set di semafori

    printf("[INFO] Atomo (PID: %d): Creato atomo con numero atomico %d e GP(%d)\n", getpid(), numero_atomico, getpgrp());

    // Ottieni l'ID della coda di messaggi
    key_t key = MESSAGE_QUEUE_KEY;
    if ((msqid = msgget(key, 0666)) < 0)
    {
        perror("[ERROR] Atomo: Impossibile ottenere l'ID della coda di messaggi");
        exit(EXIT_FAILURE);
    }

    setup_signal_handler(); // Imposta i gestori dei segnali
    // Notifica al master che l'inizializzazione è completata
    send_message(msqid, ATOMO_INIT_MSG, "Inizializzazione completata", getpid());

    if (setpgid(getpid(), *PID_MASTER) == -1)
    {
        printf("[INFO] Atomo (PID: %d): Impossibile impostare il gruppo di processi del figlio\n", getpid());
        printf("[INFO] Atomo (PID: %d): Terminazione completata\n", getpid());
        running = 0;
    }
    printf("[INFO] Atomo (PID: %d): In attesa di messaggi di scissione o terminazione\n", getpid());

    // Ciclo principale di attesa
    while (running)
    {
        pause(); // Aspetta un segnale
    }  
    printf("[INFO] Atomo (PID: %d): Terminazione completata\n", getpid());
    while (wait(NULL) > 0); // Aspetta che tutti i processi figli terminino
    
    exit(EXIT_SUCCESS);
}
