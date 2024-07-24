#ifndef COSTANTI_H
#define COSTANTI_H

// Costanti per la simulazione
#define N_ATOMI_INIT 5        // Numero iniziale di processi atomo
#define N_ATOM_MAX 100        // Numero atomico massimo n atom max figlio deve essere uguale a n_atom_max -1 del padre
#define MIN_N_ATOMICO 10     // Numero atomico minimo per scissione quando e minore di questo termina
#define ENERGY_DEMAND 50      // Energia richiesta ogni secondo dal processo master
#define STEP 1000000000       // Intervallo di tempo per la creazione di nuovi atomi (in nanosecondi)
#define N_NUOVI_ATOMI 2       // Numero di nuovi atomi creati dal processo alimentazione
#define SIM_DURATION 10    // Durata della simulazione in secondi
#define ENERGY_EXPLODE_THRESHOLD 30 // soglia per espolosione

#endif // COSTANTI_H
