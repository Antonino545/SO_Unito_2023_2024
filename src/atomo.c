#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>

int msqid; // ID della coda di messaggi
int numero_atomico = 0; // Numero atomico del processo atomo
int running = 1; // Flag che indica se il processo è in esecuzione

/**
 * Funzione che gestisce la divisione dell'atomo.
 * Viene invocata quando il processo riceve il segnale SIGUSR2.
 */
void handle_scissione(int sig)
{
    srand(time(NULL) ^ getpid()); // Inizializza il seed del numero casuale con il tempo e il PID
    int probabilita = rand() % 100; // Genera un numero tra 0 e 99

    if (probabilita < 50)
    { 
        int numero_atomico_figlio = generate_random(numero_atomico);
        numero_atomico -= numero_atomico_figlio; // Riduce il numero atomico del padre
        printf("[INFO] Atomo (PID: %d): Ricevuto segnale di scissione (SIGUSR2)\n", getpid());

        // Verifica se il numero atomico è inferiore al minimo consentito
        if (numero_atomico < *MIN_N_ATOMICO)
        {
            printf("[INFO] Atomo (PID: %d): Numero atomico minore di MIN_N_ATOMICO. Atomo terminato\n", getpid());
            send_message_to_master(msqid, INIT_MSG, "[INFO] Atomo (PID: %d): Atomo terminato", getpid());
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
        else if (pid > 0)
        {
            // Processo padre: imposta il gruppo di processi del figlio
            if (setpgid(pid, *PID_MASTER) == -1)
            {
                printf("[INFO] Atomo (PID: %d): Impossibile impostare il gruppo di processi del figlio\n", getpid());
            }
            else
            {
                printf("[INFO] Atomo (PID: %d): Processo atomo con PID: %d, gruppo impostato a %d\n", getpid(), pid, getpid());
            }
        }
        else
        {
            // Errore nella fork
            perror("[ERROR] Atomo: Fork fallita durante la scissione");
               kill(*PID_MASTER, SIGUSR1);
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

/**
 * Funzione che calcola l'energia liberata durante la divisione dell'atomo.
 * 
 * @param n1 Numero atomico del padre
 * @param n2 Numero atomico del figlio
 * @return Energia liberata
 */
int energialiberata(int n1, int n2)
{
    int energia_liberata = (n1 * n2) - (n1 > n2 ? n1 : n2); // Formula per calcolare l'energia
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

    printf("[INFO] Atomo (PID: %d): Creato con numero atomico %d e gruppo di processi %d\n", getpid(), numero_atomico, getpgrp());

    // Ottieni l'ID della coda di messaggi
    key_t key = MESSAGE_QUEUE_KEY;
    if ((msqid = msgget(key, 0666)) < 0)
    {
        perror("[ERROR] Atomo: Impossibile ottenere l'ID della coda di messaggi");
        exit(EXIT_FAILURE);
    }

    // Gestione dei segnali per la scissione (SIGUSR2) e la terminazione (SIGINT)
    struct sigaction sa2;
    bzero(&sa2, sizeof(sa2));
    sa2.sa_handler = handle_scissione;
    sigemptyset(&sa2.sa_mask);
    sigaction(SIGUSR2, &sa2, NULL);

    struct sigaction sa_int;
    bzero(&sa_int, sizeof(sa_int));
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sigaction(SIGTERM, &sa_int, NULL);

    // Notifica al master che l'inizializzazione è completata
    send_message_to_master(msqid, INIT_MSG, "[INFO] Atomo (PID: %d): Inizializzazione completata", getpid());
    printf("[INFO] Atomo (PID: %d): In attesa di messaggi di scissione o terminazione\n", getpid());

    // Ciclo principale di attesa
    while (running)
    {
        pause(); // Aspetta un segnale
    }

    printf("[INFO] Atomo (PID: %d): Terminazione completata\n", getpid());
    exit(EXIT_SUCCESS);
}
