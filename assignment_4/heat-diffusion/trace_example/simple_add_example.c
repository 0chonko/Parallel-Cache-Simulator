#include <stdio.h>
#include <omp.h>
#include "../include/annotation.h"

int main(void) {
    int sum = 0;
    #pragma omp parallel num_threads(2)
    {
        printf("thread: %d\n", omp_get_thread_num());
        #pragma omp for
        for (int i = 0; i < 4; i++) {
            start_roi();
            sum += 1; // race condition!
            end_roi();
        }
    }
    printf("sum: %d\n", sum);
    return 0;
}
