#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <numero_atomico>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int numero_atomico = atoi(argv[1]);
    printf("Atomo %d: Ho un numero atomico di %d\n", getpid(), numero_atomico);
    printf("Atomo %d: il mio ppid è %d\n", getpid(), getppid());

    return 0;
}
