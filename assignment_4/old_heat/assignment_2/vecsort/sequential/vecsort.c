#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <ctype.h>
#include <omp.h>
#include <assert.h>

/* Ordering of the vector */
typedef enum Ordering { ASCENDING, DESCENDING, RANDOM } Order;

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
	if (end - start <= 1) // if run size == 1
		return; //   consider it sorted
	// split the run longer than 1 item into halves
	int middle = (end + start) / 2;
	// recursively sort both runs from array v[] into extra_space[]
	TopDownSplitMerge(v, start, middle,
			  extra_space); // sort the left  run
	TopDownSplitMerge(v, middle, end,
			  extra_space); // sort the right run

	// merge the resulting runs from array extra_space[] into v[]
	TopDownMerge(extra_space, start, middle, end, v);
}

/* Sort vector v of l elements using mergesort */
// void vecsort(int *v, long l){
void vecsort(int **vector_vectors, int *vector_lengths, long length_outer)
{
	int i;
	int **extra_space = (int **)malloc(length_outer * sizeof(int *));
	memcpy(extra_space, vector_vectors,
	       sizeof(int *) * length_outer); // one time copy of the array
	for (i = 0; i < length_outer; i++) {
    	int length_inner = vector_lengths[i];
    	extra_space[i] = (int *)malloc(length_inner * sizeof(int));
    	memcpy(extra_space[i], vector_vectors[i], length_inner * sizeof(int));

		TopDownSplitMerge(
			extra_space[i], 0, vector_lengths[i],
			vector_vectors[i]); // sort data from extra_space into v
	}
	free(extra_space);
}

void print_v(int **vector_vectors, int *vector_lengths, long length_outer)
{
	printf("\n");
	for (long i = 0; i < length_outer; i++) {
		for (int j = 0; j < vector_lengths[i]; j++) {
			if (j != 0 && (j % 10 == 0)) {
				printf("\n");
			}
			printf("%d ", vector_vectors[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}

int main(int argc, char **argv)
{
	int c;
	int seed = 42;
	long length_outer = 1e4;
	int num_threads = 1;
	Order order = ASCENDING;
	int length_inner_min = 100;
	int length_inner_max = 1000;

	int **vector_vectors;
	int *vector_lengths;

	struct timespec before, after;

	/* Read command-line options. */
	while ((c = getopt(argc, argv, "adrgn:x:l:p:s:")) != -1) {
		switch (c) {
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
			length_outer = atol(optarg);
			break;
		case 'n':
			length_inner_min = atoi(optarg);
			break;
		case 'x':
			length_inner_max = atoi(optarg);
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
			if (optopt == 'l' || optopt == 's') {
				fprintf(stderr,
					"Option -%c requires an argument.\n",
					optopt);
			} else if (isprint(optopt)) {
				fprintf(stderr, "Unknown option '-%c'.\n",
					optopt);
			} else {
				fprintf(stderr,
					"Unknown option character '\\x%x'.\n",
					optopt);
			}
			return -1;
		default:
			return -1;
		}
	}

	/* Seed such that we can always reproduce the same random vector */
	srand(seed);

	/* Allocate vector. */
	vector_vectors = (int **)malloc(length_outer * sizeof(int *));
	vector_lengths = (int *)malloc(length_outer * sizeof(int));
	if (vector_vectors == NULL || vector_lengths == NULL) {
		fprintf(stderr, "Malloc failed...\n");
		return -1;
	}

	assert(length_inner_min < length_inner_max);

	/* Determine length of inner vectors and fill them. */
	for (long i = 0; i < length_outer; i++) {
		int length_inner =
			(rand() % (length_inner_max + 1 - length_inner_min)) +
			length_inner_min; //random number inclusive between min and max
		vector_vectors[i] = (int *)malloc(length_inner * sizeof(int));
		vector_lengths[i] = length_inner;

		/* Allocate and fill inner vector. */
		switch (order) {
		case ASCENDING:
			for (long j = 0; j < length_inner; j++) {
				vector_vectors[i][j] = (int)j;
			}
			break;
		case DESCENDING:
			for (long j = 0; j < length_inner; j++) {
				vector_vectors[i][j] = (int)(length_inner - j);
			}
			break;
		case RANDOM:
			for (long j = 0; j < length_inner; j++) {
				vector_vectors[i][j] = rand();
			}
			break;
		}
	}

	if (debug) {
		print_v(vector_vectors, vector_lengths, length_outer);
	}

	clock_gettime(CLOCK_MONOTONIC, &before);

	/* Sort */
	vecsort(vector_vectors, vector_lengths, length_outer);

	clock_gettime(CLOCK_MONOTONIC, &after);
	double time = (double)(after.tv_sec - before.tv_sec) +
		      (double)(after.tv_nsec - before.tv_nsec) / 1e9;

	printf("Vecsort took: % .6e \n", time);

	if (debug) {
		print_v(vector_vectors, vector_lengths, length_outer);
	}

	free(vector_lengths);
	free(vector_vectors);
	return 0;
}
