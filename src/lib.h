
/**
 * Genera un numero casuale tra 1 e `max`. 
 * @param max Il valore massimo che può essere generato.
 */
int generate_random(int max) {
    srand(time(NULL) ^ getpid());// serve per generare numeri casuali diversi per ogni processo se lanciato in contemporanea
    return rand() % max + 1;
}
