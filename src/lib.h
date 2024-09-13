#ifndef LIB_H
#define LIB_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include <stdarg.h>

#define INIT_MSG 1
#define MESS_SIZE 128
#define MESSAGE_QUEUE_KEY 1234
typedef struct {
    long mtype;
    char mtext[MESS_SIZE];
} msg_buffer;

/**
 * Genera un numero casuale tra 1 e `max`. 
 * @param max Il valore massimo che può essere generato.
 */
int generate_random(int max);

/* macro per il massimo tra due numeri */
#define max(a, b) ((a) > (b) ? (a) : (b))

/**
 * Crea una memoria condivisa e restituisce un puntatore ad essa.
 * @param shm_name Nome della memoria condivisa.
 * @param shm_size Dimensione della memoria condivisa.
 * @return Puntatore alla memoria condivisa.
 */
void* create_shared_memory(const char *shm_name, size_t shm_size);

/**
 * Funzione che imposta la memoria condivisa per i parametri.
 */
void* allocateParametresMemory();

/**
 * Funzione che invia un messaggio formattato al processo master.
 *
 * @param msqid ID della coda di messaggi
 * @param format Formato del messaggio (come printf)
 * @param type Tipo del messaggio
 * @param ... Argomenti variabili per il formato
 */
void send_message_to_master(int msqid,long type, const char *format, ...);


/**
 * Funzione che attende un messaggio di inizializzazione da un processo.
 * 
 * @param msqid ID della coda di messaggi
 */
void waitForNInitMsg(int msqid, int n) ;


#endif // LIB_H