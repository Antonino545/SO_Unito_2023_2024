#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
// Variabili per le configurazioni
int n_atomi_init, n_atom_max, min_n_atomico, energy_demand, n_nuovi_atomi, sim_duration, energy_explode_threshold;
long step;

void readParamether()
{
    FILE *file = fopen("../Data/parameters.txt", "r");
    if (file == NULL) {
        perror("Errore nell'apertura del file di configurazione");
        exit(EXIT_FAILURE);
    }

    char chiave[50];
    int valore;
    while (fscanf(file, "%49[^=]=%d\n", chiave, &valore) == 2) {
        if (strcmp(chiave, "N_ATOMI_INIT") == 0) {
            n_atomi_init = valore;
        } else if (strcmp(chiave, "N_ATOM_MAX") == 0) {
            n_atom_max = valore;
        } else if (strcmp(chiave, "MIN_N_ATOMICO") == 0) {
            min_n_atomico = valore;
        } else if (strcmp(chiave, "ENERGY_DEMAND") == 0) {
            energy_demand = valore;
        } else if (strcmp(chiave, "STEP") == 0) {
            step = valore;
        } else if (strcmp(chiave, "N_NUOVI_ATOMI") == 0) {
            n_nuovi_atomi = valore;
        } else if (strcmp(chiave, "SIM_DURATION") == 0) {
            sim_duration = valore;
        } else if (strcmp(chiave, "ENERGY_EXPLODE_THRESHOLD") == 0) {
            energy_explode_threshold = valore;
        }
    }

    fclose(file);

}

int main() {
    readParamether();
    printf("N_ATOMI_INIT=%d\n", n_atomi_init);
    printf("N_ATOM_MAX=%d\n", n_atom_max);
    printf("MIN_N_ATOMICO=%d\n", min_n_atomico);
    printf("ENERGY_DEMAND=%d\n", energy_demand);
    printf("STEP=%ld\n", step);
    printf("N_NUOVI_ATOMI=%d\n", n_nuovi_atomi);
    printf("SIM_DURATION=%d\n", sim_duration);
    printf("ENERGY_EXPLODE_THRESHOLD=%d\n", energy_explode_threshold);
        exit(EXIT_FAILURE);
}
