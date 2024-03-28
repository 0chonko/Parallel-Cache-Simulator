#include <sys/time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "compute.h"
#include <immintrin.h> // use with intel

#include <stdint.h> // only for debugging prints

// #include <x86intrin.h> // use with gcc

#define ALIGN 64		// cache line size
#define VEC_LENGTH 4	// 4 x 64 bit doubles in a 256 vector

#define topHalo(offset)			topHalo[offset]
#define bottomHalo(offset)		bottomHalo[offset]
#define newTemp(n,m)            temperatures[((old) * (num_gridpoints_padding)) + ((n) * ((p->M) + (padding))) + (m)] // TODO this does not work!
#define oldTemp(n,m)            temperatures[(((old)^1) * (num_gridpoints_padding)) + ((n) * ((p->M) + (padding))) + (m)] 
#define conductivity(n, m)		conduct_cpy[((n)*(p->M + padding)) + (m)]

static const double DIRECT_NEIGHBOR_WEIGHT = ((sqrt(2)) / (sqrt(2) + 1)) / 4;
static const double DIAGONAL_NEIGHBOR_WEIGHT = (1 / (sqrt(2) + 1)) / 4;

__m256d calculate_temp(__m256d conductivity, __m256d old_temp, __m256d up, __m256d down, __m256d left, __m256d right, __m256d upleft, __m256d upright, __m256d downleft, __m256d downright) {
	/* "Normal" calculation included here for reference. */
	// return  (old_temp * conductivity) + 
    //         (
    //             (1 - conductivity) * 
    //             (
    //                 ((up + down + left + right) * DIRECT_NEIGHBOR_WEIGHT) + 
    //                 ((upleft + upright + downleft + downright) * DIAGONAL_NEIGHBOR_WEIGHT)
    //             )
    //         );
	
	// Vectors of constants
	__m256d DIR_NW = _mm256_set1_pd(DIRECT_NEIGHBOR_WEIGHT);
	__m256d DIA_NW = _mm256_set1_pd(DIAGONAL_NEIGHBOR_WEIGHT);
 	// (1 - conductivity)
	__m256d ones_ = _mm256_set1_pd(1.0);

	// (old_temp * conductivity)
	__m256d temp_1 = _mm256_mul_pd(old_temp, conductivity);

	// ((up + down + left + right) * DIRECT_NEIGHBOR_WEIGHT) - changed order of operation does not matter
	__m256d temp_direct_1 = _mm256_add_pd(up, down);
	__m256d temp_direct_2 = _mm256_add_pd(left, right);
	__m256d temp_direct_ = _mm256_add_pd(temp_direct_1, temp_direct_2);
	__m256d temp_direct = _mm256_mul_pd(temp_direct_, DIR_NW);

	//((upleft + upright + downleft + downright) * DIAGONAL_NEIGHBOR_WEIGHT)
	__m256d temp_diag_1 = _mm256_add_pd(upleft, upright);
	__m256d temp_diag_2 = _mm256_add_pd(downleft, downright);
	__m256d temp_diag_ = _mm256_add_pd(temp_diag_1, temp_diag_2);
	__m256d temp_diag = _mm256_mul_pd(temp_diag_, DIA_NW);
	
	// complete computation
	__m256d out_ = _mm256_add_pd(temp_direct, temp_diag);
	__m256d reciproc_cond_ = _mm256_sub_pd(ones_, conductivity); // if it's always (1 - conductivity), wouldn't it be more efficient to store it like that?
	__m256d out__ = _mm256_mul_pd(reciproc_cond_, out_);

	return _mm256_add_pd(temp_1, out__);
}

void calculate_row(double *newRowTemp, double *conductivity, double *aboveOldRowStart, double *oldRowStart, double *belowOldRowStart, int M, int padding) {
	for (int m = 0; m < M; m += 4) {	// TODO: the number of columns is not necessarily divisible by 4...
		int left = m - 1;
		int right = m + 1;

		// Load vars into vector
		__m256d old_temp_ = _mm256_load_pd(&(oldRowStart[m]));
		__m256d left_;
		__m256d right_;
		__m256d down_ = _mm256_load_pd(&(belowOldRowStart[m]));
		__m256d downleft_;
		__m256d downright_;
		__m256d up_ = _mm256_load_pd(&(aboveOldRowStart[m]));							
		__m256d upleft_;
		__m256d upright_;

		// if this is the first iteration the left neighbors should go to M-1
		if (m == 0) {
			// only the first is different
			left_ = _mm256_set_pd(oldRowStart[2],oldRowStart[1], oldRowStart[0], oldRowStart[M-1]);
			upleft_ = _mm256_set_pd(aboveOldRowStart[2], aboveOldRowStart[1], aboveOldRowStart[0], aboveOldRowStart[M-1]);
			downleft_ = _mm256_set_pd(belowOldRowStart[2], belowOldRowStart[1], belowOldRowStart[0], belowOldRowStart[M-1]);
		}
		else {
			left_ = _mm256_loadu_pd(&(oldRowStart[left]));
			upleft_ = _mm256_loadu_pd(&(aboveOldRowStart[left]));
			downleft_ = _mm256_loadu_pd(&(belowOldRowStart[left]));
		}

		// If its the last iteration the right neighbors should go to 0 and not to the next row
		if (m >= M-4) {
			// create arrays with the right values that is alligned
			double right_array[4] __attribute((aligned(32))); 
			double upright_array[4] __attribute((aligned(32)));
			double downright_array[4] __attribute((aligned(32)));

			// fill the arrays with the correct data
			int index = 0;

			// any data still to the right of m
			for (; index < 3 - padding; index++)
			{
				right_array[index] = oldRowStart[right + index];
				upright_array[index] = aboveOldRowStart[right + index];
				downright_array[index] = belowOldRowStart[right + index];
			}

			// the first point in the column is to the right of the last gridpoint
			right_array[index] = oldRowStart[0];
			upright_array[index] = aboveOldRowStart[0];
			downright_array[index] = belowOldRowStart[0];
			index++;

			// fill up the vector with padding
			for (int i = 0; i < padding; i++, index++)
			{
				right_array[index] = 0;
				upright_array[index] = 0;
				downright_array[index] = 0;
			}

			// load the correct data in to the vector
			right_ = _mm256_load_pd(right_array);
			upright_ = _mm256_load_pd(upright_array);
			downright_ = _mm256_load_pd(downright_array);
		}
		else {
			right_ = _mm256_loadu_pd(&(oldRowStart[right]));
			upright_ = _mm256_loadu_pd(&(aboveOldRowStart[right]));
			downright_ = _mm256_loadu_pd(&(belowOldRowStart[right]));
		}

		__m256d conductivity_ = _mm256_load_pd(&(conductivity[m]));

		// store the result in the new Temperature val
		_mm256_store_pd(&(newRowTemp[m]),
			calculate_temp(conductivity_, old_temp_, up_, down_, left_, right_, upleft_, upright_, downleft_, downright_));
	}
}

void aggregate_report(const struct parameters *p, struct results *r, double *temperatures, int old, int num_gridpoints, double time, int niter) {
    r->time = time;
    r->niter = niter;
    r->tmax = p->io_tmin;   // min and max are initialized to their opposite extreme values
    r->tmin = p->io_tmax;
    r->maxdiff = 0.0;
    double total = 0;
	int padding = 4 - p->M % 4;
	int num_gridpoints_padding = p->N * (p->M + padding);
    for (int n = 0; n < p->N; n++) {
        for (int m = 0; m < p->M; m++) {
            double new_temp = newTemp(n,m);
            double old_temp = oldTemp(n,m);

            double diff = old_temp - new_temp;
            if (diff < 0)
                diff *= -1;
            if (diff > r->maxdiff) {
                r->maxdiff = diff;
            }

            if (new_temp > r->tmax)
                r->tmax = new_temp;
            if (new_temp < r->tmin)
                r->tmin = new_temp;

            total += new_temp;
        }
    }
    r->tavg = total / num_gridpoints;
}

double curr_time() {
    struct timeval time;
    if(gettimeofday(&time, 0) != 0) {
        printf("Could not get time. Exiting...\n");
        exit(1);
    }
    return (time.tv_sec + (time.tv_usec / 1000000.0));
}

void do_compute(const struct parameters* p, struct results *r)
{
    int num_gridpoints = (p->N)*(p->M);
	int padding = 4 - (p->M % 4);
	padding %= 4;
	int num_gridpoints_padding = (p->N)*(p->M + padding);
	int old = 0;    // Flips between 0 and 1, to mark which temperature array is old

	// Memory Allocation
	// Temperature Grid
    double *temperatures = (double*) _mm_malloc(sizeof(double) * 2 * num_gridpoints_padding, ALIGN);
	// copy each row + padding
	for (int n = 0; n < p->N; n++)
	{
		int position_padding = n * (p->M + padding);
		int position = n * p->M;
		memcpy(temperatures + position_padding, p->tinit + position, sizeof(double) * p->M);
		for (int i = 0; i < padding; i++)
		{
			temperatures[position_padding + p->M + i] = 0.0;
		}
	}
	// Top and Bottom Halos
	double *topHalo = (double*) _mm_malloc(sizeof(double) * p->M, ALIGN);
	memcpy(topHalo, p->tinit, sizeof(double) * p->M);
	double *bottomHalo = (double*) _mm_malloc(sizeof(double) * p->M, ALIGN);
	memcpy(bottomHalo, p->tinit + (p->N - 1) * p->M, sizeof(double) * p->M);

	// Conductivity
	double* conduct_cpy = (double*) _mm_malloc(sizeof(double) * num_gridpoints_padding, ALIGN);
	// copy each row + padding
	for (int n = 0; n < p->N; n++)
	{
		int position_padding = n * (p->M + padding);
		int position = n * p->M;
		memcpy(conduct_cpy + position_padding, p->conductivity + position, sizeof(double) * p->M);
		for (int i = 0; i < padding; i++)
		{
			// conductivity padding is 1, so the padding values will always stay zero
			conduct_cpy[position_padding + p->M + i] = 1.0;
		}
	}

	// Main Loop, starting timer
    double start_time = curr_time();
    for (int i = 1; i <= p->maxiter; i++) {
        old ^= 1;

        // Inner rows
        for (int n = 1; n < p->N - 1; n++) {
            calculate_row(&newTemp(n, 0), &conductivity(n,0), &oldTemp(n-1, 0), &oldTemp(n, 0), &oldTemp(n+1, 0), p->M, padding);
        }
        // Top and Bottom Rows for halo cells
        calculate_row(&newTemp(0,0), &conductivity(0,0), &topHalo(0), &oldTemp(0, 0), &oldTemp(1, 0), p->M, padding);
        calculate_row(&newTemp(p->N-1, 0), &conductivity(p->N-1,0), &oldTemp(p->N-2, 0), &oldTemp(p->N-1, 0), &bottomHalo(0), p->M, padding);

		// Aggregation round every p->period iterations
        if ((i == p->maxiter) || (i % p->period == 0)) {
            aggregate_report(p, r, temperatures, old, num_gridpoints, curr_time() - start_time, i);
            if (r->maxdiff < p->threshold) {
                break;
            }
            if (p->printreports && i < p->maxiter - 1) {    // Because the final report_results() call is in code we can't change, but the report still needs to be produced during the last iteration 
                report_results(p, r);
            }
        }
    }

    _mm_free(temperatures);
	_mm_free(topHalo);
	_mm_free(bottomHalo);
	_mm_free(conduct_cpy);
}