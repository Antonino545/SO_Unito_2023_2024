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
#include  <signal.h>

#define INIT_MSG 1// Tipo di messaggio per l'inizializzazione che e di tipo 1
#define MSG_TYPE_START_SIM 2 // Tipo di messaggio per l'inizio della simulazione
#define MESS_SIZE 128 // Dimensione massima del messaggio
#define MESSAGE_QUEUE_KEY 1234 //Key della coda di messaggi 
#define MES_PERM_RW_ALL 0666 //Permessi di lettura e scrittura per tutti i processi dell

/*
    * Struttura del messaggio.
    
*/
typedef struct {
    long mtype;
    char mtext[MESS_SIZE];
} msg_buffer;

/**
 * Struttura del messaggio contenente l'informazione dell'atomo.
 */
typedef struct {
    int n_atom; /**< Numero atomico dell'atomo */
    pid_t pid; /**< PID del processo creato */
} atom;
/**
 * Struttura per le registrazione di una statistica con valore totale e relativo all'ultimo secondo.
 */
typedef struct {
    int totale;       // Valore totale
    int ultimo_secondo; // Valore relativo all'ultimo secondo
} Statistica;
// Struttura per le statistiche della simulazione
typedef struct {
    Statistica Nattivazioni;     // Numero di attivazioni
    Statistica Nscissioni;       // Numero di scissioni
    Statistica energia_prodotta; // Quantità di energia prodotta
    Statistica energia_consumata; // Quantità di energia consumata
    Statistica scorie_prodotte;  // Quantità di scorie prodotte
} Statistiche;

extern int *N_ATOMI_INIT; /** Numero iniziale di atomi */
extern int *N_ATOM_MAX; /** Numero massimo di atomi */
extern int *MIN_N_ATOMICO; /** Numero atomico minimo */
extern int *ENERGY_DEMAND; /** Domanda di energia */
extern int *STEP; /** Passo per la variazione dell'energia */
extern int *N_NUOVI_ATOMI; /** Numero di nuovi atomi */
extern int *SIM_DURATION; /** Durata della simulazione */
extern int *ENERGY_EXPLODE_THRESHOLD; /** Soglia di esplosione dell'energia */
extern int *PID_MASTER; /** PID del processo master */
extern pid_t *ATOMO_GPID; /** Gruppo di processi degli atomi */ 
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
/**
 * Funzione che invia un messaggio di inizio simulazione.
 */
void sendStartSimulationMessage(int msqid) ;
/**
 * Funzione che riceve un messaggio di inizio simulazione.
 */
void receiveStartSimulationMessage(int msqid,int attivatore);

#endif // LIB_H