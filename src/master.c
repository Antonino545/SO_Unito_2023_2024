#include "lib.h"

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

Statistiche *stats;      // Statistiche della simulazione in memoria condivisa
int msqid;               // ID della coda di messaggi
int sem_id;              // ID del semaforo
pid_t attivatore_pid;    // PID del processo attivatore
pid_t alimentazione_pid; // PID del processo alimentazione

/**
 * Stampa le statistiche della simulazione.
 * Usa un semaforo per garantire che nessun altro processo modifichi le statistiche durante la stampa.
 */
void printStats()
{
    if (stats == NULL)
    {
        fprintf(stderr, "[ERROR] statistiche pointer is NULL in printStats\n");
        exit(EXIT_FAILURE);
    }

    // Now, check if the fields inside statistiche are initialized
    printf("[DEBUG] statistiche initialized: Nattivazioni: %d, Nscissioni: %d\n",
           stats->Nattivazioni.totale,
           stats->Nscissioni.totale);

    semLock(sem_id); // Blocco del semaforo

    printf("[INFO] Master (PID: %d): Statistiche della simulazione\n", getpid());
    printf("[INFO] Master (PID: %d): Attivazioni totali: %d\n", getpid(), stats->Nattivazioni.totale);
    printf("[INFO] Master (PID: %d): Attivazioni ultimo secondo: %d\n", getpid(), stats->Nattivazioni.ultimo_secondo);
    printf("[INFO] Master (PID: %d): Scissioni totali: %d\n", getpid(), stats->Nscissioni.totale);
    printf("[INFO] Master (PID: %d): Scissioni ultimo secondo: %d\n", getpid(), stats->Nscissioni.ultimo_secondo);
    printf("[INFO] Master (PID: %d): Energia prodotta totale: %d\n", getpid(), stats->energia_prodotta.totale);
    printf("[INFO] Master (PID: %d): Energia prodotta ultimo secondo: %d\n", getpid(), stats->energia_prodotta.ultimo_secondo);
    printf("[INFO] Master (PID: %d): Energia consumata totale: %d\n", getpid(), stats->energia_consumata.totale);
    printf("[INFO] Master (PID: %d): Energia consumata ultimo secondo: %d\n", getpid(), stats->energia_consumata.ultimo_secondo);
    printf("[INFO] Master (PID: %d): Scorie prodotte totali: %d\n", getpid(), stats->scorie_prodotte.totale);
    printf("[INFO] Master (PID: %d): Scorie prodotte ultimo secondo: %d\n", getpid(), stats->scorie_prodotte.ultimo_secondo);

    semUnlock(sem_id); // Sblocco del semaforo
}

void receive_energia_liberata_message()
{
    // Struttura del messaggio
    struct msgbuf
    {
        long mtype;  // Tipo del messaggio
        int energia; // Energia liberata
    } message;

    // Riceviamo il messaggio dalla coda (mtype = 1 per il messaggio di energia)
    ssize_t result = msgrcv(msqid, &message, sizeof(message.energia), 1, 0);

    // Verifica se la ricezione è andata a buon fine
    if (result == -1)
    {
        perror("[ERROR] Master: Ricezione del messaggio fallita");
        return; // Continua l'esecuzione senza terminare il programma
    }

    // Verifica che l'energia ricevuta sia un valore positivo e sensato
    if (message.energia < 0)
    {
        fprintf(stderr, "[WARNING] Master (PID: %d): Energia ricevuta negativa: %d\n", getpid(), message.energia);
        return; // Ignora il messaggio se l'energia è negativa
    }

    // Aggiorna le statistiche nel master
    printf("[INFO] Master (PID: %d): Ricevuto messaggio con energia liberata = %d\n", getpid(), message.energia);

    // Chiamata per aggiornare le statistiche: incrementa scissioni (1) e aggiorna energia prodotta
    updateStats(0, 1, message.energia, 0, 0);
}

/**
 * Funzione di pulizia che rimuove il semaforo e la memoria condivisa.
 */
void cleanup()
{
    printf("------------------------------------------------------------\n");
    printf("[CLEANUP] Master (PID: %d): Avvio della pulizia\n", getpid());
    kill(alimentazione_pid, SIGINT); // Invia il segnale di terminazione al processo alimentazione
    kill(attivatore_pid, SIGINT);    // Invia il segnale di terminazione al processo attivatore
    killpg(getpid(), SIGTERM);       // Invia il segnale di terminazione a tutti i processi figli
    while (wait(NULL) > 0)
        ; // Aspetta che tutti i processi figli terminino

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
    printf("[INFO] Master (PID: %d): Ricevuto SIGUSR2 , ma lo ignoro perche destinato agli atomi\n", getpid());
}
void handle_sigterm_master(int sig)
{
    printf("[INFO] Master %d: Ricevuto SIGTERM, ma lo ignoro perche destinato agli atomi\n", getpid());
}
/**
 * Questa funzione gestisce come il processo deve comportarsi quando riceve un segnale di meltdown.
 */
void setup_signal_handler()
{
    sigaction(SIGUSR2, &(struct sigaction){.sa_handler = handle_sigusr2}, NULL);
    sigaction(SIGTERM, &(struct sigaction){.sa_handler = handle_sigterm_master}, NULL);
    sigaction(SIGINT, &(struct sigaction){.sa_handler = handle_interruption}, NULL);
    sigaction(SIGUSR1, &(struct sigaction){.sa_handler = handle_meltdown}, NULL);
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

    stats = (Statistiche *)shmStatsPtr; // Puntatore alla struttura Statistiche

    // Check if stats pointer is initialized properly
    if (stats == NULL)
    {
        perror("[ERROR] Master: Errore durante l'inizializzazione delle statistiche");
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
    printf("[INFO] Master (PID: %d): Creazione del processo attivatore\n", getpid());
    createAttivatore();
    waitForNInitMsg(msqid, 1);
    printf("---------------------------------------\n");
    // Creazione dei processi necessari
    printf("[INFO] Master (PID: %d): Creazione del processo alimentatore\n", getpid());
    createAlimentazione();
    waitForNInitMsg(msqid, 1);
    printf("---------------------------------------\n");

    // Avvio della simulazione principale
    printf("[IMPORTANT] Master (PID: %d): Processi creati con successo. Inizio simulazione principale\n", getpid());
    int termination = 0;

    printf("Attivatore PID: %d\n", attivatore_pid);
    printf("Alimentazione PID: %d\n", alimentazione_pid);
    kill(alimentazione_pid, SIGUSR1);
    send_message(msqid, DIVISION_MSG, "inizia a dividere gli atomi", getpid());

    while (*SIM_DURATION > 0)
    {
        printf("------------------------------------------------------------\n");
        printf("[INFO] SIM_DURATION attuale: %d\n", *SIM_DURATION);
        /* if(energy<*ENERGY_DEMAND){
             printf("[INFO] Master (PID: %d): Energia attuale: %d\n", getpid(), energy);
             printf("[INFO] Master (PID: %d): Energia richiesta: %d\n", getpid(), *ENERGY_DEMAND);
             printf("[INFO] Master (PID: %d): Energia insufficiente simulazione terminata\n", getpid());
             termination=1;
             break;
         }*/
        // Esegui l'azione desiderata qui, ad esempio una pausa di 1 secondo

        // Ricezione messaggi di energia liberata dalle scissioni
        receive_energia_liberata_message();

        (*SIM_DURATION)--;
        nanosleep((const struct timespec[]){{1, 0}}, NULL); // Ogni secondo

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
    exit(EXIT_SUCCESS);
}
