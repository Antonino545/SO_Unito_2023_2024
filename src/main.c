#include <stdio.h>

#define SIZE 10

int main() {
    int arr[SIZE];  // Fixed-size array

    for (int i = 0; i < SIZE; i++) {
        arr[i] = i * i;
    }

    printf("Array elements are:\n");
    for (int i = 0; i < SIZE; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    return 0;
}
