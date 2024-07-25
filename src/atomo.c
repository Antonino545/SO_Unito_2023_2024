#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int generate_random(int max) {
    srand(time(NULL) ^ getpid());// serve per generare numeri casuali diversi per ogni processo se lanciato in contemporanea
    return rand() % max + 1;
}
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <numero_atomico>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int max = atoi(argv[1]);
    int numero_atomico = generate_random(max);
    printf("Atomo %d: Ho un numero atomico di %d\n", getpid(), numero_atomico);
    printf("Atomo %d: il mio ppid è %d\n", getpid(), getppid());

    return 0;
}
