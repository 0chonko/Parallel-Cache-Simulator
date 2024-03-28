#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <omp.h>

#define print_thread_num() printf("%d\n", omp_get_thread_num())
#define TASK_SIZE 750

/* Ordering of the vector */
typedef enum Ordering {ASCENDING, DESCENDING, RANDOM} Order;

int debug = 0;

// Left source half is  v[start:middle-1].
// Right source half is v[middle:end-1].
// Result is            extra_space[start:end-1].
void TopDownMerge(int *v, int start, int middle, int end, int *extra_space)
{
    int i = start;
    int j = middle;

    // While there are elements in the left or right runs...
    for (int k = start; k < end; k++) {
        // If left run head exists and is <= existing right run head.
        if (i < middle && (j >= end || v[i] <= v[j])) {
            extra_space[k] = v[i];
            i++;
        } else {
            extra_space[k] = v[j];
            j++;
        }
    }
}

// Split v[] into 2 runs, sort both runs into extra_space[], merge both runs from extra_space[] to v[]
void TopDownSplitMerge(int *extra_space, int start, int end, int *v)
{
    int size = end - start;
    if (size <= 1)                       // if run size == 1
        return;                                 //   consider it sorted

    // split the run longer than 1 item into halves
    int middle = (end + start) / 2;

    // recursively sort both runs from array v[] into extra_space[]
    #pragma omp task if (size > TASK_SIZE)
    TopDownSplitMerge(v, start,  middle, extra_space);  // sort the left  run
    #pragma omp task if (size > TASK_SIZE)
    TopDownSplitMerge(v, middle,    end, extra_space);  // sort the right run

    #pragma omp taskwait

    // merge the resulting runs from array extra_space[] into v[]
    TopDownMerge(extra_space, start, middle, end, v);
}

/* Sort vector v of l elements using mergesort */
void msort(int *v, long l){
    int *extra_space = malloc(sizeof(int) * l);
    memcpy(extra_space, v, sizeof(int) * l);

    #pragma omp parallel
    #pragma omp single
    TopDownSplitMerge(extra_space, 0, l, v);    // sort data from extra_space into v    
}

void print_v(int *v, long l) {
    printf("\n");
    for(long i = 0; i < l; i++) {
        if(i != 0 && (i % 10 == 0)) {
            printf("\n");
        }
        printf("%d ", v[i]);
    }
    printf("\n");
}

int main(int argc, char **argv) {
    int c;
    int seed = 42;
    long length = 1e4;
    int num_threads = 1;
    Order order = RANDOM;
    int *vector;

    struct timespec before, after;

    /* Read command-line options. */
    while((c = getopt(argc, argv, "adrgp:l:s:")) != -1) {
        switch(c) {
            case 'a':
                order = ASCENDING;
                break;
            case 'd':
                order = DESCENDING;
                break;
            case 'r':
                order = RANDOM;
                break;
            case 'l':
                length = atol(optarg);
                break;
            case 'g':
                debug = 1;
                break;
            case 's':
                seed = atoi(optarg);
                break;
            case 'p':
                num_threads = atoi(optarg);
                break;
            case '?':
                if(optopt == 'l' || optopt == 's') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                }
                else if(isprint(optopt)) {
                    fprintf(stderr, "Unknown option '-%c'.\n", optopt);
                }
                else {
                    fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
                }
                return -1;
            default:
                return -1;
        }
    }

    /* Seed such that we can always reproduce the same random vector */
    srand(seed);

    /* Allocate vector. */
    vector = (int*)malloc(length*sizeof(int));
    if(vector == NULL) {
        fprintf(stderr, "Malloc failed...\n");
        return -1;
    }

    /* Fill vector. */
    switch(order){
        case ASCENDING:
            for(long i = 0; i < length; i++) {
                vector[i] = (int)i;
            }
            break;
        case DESCENDING:
            for(long i = 0; i < length; i++) {
                vector[i] = (int)(length - i);
            }
            break;
        case RANDOM:
            for(long i = 0; i < length; i++) {
                vector[i] = rand();
            }
            break;
    }

    if(debug) {
        print_v(vector, length);
    }

    clock_gettime(CLOCK_MONOTONIC, &before);

    /* Sort */
    msort(vector, length);

    clock_gettime(CLOCK_MONOTONIC, &after);
    double time = (double)(after.tv_sec - before.tv_sec) +
                  (double)(after.tv_nsec - before.tv_nsec) / 1e9;

    printf("Mergesort took: %f seconds \n", time);

    // Check correctness
    for (int i = 0; i < length - 1; i++) {
        if (vector[i] > vector[i+1]) {
            printf("Sort failed! %d %d\n", vector[i], vector[i+1]);
            break;
        }
    }

    if(debug) {
        print_v(vector, length);
    }

    return 0;
}