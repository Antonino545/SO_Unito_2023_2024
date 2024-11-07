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

// Message types
#define ATOMO_INIT_MSG 1           // Tipo di messaggio per l'inizializzazione di un atomo
#define ATTIVATORE_INIT_MSG 2      // Tipo di messaggio per l'inizializzazione di un attivatore
#define ALIMENTAZIONE_INIT_MSG 3   // Tipo di messaggio per l'inizializzazione di un alimentatore
#define INIBITORE_INIT_MSG 4       // Tipo di messaggio per l'inizializzazione di un inibitore
#define MESS_SIZE 30               // Dimensione massima del messaggio
#define MESSAGE_QUEUE_KEY 1234     // Key della coda di messaggi
#define SEMAPHORE_STATS_KEY 12345  // Key dei semafori per le statistiche
#define SEMAPHORE_START_KEY 12346  // Key dei semafori per l'avvio della simulazione
#define SEMAPHORE_INIBITORE_KEY 44444 // Key dei semafori per l'inibitore
#define MES_PERM_RW_ALL 0666       // Permessi di lettura e scrittura per tutti i processi

/**
 * Struttura del messaggio.
 */
typedef struct {
    long mtype;
    char mtext[MESS_SIZE];
} msg_buffer;

/**
 * Struttura per le registrazione di una statistica con valore totale e relativo all'ultimo secondo.
 */
typedef struct {
    struct {
        int totale;
        int ultimo_secondo;
    } Nattivazioni;
    struct {
        int totale;
        int ultimo_secondo;
    } Nscissioni;
    struct {
        int totale;
        int ultimo_secondo;
    } energia_prodotta;
    struct {
        int totale;
        int ultimo_secondo;
    } energia_consumata;
    struct {
        int totale;
        int ultimo_secondo;
    } scorie_prodotte;
    struct {
        int totale;
        int ultimo_secondo;
    } energia_assorbita;
    struct {
        int totale;
        int ultimo_secondo;
    } bilanciamento;
} Statistiche;

// Variabili globali
extern int *N_ATOMI_INIT;             // Numero iniziale di atomi
extern int *N_ATOM_MAX;               // Numero atomico massimo
extern int *MIN_N_ATOMICO;            // Numero atomico minimo
extern int *ENERGY_DEMAND;            // Energia richiesta
extern int *STEP;                     // Passo per la variazione dell'energia
extern int *N_NUOVI_ATOMI;            // Numero di nuovi atomi
extern int *SIM_DURATION;             // Durata della simulazione
extern int *ENERGY_EXPLODE_THRESHOLD; // Soglia di esplosione dell'energia
extern int *PID_MASTER;               // PID del processo master
extern int *isCleaning;               // Flag che indica se la pulizia è in corso
extern Statistiche *stats;            // Puntatore alle statistiche
extern int sem_stats;                 // ID del semaforo per le statistiche
extern int sem_start;                 // ID del semaforo per l'avvio della simulazione
extern int sem_inibitore;             // ID del semaforo per l'inibitore
extern int *PID_GROUP_ATOMO;          // PID del gruppo di processi degli atomi
extern int *isinibitoreactive;        // Flag che indica se l'inibitore è attivo

/**
 * Genera un numero casuale tra 1 e `max`.
 * @param max Il valore massimo che può essere generato.
 * @return Un numero casuale tra 1 e `max`.
 */
int generate_random(int max);

/* Macro per il massimo tra due numeri */
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
 * @return Puntatore alla memoria condivisa per i parametri.
 */
void *allocateParametresMemory();

/**
 * Funzione che alloca la memoria condivisa per le statistiche.
 * @return Puntatore alla memoria condivisa per le statistiche.
 */
void *allocateStatisticsMemory();

/**
 * Accede alla memoria condivisa contenente una struttura `Statistiche`.
 * @return Puntatore alla struttura `Statistiche`.
 */
Statistiche *accessStatisticsMemory();



/**
 * Funzione per sbloccare il semaforo.
 * @param sem_stats ID del semaforo da sbloccare.
 */
void semUnlock(int semid);

/**
 * Funzione per decrementa il semaforo di 1
 * @param semid ID del semaforo.
 */
void semwait(int semid);

/**
 * Funzione per aggiornare le statistiche, protetta da semafori.
 * @param attivazioni Numero di attivazioni effettuate.
 * @param scissioni Numero di scissioni effettuate.
 * @param energia_prod Energia prodotta.
 * @param energia_cons Energia consumata.
 * @param scorie Scorie prodotte.
 * @param energia_assorbita Energia assorbita dall'inibitore.
 * @param bilanciamento Bilanciamento.
 */
void updateStats(int attivazioni, int scissioni, int energia_prod, int energia_cons, int scorie, int energia_assorbita, int bilanciamento);

/**
 * Funzione che invia un messaggio formattato al processo master.
 * @param msqid ID della coda di messaggi.
 * @param type Tipo del messaggio.
 * @param messagetext Testo del messaggio.
 */
void send_message(int msqid, long type, char *messagetext) ;

/**
 * Funzione che attende un messaggio di inizializzazione da un processo.
 * @param msqid ID della coda di messaggi.
 * @param n Numero di messaggi di inizializzazione da attendere.
 */
void waitForNInitMsg(int msqid, int n);

/**
 * Funzione per rimuovere il set di semafori.
 * @param semid ID del set di semafori.
 */
void removeSemaphoreSet(int semid);

/**
 * Funzione per ottenere l'ID del set di semafori per le statistiche.
 * @return ID del set di semafori.
 */
int getSemaphoreStatsSets();

/**
 * Funzione per ottenere l'ID del set di semafori per l'avvio della simulazione.
 * @return ID del set di semafori.
 */
int getSemaphoreStartset();

/**
 * Funzione per ottenere l'ID del set di semafori per l'inibitore.
 * @return ID del set di semafori.
 */
int getSemaphoreInibitoreSet();

/**
 * Funzione per verificare se il semaforo è sbloccato.
 * @param semid ID del semaforo.
 * @return 1 se il semaforo è sbloccato, 0 altrimenti.
 */
int isSemaphoreUnlocked(int semid);

#endif // LIB_H