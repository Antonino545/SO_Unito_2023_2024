#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>

int msqid; // Declare msqid as a global variable

int numero_atomico = 0;

/**
 * Funzione che gestisce la divisione dell'atomo.
 */
void handle_scissione(int sig)
{
    printf("[INFO] Atomo (PID: %d): Ricevuto segnale di scissione (SIGUSR2)\n", getpid());
    // Genera un numero casuale per decidere se scindersi
    srand(time(NULL) ^ getpid());   // Inizializza il seed con il tempo attuale e il PID
    int probabilita = rand() % 100; // Genera un numero tra 0 e 99 (percentuale)

    if (probabilita < 50)
    { // 50% di probabilità di scindersi
        int numero_atomico_figlio= generate_random(numero_atomico);
        numero_atomico -= numero_atomico_figlio;

        printf("[INFO] Atomo (PID: %d): Scissione avviata (50%% probabilità soddisfatta)\n", getpid());

        if (numero_atomico < *MIN_N_ATOMICO)
        {
            printf("[INFO] Atomo (PID: %d): Numero atomico minore di MIN_N_ATOMICO. Atomo terminato\n", getpid());
                    send_message_to_master(msqid, INIT_MSG, "[INFO] Atomo (PID: %d): Atomo terminato", getpid());

        
            exit(EXIT_SUCCESS);
        }

        // Crea un nuovo processo figlio per rappresentare la scissione
        pid_t pid = fork();
        if (pid == 0)
        {
            // Questo è il processo figlio (nuovo atomo)
            printf("[INFO]  Atomo (PID: %d): Creato da scissione del PID %d\n", getpid(), getppid());
            // Il processo figlio può eseguire logica specifica qui
            
             char num_atomico_str[20];
            snprintf(num_atomico_str, sizeof(num_atomico_str), "%d", numero_atomico_figlio); // Converte il numero atomico in stringa
            printf("[INFO] Atomo (PID: %d): Avvio processo atomo con numero atomico %d\n", getpid(), numero_atomico);

            // Esegue `atomo` con il numero atomico come argomento
            if (execlp("./atomo", "atomo", num_atomico_str, NULL) == -1)
            {
                perror("[ERROR] Atomo: execlp fallito durante l'esecuzione del processo atomo");
                exit(EXIT_FAILURE);
            }
        }
        else if (pid > 0)
        {
                if (setpgid(pid, getpid()) == -1)
            {
                printf("[INFO] Atomo (PID: %d): Impossibile impostare il gruppo di processi del figlio", getpid());
            }
            else
            {
                printf("[INFO] Atomo (PID: %d): Processo atomo con PID: %d, gruppo impostato a %d\n", getpid(), pid, getpid());
            }
        }
        else
        {
            perror("[ERROR] Atomo: Fork fallita durante la scissione");
        }
    }
    else
    {
        printf("[INFO] Atomo (PID: %d): Non scisso (50%% probabilità non soddisfatta)\n", getpid());
    }
}


int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "[ERROR] Atomo (PID: %d): Numero di argomenti errato\n", getpid());
        exit(EXIT_FAILURE);
    }

    void *shm_ptr = allocateParametresMemory();         // Inizializza la memoria condivisa
    MIN_N_ATOMICO = (int *)(shm_ptr + 2 * sizeof(int)); // Recupera il valore di MIN_N_ATOMICO dalla memoria condivisa
    PID_MASTER = (int *)(shm_ptr + 8 * sizeof(int));    // Recupera il PID del processo master dalla memoria condivisa
    numero_atomico = atoi(argv[1]);
    printf("[INFO] Atomo (PID: %d): Sono stato appena creato con numero atomico %d con gruppo di processi %d\n", getpid(), numero_atomico, getpgrp());

    // Ottieni l'ID della coda di messaggi
    key_t key = MESSAGE_QUEUE_KEY;
    
    if ((msqid = msgget(key, 0666)) < 0)
    {
        perror("Errore msgget: impossibile ottenere l'ID della coda di messaggi");
        exit(1);
    }

    // Verifica se l'atomo deve terminare immediatamente
   struct sigaction sa2;
    bzero(&sa2, sizeof(sa2));
    sa2.sa_handler = handle_scissione;  // Setting the handler for SIGUSR2
    sigemptyset(&sa2.sa_mask);
    sigaction(SIGUSR2, &sa2, NULL);
    

    // Invia messaggio di "Inizializzazione completata" al master
    send_message_to_master(msqid, INIT_MSG, "[INFO] Atomo (PID: %d): Inizializzazione completata", getpid());
    printf("[INFO] Atomo (PID: %d): In attesa di messaggi di scissione\n", getpid());
    
    wait(NULL); // Wait for child process to finish
   exit(EXIT_SUCCESS);
}
