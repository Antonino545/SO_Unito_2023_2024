
/**
 * Genera un numero casuale tra 1 e `max`. 
 * @param max Il valore massimo che può essere generato.
 */
int generate_random(int max) ;

extern int N_ATOMI_INIT;// Numero di atomi iniziali
extern int N_ATOM_MAX;// Numero antomico massimo
extern int MIN_N_ATOMICO;// Numero atomico minimo
extern int ENERGY_DEMAND;// Energia richiesta per ogni atomo
extern int STEP;// Passo di simulazione
extern int N_NUOVI_ATOMI;// Numero di nuovi atomi creati ad ogni passo
extern int SIM_DURATION;// Durata della simulazione
extern int ENERGY_EXPLODE_THRESHOLD;// Soglia di energia per l'esplosione