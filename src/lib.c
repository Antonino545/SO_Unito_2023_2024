#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "lib.h"


int N_ATOMI_INIT;
int N_ATOM_MAX;
int MIN_N_ATOMICO;
int ENERGY_DEMAND;
int STEP;
int N_NUOVI_ATOMI;
int SIM_DURATION;
int ENERGY_EXPLODE_THRESHOLD;

int generate_random(int max) {
    srand(time(NULL) ^ getpid());// serve per generare numeri casuali diversi per ogni processo se lanciato in contemporanea
    return rand() % max + 1;
}
