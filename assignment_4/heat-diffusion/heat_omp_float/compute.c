#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <float.h>
#include <time.h>

#include "compute.h"
#include "fail.h"
#include "input.h"
#include "annotation.h"
#include "omp.h"

/* You may assume a maximum number of threads to be fixed as a (symbolic) compile time constant. */

#define topHalo(offset)            	((float)p->tinit[offset])
#define bottomHalo(offset)         	((float)p->tinit[(p->N - 1) * p->M + offset])
#define newTemp(n,m)            	((float)temperatures[((old) * (num_gridpoints)) + ((n) * p->M) + (m)])
#define oldTemp(n,m)            	((float)temperatures[(((old)^1) * (num_gridpoints)) + ((n) * p->M) + (m)])
#define conductivity(n,m)       	((float)p->conductivity[((n)*p->M) + (m)])


static const float DIRECT_NEIGHBOR_WEIGHT = ((sqrt(2)) / (sqrt(2) + 1)) / 4;
static const float DIAGONAL_NEIGHBOR_WEIGHT = (1 / (sqrt(2) + 1)) / 4;

float calculate_point(float conductivity, float old_temp, float up, float down, float left, float right, float upleft, float upright, float downleft, float downright) {
    float result = (old_temp * conductivity) + 
                    (
                        (1 - conductivity) * 
                        (
                            ((up + down + left + right) * DIRECT_NEIGHBOR_WEIGHT) + 
                            ((upleft + upright + downleft + downright) * DIAGONAL_NEIGHBOR_WEIGHT)
                        )
                    );
    return  result;
}

void calculate_row(float *newRowTemp, float *conductivity, float *aboveOldRowStart, float *oldRowStart, float *belowOldRowStart, int M) {
    for (int m = 0; m < M; m++) {
        start_roi();
        int left = (m == 0) ? (M-1) : (m-1); 
        int right = (m == M-1) ? (0) : (m+1); 

        newRowTemp[m] = calculate_point(conductivity[m], oldRowStart[m],
                                    aboveOldRowStart[m], belowOldRowStart[m], oldRowStart[left], oldRowStart[right],
                                    aboveOldRowStart[left], aboveOldRowStart[right], belowOldRowStart[left], belowOldRowStart[right]
                                    );
        end_roi();

    }
}

void aggregate_report(const struct parameters *p, struct results *r, float *temperatures, int old, int num_gridpoints, float time, int niter) {
    r->time = time;
    r->niter = niter;
    r->tmax = p->io_tmin;   // min and max are initialized to their opposite extreme values
    r->tmin = p->io_tmax;
    r->maxdiff = 0.0;
    float total = 0;
    // openMP these two loops can be unrolled, since it just loops through our grid
    #pragma omp parallel for shared(temperatures, p) reduction(+: total) collapse(2)
    for (int n = 0; n < p->N; n++) {
        for (int m = 0; m < p->M; m++) {
            float new_temp = newTemp(n,m);
            float old_temp = oldTemp(n,m);
            float diff = old_temp - new_temp;
            if (diff < 0)
                diff *= -1;

            /* Criticals are to avoid race conditions */
            #pragma omp critical
            if (diff > r->maxdiff)
                r->maxdiff = diff;

            #pragma omp critical
            if (new_temp > r->tmax)
                r->tmax = new_temp;

            #pragma omp critical
            if (new_temp < r->tmin)
                r->tmin = new_temp;
            
            total += new_temp;
        }
    }
    r->tavg = total / num_gridpoints;
}

float curr_time() {
    struct timeval time;
    if(gettimeofday(&time, 0) != 0) {
        printf("Could not get time. Exiting...\n");
        exit(1);
    }
    return (time.tv_sec + (time.tv_usec / 1000000.0));
}

void do_compute(const struct parameters* p, struct results *r) {   
    int num_gridpoints = (p->N)*(p->M);
    int old = 0;  // age bit

    float *temperatures = (float*) malloc(sizeof(float) * 2 * num_gridpoints);
    memcpy(temperatures, p->tinit, sizeof(float) * num_gridpoints);

    float start_time = curr_time();

    for (int i = 1; i <= p->maxiter; i++) { // no paralleization of iterations
        old ^= 1;

        #pragma omp parallel shared(temperatures, p)
        {
            #pragma omp for nowait
            for (int n = 1; n < p->N - 1; n++) {
                calculate_row(&newTemp(n, 0), &conductivity(n,0), &oldTemp(n-1, 0), &oldTemp(n, 0), &oldTemp(n+1, 0), p->M);
            }

            calculate_row(&newTemp(0,0), &conductivity(0,0), &topHalo(0), &oldTemp(0, 0), &oldTemp(1, 0), p->M);
            calculate_row(&newTemp(p->N-1, 0), &conductivity(p->N-1,0), &oldTemp(p->N-2, 0), &oldTemp(p->N-1, 0), &bottomHalo(0), p->M);
        } 

        if ((i == p->maxiter) || (i % p->period == 0)) {
            aggregate_report(p, r, temperatures, old, num_gridpoints, curr_time() - start_time, i);
            if (r->maxdiff < p->threshold) {
                break;
            }
            if (p->printreports && i < p->maxiter - 1) {   // Print reports for all iterations except the last one
                report_results(p, r);
            }
        }
    }
}

