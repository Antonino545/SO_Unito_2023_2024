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
#include <signal.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <errno.h>

#define ATOMO_INIT_MSG 1
#define ATTIVATORE_INIT_MSG 2
#define ALIMENTAZIONE_INIT_MSG 3
#define TERMINATION_MSG 4      // Tipo di messaggio per la terminazione dell'atomo
#define DIVISION_MSG 5         // Tipo di messaggio per la divisione dell'atomo
#define MSG_TYPE_START_SIM 5   // Tipo di messaggio per l'inizio della simulazione
#define MESS_SIZE 30           // Dimensione massima del messaggio
#define MESSAGE_QUEUE_KEY 1234 // Key della coda di messaggi
#define SEMAPHORE_KEY 12345    // Key dei semafori
#define MES_PERM_RW_ALL 0666   // Permessi di lettura e scrittura per tutti i processi dell

/**
 * Struttura del messaggio.
 */
typedef struct
{
    long mtype;
    char mtext[MESS_SIZE];
} msg_buffer;



/**
 * Struttura per le registrazione di una statistica con valore totale e relativo all'ultimo secondo.
 */
typedef struct
{
    struct
    {
        int totale;
        int ultimo_secondo;
    } Nattivazioni;
    struct
    {
        int totale;
        int ultimo_secondo;
    } Nscissioni;
    struct
    {
        int totale;
        int ultimo_secondo;
    } energia_prodotta;
    struct
    {
        int totale;
        int ultimo_secondo;
    } energia_consumata;
    struct
    {
        int totale;
        int ultimo_secondo;
    } scorie_prodotte;
} Statistiche;

extern int *N_ATOMI_INIT;             /** Numero iniziale di atomi */
extern int *N_ATOM_MAX;               /** Numero massimo di atomi */
extern int *MIN_N_ATOMICO;            /** Numero atomico minimo */
extern int *ENERGY_DEMAND;            /** Domanda di energia */
extern int *STEP;                     /** Passo per la variazione dell'energia */
extern int *N_NUOVI_ATOMI;            /** Numero di nuovi atomi */
extern int *SIM_DURATION;             /** Durata della simulazione */
extern int *ENERGY_EXPLODE_THRESHOLD; /** Soglia di esplosione dell'energia */
extern int *PID_MASTER;               /** PID del processo master */
extern int *isCleaning ;        // flag che indica se la pulizia è in corso

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
void *create_shared_memory(const char *shm_name, size_t shm_size);

/**
 * Funzione che imposta la memoria condivisa per i parametri.
 */
void *allocateParametresMemory();

/**
 * Funzione che alloca la memoria condivisa per le statistiche.
 * @return Puntatore alla memoria condivisa per le statistiche.
 */
void *allocateStatisticsMemory();

/**
 * @brief Accede alla memoria condivisa contenente una struttura `Statistiche`.
 *
 * Apre e mappa la memoria condivisa esistente `/Statistics`, restituendo un puntatore
 * alla struttura `Statistiche`. In caso di errore, stampa un messaggio e termina il programma.
 *
 * @return Statistiche* Puntatore alla struttura mappata.
 */
Statistiche *accessStatisticsMemory();

/**
 * Funzione per bloccare il semaforo.
 */
void semLock(int sem_id);

/**
 * Funzione per sbloccare il semaforo.
 */
void semUnlock(int sem_id);

/**
 * Funzione per aggiornare le statistiche, protetta da semafori.
 * Gli altri processi chiameranno questa funzione per aggiornare i dati.
 * @param attivazioni Numero di attivazioni effettuate
 * @param scissioni Numero di scissioni effettuate
 * @param energia_prod Energia prodotta
 * @param energia_cons Energia consumata
 * @param scorie Scorie prodotte
 *
 */
void updateStats(int attivazioni, int scissioni, int energia_prod, int energia_cons, int scorie);

/**
 * Funzione che invia un messaggio formattato al processo master.
 *
 * @param msqid ID della coda di messaggi
 * @param format Formato del messaggio (come printf)
 * @param type Tipo del messaggio
 * @param ... Argomenti variabili per il formato
 */
void send_message(int msqid, long type, const char *format, ...);

/**
 * Funzione che attende un messaggio di inizializzazione da un processo.
 *
 * @param msqid ID della coda di messaggi
 */
void waitForNInitMsg(int msqid, int n);
/**
 * Funzione che invia un messaggio di inizio simulazione.
 */
void sendStartSimulationSignal(pid_t attivatore_pid, pid_t alimentazione_pid);
/**
 * Funzione per ottenere l'ID del set di semafori.
 * @return L'ID del set di semafori.
 */
int getSemaphoreSet();
void removeSemaphoreSet(int semid);
void P(int semid); // Operazione di wait (down)
void V(int semid); // Operazione di signal (up)

#endif // LIB_H
