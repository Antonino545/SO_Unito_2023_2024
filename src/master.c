#include "lib.h"
#include <errno.h>

/**
 * @file master.c
 * @brief Programma principale per la simulazione di atomi e processi correlati.
 * @author Antonino Incorvaia e Desirée Gaudio
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


Statistiche *statistiche; // Statistiche della simulazione
int msqid; /** ID della coda di messaggi */
 
 /**
  * Funzione che initilizza le statistiche della simulazione.
  */
void initStats() {
    statistiche = malloc(sizeof(*statistiche));
    if (statistiche == NULL) {
        perror("Failed to allocate memory for statistiche");
        exit(EXIT_FAILURE);
    }
    memset(statistiche, 0, sizeof(Statistiche));// Inizializza tutte le variabili di statistiche a 0 

}
/**
 * Stampa le statistiche della simulazione.
 */
void printStats(){
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

}


/**
 * Funzione di pulizia che gestisce la terminazione dei processi e la rimozione delle risorse.
 */
void cleanup() {
        printf("[CLEANUP] Master (PID: %d): Avvio della pulizia\n", getpid());
        killpg(*ATOMO_GPID, SIGINT); // Invia il segnale di terminazione a tutti i processi figli
        // Attende la terminazione di tutti i processi figli
        while (wait(NULL) != -1);
        const char  *shm_name="/Parametres";
        // Pulizia della memoria condivisa
        printf("[CLEANUP] Master (PID: %d): Inizio pulizia della memoria condivisa\n", getpid());
        if (shm_unlink(shm_name) == -1) {
            perror("[ERROR] Master: Errore durante la pulizia della memoria condivisa (shm_unlink fallita)");
        } else {
            printf("[CLEANUP] Master (PID: %d): Memoria condivisa pulita con successo\n", getpid());
        }

        // Rimuove la coda di messaggi
        if (msgctl(msqid, IPC_RMID, NULL) < 0) {
            perror("[ERROR] Master: Errore durante la rimozione della coda di messaggi (msgctl fallita)");
        }
    
}

/**
 * Questa funzione viene lanciata quando il processo riceve un segnale di meltdown.
 * @param sig Il segnale ricevuto.
 */
void handle_meltdown(int sig) {
    cleanup();
    printf("[TERMINATION] Master (PID: %d): Simulazione terminata a causa di un meltdown. Chiusura programma.\n", getpid());
    exit(EXIT_SUCCESS);
}
/**
 * Questa funzione viene lanciata quando il processo riceve un segnale di interruzione.
 * @param sig Il segnale ricevuto.
 */
void handle_interruption(int sig){
    cleanup();
    printf("[TERMINATION] Master (PID: %d): Simulazione terminata a causa della ricezione di un segnale di interruzione. Chiusura programma.\n", getpid());
    exit(EXIT_SUCCESS);
}
void handle_sigusr2(int signum) {
    // Custom logic for handling SIGUSR2
    printf("Received SIGUSR2 signal\n");
}
/**
 * Questa funzione gestisce come il processo deve comportarsi quando riceve un segnale di meltdown.
 */
void setup_signal_handler() {
    struct sigaction sa;// Struttura per la gestione dei segnali
    bzero (& sa , sizeof ( sa ) ) ;
    sa.sa_handler = handle_meltdown;// Imposta la funzione di gestione del segnale;
    sigemptyset(&sa.sa_mask);  // Inizializza il set dei segnali bloccati durante l'esecuzione della funzione di gestione
    sigaction(SIGUSR1, &sa, NULL);// Imposta la gestione del segnale SIGUSR1
    struct sigaction interrupt_sa;
    interrupt_sa.sa_handler = handle_interruption;
    sigemptyset(&interrupt_sa.sa_mask);
    sigaction(SIGINT, &interrupt_sa, NULL);
    struct sigaction sa2;
    sa2.sa_handler = handle_sigusr2;
    sigemptyset(&sa2.sa_mask);
    sigaction(SIGUSR2, &sa2, NULL);
} 


/*
 * Crea un nuovo processo figlio per eseguire il programma `atomo` con un numero atomico casuale.
 */
void createAtomo() {
    int numero_atomico = generate_random(*N_ATOM_MAX);
    char num_atomico_str[20];
    snprintf(num_atomico_str, sizeof(num_atomico_str), "%d", numero_atomico);
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("[ERROR] Master: Fork fallita durante la creazione di un atomo");
        kill(*PID_MASTER, SIGUSR1);
    } else if (pid == 0) {
        printf("[INFO] Atomo (PID: %d): Avvio processo atomo con numero atomico %d\n", getpid(), numero_atomico);
        if (execlp("./atomo", "atomo", num_atomico_str, NULL) == -1) {
            perror("[ERROR] Atomo: execlp fallito durante l'esecuzione del processo atomo");
            exit(EXIT_FAILURE);
        }
    } else {
        if (*ATOMO_GPID == -1) {
            *ATOMO_GPID = pid; // Imposta il GPID del primo atomo
            printf("[INFO] Master (PID: %d): GPID del primo atomo impostato a %d\n", getpid(), *ATOMO_GPID);
        }
        if (setpgid(pid, *ATOMO_GPID) == -1) {
            perror("[ERROR] Master: Impossibile impostare il gruppo di processi del figlio");
        } else {
            printf("[INFO] Master (PID: %d): Processo atomo con PID: %d, gruppo impostato a %d\n", getpid(), pid, *ATOMO_GPID);
        }
    }
}
/**
 * Crea un nuovo processo figlio per eseguire il programma `attivatore`.
 */
void createAttivatore() {
    pid_t pid = fork(); // Crea un nuovo processo

    if (pid < 0) { // Errore nella creazione del processo
        perror("[ERROR] Master: Fork fallita durante la creazione di un attivatore");
        kill(*PID_MASTER, SIGUSR1);

    } else if (pid == 0) { // Processo figlio
        printf("[INFO] Attivatore (PID: %d): Avvio processo attivatore\n", getpid());

        // Esegue `attivatore`
        if (execlp("./attivatore", "attivatore", NULL) == -1) {
            perror("[ERROR] Attivatore: execlp fallito durante l'esecuzione del processo attivatore");
            exit(EXIT_FAILURE);
        }
    } else { // Processo padre
        printf("[INFO] Master (PID: %d): Processo attivatore creato con PID: %d\n", getpid(), pid);
    }
}

/**
 * Crea un nuovo processo figlio per eseguire il programma `alimentazione`.
 */
void createAlimentazione() {
    pid_t pid = fork(); // Crea un nuovo processo

    if (pid < 0) { // Errore nella creazione del processo
        perror("[ERROR] Master: Fork fallita durante la creazione di un Alimentazione");
        kill(*PID_MASTER, SIGUSR1);

    } else if (pid == 0) { // Processo figlio
        printf("[INFO] Alimentazione (PID: %d): Avvio processo alimentazione\n", getpid());

        // Esegue `alimentazione`
        if (execlp("./alimentazione", "alimentazione", NULL) == -1) {
            perror("[ERROR] Alimentazione: execlp fallito durante l'esecuzione del processo alimentazione");
            exit(EXIT_FAILURE);
        }
    } else { // Processo padre
        printf("[INFO] Master (PID: %d): Processo alimentazione creato con PID: %d\n", getpid(), pid);
    }
}

/**
 * Legge i parametri dal file di configurazione e li memorizza nella memoria condivisa.
 * @param file Puntatore al file di configurazione aperto.
 */
void readparameters(FILE *file) {
    if (file == NULL) { // Verifica che il file sia stato aperto correttamente
        perror("[ERROR] Master: Impossibile aprire il file di configurazione");
        exit(EXIT_FAILURE);
    }
    *PID_MASTER=getpid();
    printf("[DEBUG] Master: PID impostato a %d\n", getpid());
    char line[256]; // Buffer per leggere ogni linea del file
    while (fgets(line, sizeof(line), file)) { // Legge il file riga per riga
        line[strcspn(line, "\r\n")] = 0; // Rimuove il carattere di newline

        char key[128];
        int value;

        // Parsea la linea nel formato chiave=valore
        if (sscanf(line, "%127[^=]=%d", key, &value) == 2) {
            if (strcmp(key, "N_ATOMI_INIT") == 0) {
                *N_ATOMI_INIT = value;
                printf("[DEBUG] Master: N_ATOMI_INIT impostato a %d\n", value);
            } else if (strcmp(key, "N_ATOM_MAX") == 0) {
                *N_ATOM_MAX = value;
                printf("[DEBUG] Master: N_ATOM_MAX impostato a %d\n", value);
            } else if (strcmp(key, "MIN_N_ATOMICO") == 0) {
                *MIN_N_ATOMICO = value;
                printf("[DEBUG] Master: MIN_N_ATOMICO impostato a %d\n", value);
            } else if (strcmp(key, "ENERGY_DEMAND") == 0) {
                *ENERGY_DEMAND = value;
                printf("[DEBUG] Master: ENERGY_DEMAND impostato a %d\n", value);
            } else if (strcmp(key, "STEP") == 0) {
                *STEP = value;
                printf("[DEBUG] Master: STEP impostato a %d\n", value);
            } else if (strcmp(key, "N_NUOVI_ATOMI") == 0) {
                *N_NUOVI_ATOMI = value;
                printf("[DEBUG] Master: N_NUOVI_ATOMI impostato a %d\n", value);
            } else if (strcmp(key, "SIM_DURATION") == 0) {
                *SIM_DURATION = value;
                printf("[DEBUG] Master: SIM_DURATION impostato a %d\n", value);
            } else if (strcmp(key, "ENERGY_EXPLODE_THRESHOLD") == 0) {
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
int main() {
    printf("[INFO] Master (PID: %d): Inizio esecuzione del programma principale\n", getpid());

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
    PID_MASTER=(int *)(shmParamsPtr + 8 * sizeof(int));
    ATOMO_GPID=(pid_t *)(shmParamsPtr + 9 * sizeof(int));
        *ATOMO_GPID=-1;

    printf("[INFO] Master (PID: %d): Memoria condivisa mappata con successo. Inizio lettura del file di configurazione\n", getpid());

    // Apri il file di configurazione e leggi i parametri
    FILE *file = fopen("../Data/parameters.txt", "r");
    if (file == NULL) {
        perror("[ERROR] Master: Impossibile aprire il file di configurazione");
        cleanup();
        exit(EXIT_FAILURE);
    } else {
        readparameters(file);
    }

    // Stampa informazioni di inizio simulazione
    printf("[INFO] Master (PID: %d): Parametri letti dal file di configurazione. Inizio creazione Proccessi iniziali\n", getpid());

    // Creazione della coda di messaggi
    key_t key = MESSAGE_QUEUE_KEY;
    if ((msqid = msgget(key, IPC_CREAT | MES_PERM_RW_ALL)) < 0) {
        perror("[ERROR] Master: Errore durante la creazione della coda di messaggi (msgget fallita)");
        exit(EXIT_FAILURE);
    }
    printf("[INFO] Master (PID: %d): Coda di messaggi creata con successo\n", getpid());
    printf("[INFO] Master (PID: %d): Inizializzo la gestione del segnale di meltdown\n", getpid());
    setup_signal_handler(); // Imposta il gestore del segnale per il meltdown

    printf("[INFO] Master (PID: %d): Inizio creazione atomi iniziali\n", getpid());
    for (int i = 0; i < *N_ATOMI_INIT; i++) {
        createAtomo();
    }

    // Ricezione dei messaggi dai processi
    waitForNInitMsg(msqid, *N_ATOMI_INIT);

    printf("[INFO] Master (PID: %d): Fine creazione atomi iniziali\n", getpid());
    // Creazione dei processi necessari
    printf("[INFO] Master (PID: %d): Creazione del processo alimentatore\n", getpid());
    createAlimentazione();
    waitForNInitMsg(msqid, 1);
    printf("[INFO] Master (PID: %d): Creazione del processo attivatore\n", getpid());
    createAttivatore();
    waitForNInitMsg(msqid, 1);
    
    initStats(); // Inizializza le statistiche della simulazione
    // Avvio della simulazione principale
    printf("[IMPORTANT] Master (PID: %d): Processi creati con successo. Inizio simulazione principale\n", getpid());
    int termination =0;
    while (*SIM_DURATION > 0) {
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
        struct timespec my_time ;
        my_time.tv_sec = 1;
        my_time.tv_nsec = 0;
        nanosleep(&my_time, NULL);//Uso nanosleep per aspettare un secondo invece di sleep per evitare che il processo venga interrotto da un segnale
        printStats();
    }
    cleanup();
    if(termination==0){
        printf("[TERMINATION] Master (PID: %d): Simulazione terminata con successo. Chiusura programma.\n", getpid());
    }
    else if(termination==1){
        printf("[TERMINATION] Master (PID: %d): Simulazione terminata a causa di blackout . Chiusura programma.\n", getpid());
    }
    exit(EXIT_SUCCESS);
}
