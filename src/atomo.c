#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "lib.h"


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <numero_atomico>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int numero_atomico = atoi(argv[1]);
    printf("Atomo %d: Sono un atomo con numero atomico %d\n", getpid(), numero_atomico);
    return 0;
}

void atomdivision(){

}
