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

    sem_id = getSemaphoreSet();

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

void cleanup()
{
    *isCleaning = 1;
    printf("------------------------------------------------------------\n");
    printf("[INFO] Master (PID: %d): Avvio della pulizia\n", getpid());
    // Invia il segnale di terminazione a tutti i processi nel gruppo di processi
    if (attivatore_pid > 0)
        kill(attivatore_pid, SIGINT);
    if (alimentazione_pid > 0)
        kill(alimentazione_pid, SIGINT);
    if (alimentazione_pid < 0 && attivatore_pid < 0)
        killpg(*PID_MASTER, SIGTERM); // per gli atomi iniziali
    // timeout per evitare di rimanere bloccati indefinitamente
    time_t start_time = time(NULL);
    while (wait(NULL) > 0)
    {
        killpg(*PID_MASTER, SIGTERM); // serve per assicurare che tutti i processi atomo siano terminati
    }

    printf("\n------------------------------------------------------------\n");

    printStats();

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

    // Rimozione della memoria condivisa per le statistiche
    if (shm_unlink("/Statistics") == -1)
    {
        perror("[ERROR] Master: Errore durante la rimozione della memoria condivisa per le statistiche");
    }
}

/**
 *
 * Questa funzione viene lanciata quando il processo riceve un segnale di meltdown.
 * @param sig Il segnale ricevuto.
 */
void handle_meltdown(int sig)
{
    printf("[INFO] Master (PID: %d): Ricevuto segnale di meltdown. Inizio fase di cleanup\n", getpid());
    cleanup();
    printf("[TERMINATION] Master (PID: %d): Simulazione terminata a causa di un meltdown. Chiusura programma.\n", getpid());
    exit(EXIT_FAILURE);
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

/**
 * Questa funzione imposta i gestori di segnali per il processo:
 * - Ignora SIGUSR2 e SIGTERM.
 * - Gestisce SIGINT con la funzione handle_interruption.
 * - Gestisce SIGUSR1 con la funzione handle_meltdown
 */
void setup_signal_handler()
{
    signal(SIGUSR2, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    sigaction(SIGINT, &(struct sigaction){.sa_handler = handle_interruption}, NULL);
    signal(SIGUSR1, handle_meltdown);
}

/*
 * Crea un nuovo processo figlio per eseguire il programma `atomo` con un numero atomico casuale.
 */
void createAtomo()
{
    // Controlla se il processo è in fase di pulizia
    if (*isCleaning == 1)
    {
        printf("[INFO] Master: Impossibile creare nuovi processi. La fase di cleanup è in corso.\n");
        return; // Esce dalla funzione senza creare il nuovo processo
    }

    int numero_atomico = generate_random(*N_ATOM_MAX);
    char num_atomico_str[20];
    snprintf(num_atomico_str, sizeof(num_atomico_str), "%d", numero_atomico);

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("[ERROR] Master: Fork fallita durante la creazione di un atomo");
        handle_meltdown(SIGUSR1);
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
    pid_t pid = fork();

    if (pid < 0)
    { // Errore nella creazione del processo
        printf("[ERROR] Master(PID: %d): Fork fallita durante la creazione di un Attivatore\n", getpid());
        handle_meltdown(SIGUSR1);
    }
    else if (pid == 0)
    {
        printf("[INFO] Attivatore (PID: %d): Avvio processo attivatore\n", getpid());

        // Esegue `attivatore`
        if (execlp("./attivatore", "attivatore", NULL) == -1)
        {
            perror("[ERROR] Attivatore: execlp fallito durante l'esecuzione del processo attivatore");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        attivatore_pid = pid;
        printf("[INFO] Master (PID: %d): Processo attivatore creato con PID: %d\n", getpid(), pid);
    }
}

/**
 * Crea un nuovo processo figlio per eseguire il programma `alimentazione`.
 */
void createAlimentazione()
{
    pid_t pid = fork();

    if (pid < 0)
    { // Errore nella creazione del processo
        perror("[ERROR] Master: Fork fallita durante la creazione di un Alimentazione");
        handle_meltdown(SIGUSR1);
    }
    else if (pid == 0)
    {
        printf("[INFO] Alimentazione (PID: %d): Avvio processo alimentazione\n", getpid());

        // Esegue `alimentazione`
        if (execlp("./alimentazione", "alimentazione", NULL) == -1)
        {
            perror("[ERROR] Alimentazione: execlp fallito durante l'esecuzione del processo alimentazione");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
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
    {
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
 * - Imposta il gruppo di processi e i PID iniziali.
 * - Inizializza il set di semafori.
 * - Crea e mappa la memoria condivisa per i parametri e le statistiche.
 * - Legge i parametri di configurazione da un file.
 * - Crea la coda di messaggi e imposta i gestori di segnali.
 * - Crea i processi principali della simulazione: atomi, attivatore e alimentatore.
 * - Esegue la simulazione principale, monitorando la durata, il consumo di energia e le condizioni di arresto.
 * - Gestisce la terminazione della simulazione in caso di timeout, blackout o esplosione.
 * - Pulizia finale e rilascio delle risorse condivise.
 * @return Codice di uscita del programma.
 */
int main()
{
    setpgid(0, 0);
    alimentazione_pid = -1;
    attivatore_pid = -1;
    printf("[INFO] Master (PID: %d): Inizio esecuzione del programma principale il mio gruppo di processi è %d\n", getpid(), getpgrp());

    sem_id = getSemaphoreSet();
    printf("[INFO] Master (PID: %d): Semaphore set initialized with ID: %d\n", getpid(), sem_id);

    // Configura la memoria condivisa
    const char *shm_name = "/Parametres";
    const size_t shm_size = 10 * sizeof(int);
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
    isCleaning = (int *)(shmParamsPtr + 9 * sizeof(int));
    *isCleaning = 0;
    printf("[INFO] Master (PID: %d): Memoria condivisa mappata con successo. Inizio lettura del file di configurazione\n", getpid());

    // Configura la memoria condivisa per le statistiche
    const char *shm_stats_name = "/Statistics";                               // Nome della memoria condivisa per le statistiche
    const size_t shm_stats_size = sizeof(Statistiche);                        // Dimensione della memoria condivisa per le statistiche
    void *shmStatsPtr = create_shared_memory(shm_stats_name, shm_stats_size); // Puntatori alle statistiche

    // Controlla se la shared memory per stats è stata creata con successo
    if (shmStatsPtr == NULL)
    {
        fprintf(stderr, "[ERROR] Shared memory for statistics could not be created\n");
        cleanup();
        exit(EXIT_FAILURE);
    }

    stats = (Statistiche *)shmStatsPtr;

    if (stats == NULL)
    {
        perror("[ERROR] Master: Errore durante l'inizializzazione delle statistiche");
        exit(EXIT_FAILURE);
    }
    printf("[DEBUG] stats initialized successfully: %p\n", (void *)stats);

    printf("[INFO] Master (PID: %d): Memoria condivisa per le statistiche creata e inizializzata con successo.\n", getpid());
    // Apre il file di configurazione e legge i parametri
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
    send_message(msqid, START_SIM_ATIV_MSG, "inizia a dividere gli atomi", getpid());
    send_message(msqid, START_SIM_ALIM_MSG, "inizia la simulazione", getpid());
    while (*SIM_DURATION > 0)
    {
        printf("------------------------------------------------------------\n");
        printf("[INFO] SIM_DURATION attuale: %d\n", *SIM_DURATION);

        nanosleep((const struct timespec[]){{1, 0}}, NULL); // Ogni secondo

        int energy = stats->energia_prodotta.totale - stats->energia_consumata.totale;

        if (energy < *ENERGY_DEMAND)
        {
            printf("[INFO] Master (PID: %d): Energia attuale: %d\n", getpid(), energy);
            printf("[INFO] Master (PID: %d): Energia richiesta: %d\n", getpid(), *ENERGY_DEMAND);
            printf("[INFO] Master (PID: %d): Energia insufficiente simulazione terminata\n", getpid());
            termination = 1;
            break;
        }

        if ((energy - *ENERGY_DEMAND) > *ENERGY_EXPLODE_THRESHOLD)
        {
            printf("[INFO] Master (PID: %d): Energia attuale: %d\n", getpid(), energy);
            printf("[INFO] Master (PID: %d): Energia soglia di esplosione: %d\n", getpid(), *ENERGY_EXPLODE_THRESHOLD);
            printf("[INFO] Master (PID: %d): Energia superiore alla soglia di esplosione simulazione terminata\n", getpid());
            termination = 2;
            break;
        }

        updateStats(0, 0, 0, *ENERGY_DEMAND, 0);

        printStats();

        (*SIM_DURATION)--;

        if (*SIM_DURATION > 0)
        {
            stats->Nattivazioni.ultimo_secondo = 0;
            stats->Nscissioni.ultimo_secondo = 0;
            stats->energia_prodotta.ultimo_secondo = 0;
            stats->energia_consumata.ultimo_secondo = 0;
            stats->scorie_prodotte.ultimo_secondo = 0;
        }
    }

    cleanup();

    if (termination == 0)
    {
        printf("[TERMINATION] Master (PID: %d): Simulazione terminata per timeout. Chiusura programma.\n", getpid());
    }
    else if (termination == 1)
    {
        printf("[TERMINATION] Master (PID: %d): Simulazione terminata a causa di blackout . Chiusura programma.\n", getpid());
    }
    else if (termination == 2)
    {
        printf("[TERMINATION] Master (PID: %d): Simulazione terminata a causa di un explode . Chiusura programma.\n", getpid());
    }
    exit(EXIT_SUCCESS);
}
