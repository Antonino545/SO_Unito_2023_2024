#include "lib.h"
#include <errno.h>

/**
 * @file master.c
 * @brief Programma principale per la simulazione di atomi e processi correlati.
 * @author Antonino Incorvaia,  Désirée Gaudio
 * Questo programma crea e gestisce processi per simulare atomi, un attivatore e un alimentatore,
 * utilizzando memoria condivisa e una coda di messaggi per la comunicazione tra processi.
 *
 * Le principali funzionalità del programma includono:
 * - Configurazione della memoria condivisa.
 * - Creazione di processi per la simulazione di atomi, attivatori e alimentatori.
 * - Gestione della durata della simulazione.
 * - Pulizia della memoria condivisa e della coda di messaggi al termine della simulazione o in caso di errore.
 */

/**
 * Puntatori alle variabili nella memoria condivisa.
 */

Statistiche *statistiche; // Statistiche della simulazione in memoria condivisa
int msqid;                // ID della coda di messaggi
int sem_id;               // ID del semaforo
pid_t attivatore_pid;     // PID del processo attivatore
pid_t alimentazione_pid;  // PID del processo alimentazione

/**
 * Funzione per bloccare il semaforo.
 */
void semLock(int sem_id)
{
    struct sembuf sb = {0, -1, 0}; // Operazione di lock
    if (semop(sem_id, &sb, 1) == -1)
    {
        perror("semop lock");
        exit(EXIT_FAILURE);
    }
}

/**
 * Funzione per sbloccare il semaforo.
 */
void semUnlock(int sem_id)
{
    struct sembuf sb = {0, 1, 0}; // Operazione di unlock
    if (semop(sem_id, &sb, 1) == -1)
    {
        perror("semop unlock");
        exit(EXIT_FAILURE);
    }
}

/**
 * Stampa le statistiche della simulazione.
 * Usa un semaforo per garantire che nessun altro processo modifichi le statistiche durante la stampa.
 */
void printStats()
{
    if (statistiche == NULL)
    {
        fprintf(stderr, "[ERROR] statistiche pointer is NULL in printStats\n");
        exit(EXIT_FAILURE);
    }

    // Now, check if the fields inside statistiche are initialized
    printf("[DEBUG] statistiche initialized: Nattivazioni: %d, Nscissioni: %d\n",
           statistiche->Nattivazioni.totale,
           statistiche->Nscissioni.totale);

    semLock(sem_id); // Blocco del semaforo

    printf("[INFO] Master (PID: %d): Statistiche della simulazione\n", getpid());
    printf("[INFO] Master (PID: %d): Attivazioni totali: %d\n", getpid(), statistiche->Nattivazioni.totale);
    printf("[INFO] Master (PID: %d): Attivazioni ultimo secondo: %d\n", getpid(), statistiche->Nattivazioni.ultimo_secondo);
    printf("[INFO] Master (PID: %d): Scissioni totali: %d\n", getpid(), statistiche->Nscissioni.totale);
    printf("[INFO] Master (PID: %d): Scissioni ultimo secondo: %d\n", getpid(), statistiche->Nscissioni.ultimo_secondo);
    printf("[INFO] Master (PID: %d): Energia prodotta totale: %d\n", getpid(), statistiche->energia_prodotta.totale);
    printf("[INFO] Master (PID: %d): Energia prodotta ultimo secondo: %d\n", getpid(), statistiche->energia_prodotta.ultimo_secondo);
    printf("[INFO] Master (PID: %d): Energia consumata totale: %d\n", getpid(), statistiche->energia_consumata.totale);
    printf("[INFO] Master (PID: %d): Energia consumata ultimo secondo: %d\n", getpid(), statistiche->energia_consumata.ultimo_secondo);
    printf("[INFO] Master (PID: %d): Scorie prodotte totali: %d\n", getpid(), statistiche->scorie_prodotte.totale);
    printf("[INFO] Master (PID: %d): Scorie prodotte ultimo secondo: %d\n", getpid(), statistiche->scorie_prodotte.ultimo_secondo);

    semUnlock(sem_id); // Sblocco del semaforo
}

/**
 * Funzione per aggiornare le statistiche, protetta da semafori.
 * Gli altri processi chiameranno questa funzione per aggiornare i dati.
 */
void updateStats(int attivazioni, int scissioni, int energia_prod, int energia_cons, int scorie)
{
    semLock(sem_id); // Blocco del semaforo

    statistiche->Nattivazioni.totale += attivazioni;
    statistiche->Nattivazioni.ultimo_secondo = attivazioni;
    statistiche->Nscissioni.totale += scissioni;
    statistiche->Nscissioni.ultimo_secondo = scissioni;
    statistiche->energia_prodotta.totale += energia_prod;
    statistiche->energia_prodotta.ultimo_secondo = energia_prod;
    statistiche->energia_consumata.totale += energia_cons;
    statistiche->energia_consumata.ultimo_secondo = energia_cons;
    statistiche->scorie_prodotte.totale += scorie;
    statistiche->scorie_prodotte.ultimo_secondo = scorie;

    semUnlock(sem_id); // Sblocco del semaforo
}

/**
 * Funzione di pulizia che rimuove il semaforo e la memoria condivisa.
 */
void cleanup()
{
    printf("[CLEANUP] Master (PID: %d): Avvio della pulizia\n", getpid());
    kill(attivatore_pid, SIGINT);    // Invia il segnale di terminazione al processo attivatore
    kill(alimentazione_pid, SIGINT); // Invia il segnale di terminazione al processo alimentazione
    killpg(getpid(), SIGTERM);       // Invia il segnale di terminazione a tutti i processi figli

    // Attende la terminazione di tutti i processi figli
    while (wait(NULL) != -1)
        ;

    // Rimozione del semaforo
    if (semctl(sem_id, 0, IPC_RMID) == -1)
    {
        perror("[ERROR] Master: Errore durante la rimozione del semaforo");
    }

    // Rimozione della coda di messaggi
    if (msgctl(msqid, IPC_RMID, NULL) < 0)
    {
        perror("[ERROR] Master: Errore durante la rimozione della coda di messaggi");
    }
}

/**
 * Questa funzione viene lanciata quando il processo riceve un segnale di meltdown.
 * @param sig Il segnale ricevuto.
 */
void handle_meltdown(int sig)
{
    cleanup();
    printf("[TERMINATION] Master (PID: %d): Simulazione terminata a causa di un meltdown. Chiusura programma.\n", getpid());
    exit(EXIT_SUCCESS);
}
/**
 * Questa funzione viene lanciata quando il processo riceve un segnale di interruzione.
 * @param sig Il segnale ricevuto.
 */
void handle_interruption(int sig)
{
    cleanup();
    printf("[TERMINATION] Master (PID: %d): Simulazione terminata a causa della ricezione di un segnale di interruzione. Chiusura programma.\n", getpid());
    exit(EXIT_SUCCESS);
}
void handle_sigusr2(int signum)
{
    // Custom logic for handling SIGUSR2
    printf("[INFO] Master (PID: %d): Ricevuto SIGUSR2 , ma lo ignoro perche destinato agli atomi\n", getpid());
}
void handle_sigterm_master(int sig)
{
    // Ignora SIGTERM nel master
    printf("[INFO] Master %d: Ricevuto SIGTERM, ma lo ignoro perche destinato agli atomi\n", getpid());
}
/**
 * Questa funzione gestisce come il processo deve comportarsi quando riceve un segnale di meltdown.
 */
void setup_signal_handler()
{
    struct sigaction sa; // Struttura per la gestione dei segnali
    bzero(&sa, sizeof(sa));
    sa.sa_handler = handle_meltdown; // Imposta la funzione di gestione del segnale;
    sigemptyset(&sa.sa_mask);        // Inizializza il set dei segnali bloccati durante l'esecuzione della funzione di gestione
    sigaction(SIGUSR1, &sa, NULL);   // Imposta la gestione del segnale SIGUSR1
    struct sigaction interrupt_sa;
    interrupt_sa.sa_handler = handle_interruption;
    sigemptyset(&interrupt_sa.sa_mask);
    sigaction(SIGINT, &interrupt_sa, NULL);
    struct sigaction sa2;
    sa2.sa_handler = handle_sigusr2;
    sigemptyset(&sa2.sa_mask);
    sigaction(SIGUSR2, &sa2, NULL);
    struct sigaction sa_term;
    sa_term.sa_handler = handle_sigterm_master;
    sa_term.sa_flags = 0;
    sigemptyset(&sa_term.sa_mask);
    sigaction(SIGTERM, &sa_term, NULL);
}

/*
 * Crea un nuovo processo figlio per eseguire il programma `atomo` con un numero atomico casuale.
 */
void createAtomo()
{
    int numero_atomico = generate_random(*N_ATOM_MAX);
    char num_atomico_str[20];
    snprintf(num_atomico_str, sizeof(num_atomico_str), "%d", numero_atomico);

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("[ERROR] Master: Fork fallita durante la creazione di un atomo");
        kill(*PID_MASTER, SIGUSR1);
    }
    else if (pid == 0)
    {
        printf("[INFO] Atomo (PID: %d): Avvio processo atomo con numero atomico %d\n", getpid(), numero_atomico);
        if (execlp("./atomo", "atomo", num_atomico_str, NULL) == -1)
        {
            perror("[ERROR] Atomo: execlp fallito durante l'esecuzione del processo atomo");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
    }
}
/**
 * Crea un nuovo processo figlio per eseguire il programma `attivatore`.
 */
void createAttivatore()
{
    pid_t pid = fork(); // Crea un nuovo processo

    if (pid < 0)
    { // Errore nella creazione del processo
        perror("[ERROR] Master: Fork fallita durante la creazione di un attivatore");
        kill(*PID_MASTER, SIGUSR1);
    }
    else if (pid == 0)
    { // Processo figlio
        printf("[INFO] Attivatore (PID: %d): Avvio processo attivatore\n", getpid());

        // Esegue `attivatore`
        if (execlp("./attivatore", "attivatore", NULL) == -1)
        {
            perror("[ERROR] Attivatore: execlp fallito durante l'esecuzione del processo attivatore");
            exit(EXIT_FAILURE);
        }
    }
    else
    { // Processo padre
        attivatore_pid = pid;
        printf("[INFO] Master (PID: %d): Processo attivatore creato con PID: %d\n", getpid(), pid);
    }
}

/**
 * Crea un nuovo processo figlio per eseguire il programma `alimentazione`.
 */
void createAlimentazione()
{
    pid_t pid = fork(); // Crea un nuovo processo

    if (pid < 0)
    { // Errore nella creazione del processo
        perror("[ERROR] Master: Fork fallita durante la creazione di un Alimentazione");
        kill(*PID_MASTER, SIGUSR1);
    }
    else if (pid == 0)
    { // Processo figlio
        printf("[INFO] Alimentazione (PID: %d): Avvio processo alimentazione\n", getpid());

        // Esegue `alimentazione`
        if (execlp("./alimentazione", "alimentazione", NULL) == -1)
        {
            perror("[ERROR] Alimentazione: execlp fallito durante l'esecuzione del processo alimentazione");
            exit(EXIT_FAILURE);
        }
    }
    else
    { // Processo padre
        alimentazione_pid = pid;
        printf("[INFO] Master (PID: %d): Processo alimentazione creato con PID: %d\n", getpid(), pid);
    }
}

/**
 * Legge i parametri dal file di configurazione e li memorizza nella memoria condivisa.
 * @param file Puntatore al file di configurazione aperto.
 */
void readparameters(FILE *file)
{
    if (file == NULL)
    { // Verifica che il file sia stato aperto correttamente
        perror("[ERROR] Master: Impossibile aprire il file di configurazione");
        exit(EXIT_FAILURE);
    }
    *PID_MASTER = getpid();
    printf("[DEBUG] Master: PID impostato a %d\n", getpid());
    char line[256]; // Buffer per leggere ogni linea del file
    while (fgets(line, sizeof(line), file))
    {                                    // Legge il file riga per riga
        line[strcspn(line, "\r\n")] = 0; // Rimuove il carattere di newline

        char key[128];
        int value;

        // Parsea la linea nel formato chiave=valore
        if (sscanf(line, "%127[^=]=%d", key, &value) == 2)
        {
            if (strcmp(key, "N_ATOMI_INIT") == 0)
            {
                *N_ATOMI_INIT = value;
                printf("[DEBUG] Master: N_ATOMI_INIT impostato a %d\n", value);
            }
            else if (strcmp(key, "N_ATOM_MAX") == 0)
            {
                *N_ATOM_MAX = value;
                printf("[DEBUG] Master: N_ATOM_MAX impostato a %d\n", value);
            }
            else if (strcmp(key, "MIN_N_ATOMICO") == 0)
            {
                *MIN_N_ATOMICO = value;
                printf("[DEBUG] Master: MIN_N_ATOMICO impostato a %d\n", value);
            }
            else if (strcmp(key, "ENERGY_DEMAND") == 0)
            {
                *ENERGY_DEMAND = value;
                printf("[DEBUG] Master: ENERGY_DEMAND impostato a %d\n", value);
            }
            else if (strcmp(key, "STEP") == 0)
            {
                *STEP = value;
                printf("[DEBUG] Master: STEP impostato a %d\n", value);
            }
            else if (strcmp(key, "N_NUOVI_ATOMI") == 0)
            {
                *N_NUOVI_ATOMI = value;
                printf("[DEBUG] Master: N_NUOVI_ATOMI impostato a %d\n", value);
            }
            else if (strcmp(key, "SIM_DURATION") == 0)
            {
                *SIM_DURATION = value;
                printf("[DEBUG] Master: SIM_DURATION impostato a %d\n", value);
            }
            else if (strcmp(key, "ENERGY_EXPLODE_THRESHOLD") == 0)
            {
                *ENERGY_EXPLODE_THRESHOLD = value;
                printf("[DEBUG] Master: ENERGY_EXPLODE_THRESHOLD impostato a %d\n", value);
            }
        }
    }

    fclose(file); // Chiude il file dopo aver letto tutti i parametri
}

/**
 * Funzione principale del programma.
 * - Configura e mappa la memoria condivisa.
 * - Legge i parametri dal file di configurazione.
 * - Crea processi per la simulazione e gestisce la loro esecuzione.
 * - Gestisce la durata della simulazione e la pulizia finale.
 * @return Codice di uscita del programma.
 */
int main()
{
    setpgid(0, 0);
    printf("[INFO] Master (PID: %d): Inizio esecuzione del programma principale il mio gruppo di processi è %d\n", getpid(), getpgrp());

    sem_id = getSemaphoreSet(); // Initialize semaphore
    printf("[INFO] Master (PID: %d): Semaphore set initialized with ID: %d\n", getpid(), sem_id);

    // Configura la memoria condivisa
    const char *shm_name = "/Parametres";
    const size_t shm_size = 10 * sizeof(int); // Dimensione della memoria condivisa (8 interi)
    void *shmParamsPtr = create_shared_memory(shm_name, shm_size);

    // Puntatori alle variabili nella memoria condivisa
    N_ATOMI_INIT = (int *)shmParamsPtr;
    N_ATOM_MAX = (int *)(shmParamsPtr + sizeof(int));
    MIN_N_ATOMICO = (int *)(shmParamsPtr + 2 * sizeof(int));
    ENERGY_DEMAND = (int *)(shmParamsPtr + 3 * sizeof(int));
    STEP = (int *)(shmParamsPtr + 4 * sizeof(int));
    N_NUOVI_ATOMI = (int *)(shmParamsPtr + 5 * sizeof(int));
    SIM_DURATION = (int *)(shmParamsPtr + 6 * sizeof(int));
    ENERGY_EXPLODE_THRESHOLD = (int *)(shmParamsPtr + 7 * sizeof(int));
    PID_MASTER = (int *)(shmParamsPtr + 8 * sizeof(int));

    printf("[INFO] Master (PID: %d): Memoria condivisa mappata con successo. Inizio lettura del file di configurazione\n", getpid());

    // Configura la memoria condivisa per le statistiche
    const char *shm_stats_name = "/Statistics";                               // Nome della memoria condivisa per le statistiche
    const size_t shm_stats_size = sizeof(Statistiche);                        // Dimensione della memoria condivisa per le statistiche
    void *shmStatsPtr = create_shared_memory(shm_stats_name, shm_stats_size); // Puntatori alle statistiche

    // Check if shared memory for stats was created successfully
    if (shmStatsPtr == NULL)
    {
        fprintf(stderr, "[ERROR] Shared memory for statistics could not be created\n");
        cleanup();
        exit(EXIT_FAILURE);
    }

    Statistiche *stats = (Statistiche *)shmStatsPtr; // Puntatore alla struttura Statistiche
    memset(stats, 0, shm_stats_size);

    // Check if stats pointer is initialized properly
    if (stats == NULL)
    {
        fprintf(stderr, "[ERROR] stats pointer is NULL\n");
        exit(EXIT_FAILURE);
    }
    printf("[DEBUG] stats initialized successfully: %p\n", (void *)stats);

    printf("[INFO] Master (PID: %d): Memoria condivisa per le statistiche creata e inizializzata con successo.\n", getpid());
    printStats();

    // Apri il file di configurazione e leggi i parametri
    FILE *file = fopen("../Data/parameters.txt", "r");
    if (file == NULL)
    {
        perror("[ERROR] Master: Impossibile aprire il file di configurazione");
        cleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        readparameters(file);
    }

    // Stampa informazioni di inizio simulazione
    printf("[INFO] Master (PID: %d): Parametri letti dal file di configurazione. Inizio creazione Proccessi iniziali\n", getpid());

    // Creazione della coda di messaggi
    key_t key = MESSAGE_QUEUE_KEY;
    if ((msqid = msgget(key, IPC_CREAT | MES_PERM_RW_ALL)) < 0)
    {
        perror("[ERROR] Master: Errore durante la creazione della coda di messaggi (msgget fallita)");
        exit(EXIT_FAILURE);
    }
    printf("[INFO] Master (PID: %d): Coda di messaggi creata con successo\n", getpid());
    printf("[INFO] Master (PID: %d): Inizializzo la gestione del segnale di meltdown\n", getpid());
    setup_signal_handler(); // Imposta il gestore del segnale per il meltdown

    printf("[INFO] Master (PID: %d): Inizio creazione atomi iniziali\n", getpid());
    for (int i = 0; i < *N_ATOMI_INIT; i++)
    {
        createAtomo();
    }

    // Ricezione dei messaggi dai processi
    waitForNInitMsg(msqid, *N_ATOMI_INIT);

    printf("[INFO] Master (PID: %d): Fine creazione atomi iniziali\n", getpid());
    printf("---------------------------------------\n");
    // Creazione dei processi necessari
    printf("[INFO] Master (PID: %d): Creazione del processo alimentatore\n", getpid());
    createAlimentazione();
    waitForNInitMsg(msqid, 1);
    printf("---------------------------------------\n");
    printf("[INFO] Master (PID: %d): Creazione del processo attivatore\n", getpid());
    createAttivatore();
    waitForNInitMsg(msqid, 1);
    printf("---------------------------------------\n");
    // Avvio della simulazione principale
    printf("[IMPORTANT] Master (PID: %d): Processi creati con successo. Inizio simulazione principale\n", getpid());
    int termination = 0;
    printf("Attivatore PID: %d\n", attivatore_pid);
    printf("Alimentazione PID: %d\n", alimentazione_pid);
    sendStartSimulationSignal(attivatore_pid, alimentazione_pid);
    while (*SIM_DURATION > 0)
    {
        printf("[INFO] SIM_DURATION attuale: %d\n", *SIM_DURATION);
        /* if(energy<*ENERGY_DEMAND){
             printf("[INFO] Master (PID: %d): Energia attuale: %d\n", getpid(), energy);
             printf("[INFO] Master (PID: %d): Energia richiesta: %d\n", getpid(), *ENERGY_DEMAND);
             printf("[INFO] Master (PID: %d): Energia insufficiente simulazione terminata\n", getpid());
             termination=1;
             break;
         }*/
        // Esegui l'azione desiderata qui, ad esempio una pausa di 1 secondo
        (*SIM_DURATION)--;
        struct timespec my_time;
        my_time.tv_sec = 1;
        my_time.tv_nsec = 0;
        nanosleep(&my_time, NULL); // Uso nanosleep per aspettare un secondo invece di sleep per evitare che il processo venga interrotto da un segnale
        printStats();
    }
    cleanup();
    if (termination == 0)
    {
        printf("[TERMINATION] Master (PID: %d): Simulazione terminata con successo. Chiusura programma.\n", getpid());
    }
    else if (termination == 1)
    {
        printf("[TERMINATION] Master (PID: %d): Simulazione terminata a causa di blackout . Chiusura programma.\n", getpid());
    }
    //
    exit(EXIT_SUCCESS);
}
