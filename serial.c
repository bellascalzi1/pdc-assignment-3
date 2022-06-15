#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int *generateRandomArray(size_t size) {
    int *arr = (int *)malloc(sizeof(int) * size);

    for(size_t i=0; i<size; i++) {
        // Make verifying results easier for me :( max array size is probs gonna be 1mil
        arr[i] = rand() % 1000000;
    }

    return arr;
}

void printArray(int *arr, size_t size) {
    printf("Printing Array...\n");
    for(size_t i=0; i<size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

void checkResult(int *arr, size_t size) {
    for(size_t i=0; i<size-1; i++) {
        if(arr[i] > arr[i+1]) {
            printf("ERROR %d is greater than %d\n", arr[i], arr[i+1]);
        }
    }
}

int main(int argc, char *argv[]) {

    if(argc != 2) {
        printf("Give me the array size, you idiot!\n");
        exit(EXIT_FAILURE);
    }

    const size_t arraySize = (size_t) atoi(argv[1]);

    // printf("Serial, array size %ld\n", arraySize);

    // Seed the RNG with the current time
    srand(time(NULL));

    int *arrPtr = generateRandomArray(arraySize);

    // printf("Array before sort is:\n");
    // printArray(arrPtr, arraySize);

    clock_t start, end;
    start = clock();

    // Array is guaranteed to be sorted in no more than arraySize steps
    for(size_t i=0; i<arraySize; i++) {
        
        // Even Step
        for(size_t j=0; j<arraySize; j += 2) {
            if(arrPtr[j+1] < arrPtr[j]) {
                // Swap
                int temp = arrPtr[j + 1];
                arrPtr[j + 1] = arrPtr[j];
                arrPtr[j] = temp;
            }
        }

        // Odd Step
        for(size_t k=1; k<arraySize; k += 2) {
            if(arrPtr[k+1] < arrPtr[k]) {
                // Swap
                int temp = arrPtr[k + 1];
                arrPtr[k + 1] = arrPtr[k];
                arrPtr[k] = temp;
            }
        }
    }

    end = clock();

    // Get the CPU time in seconds, then convert to milliseconds because this is what the parallel algorithm uses
    double cpuTimeInSeconds = ((double)(end - start) / CLOCKS_PER_SEC);
    double cpuTimeInMilliSec = cpuTimeInSeconds * 1000;

    printf("%0.3f ",  cpuTimeInMilliSec);

    checkResult(arrPtr, arraySize);

    free(arrPtr);

    return EXIT_SUCCESS;
}