#include <sys/time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "compute.h"

#define topHalo(offset)            p->tinit[offset]
#define bottomHalo(offset)         p->tinit[(p->N - 1) * p->M + offset]
#define newTemp(n,m)            temperatures[((old) * (num_gridpoints)) + ((n) * p->M) + (m)]
#define oldTemp(n,m)            temperatures[(((old)^1) * (num_gridpoints)) + ((n) * p->M) + (m)]
#define conductivity(n,m)       p->conductivity[((n)*p->M) + (m)]

static const double DIRECT_NEIGHBOR_WEIGHT = ((sqrt(2)) / (sqrt(2) + 1)) / 4;
static const double DIAGONAL_NEIGHBOR_WEIGHT = (1 / (sqrt(2) + 1)) / 4;

double calculate_point(double conductivity, double old_temp, double up, double down, double left, double right, double upleft, double upright, double downleft, double downright) {
    return  (old_temp * conductivity) + 
            (
                (1 - conductivity) * 
                (
                    ((up + down + left + right) * DIRECT_NEIGHBOR_WEIGHT) + 
                    ((upleft + upright + downleft + downright) * DIAGONAL_NEIGHBOR_WEIGHT)
                )
            );
}

void calculate_row(double *newRowTemp, const double *conductivity, const double *aboveOldRowStart, const double *oldRowStart, const double *belowOldRowStart, int M) {
    for (int m = 0; m < M; m++) {
        int left = (m == 0) ? (M-1) : (m-1); 
        int right = (m == M-1) ? (0) : (m+1); 
        
        newRowTemp[m] = calculate_point(conductivity[m], oldRowStart[m],
                                    aboveOldRowStart[m], belowOldRowStart[m], oldRowStart[left], oldRowStart[right],
                                    aboveOldRowStart[left], aboveOldRowStart[right], belowOldRowStart[left], belowOldRowStart[right]
                                    );
    }
}

void aggregate_report(const struct parameters *p, struct results *r, double *temperatures, int old, int num_gridpoints, double time, int niter) {
    r->time = time;
    r->niter = niter;
    r->tmax = p->io_tmin;   // min and max are initialized to their opposite extreme values
    r->tmin = p->io_tmax;
    r->maxdiff = 0.0;
    double total = 0;
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
    int old = 0;    // Flips between 0 and 1, to mark which temperature array is old

    // Memory Allocation for temperature grid
    double *temperatures = (double*) malloc(sizeof(double) * 2 * num_gridpoints);
    memcpy(temperatures, p->tinit, sizeof(double) * num_gridpoints);

	// Main Loop, starting timer
    double start_time = curr_time();
    for (int i = 1; i <= p->maxiter; i++) {
        old ^= 1;

        // Inner rows
        for (int n = 1; n < p->N - 1; n++) {
            calculate_row(&newTemp(n, 0), &conductivity(n,0), &oldTemp(n-1, 0), &oldTemp(n, 0), &oldTemp(n+1, 0), p->M);
        }
        // Top and Bottom Rows for halo cells
        calculate_row(&newTemp(0,0), &conductivity(0,0), &topHalo(0), &oldTemp(0, 0), &oldTemp(1, 0), p->M);
        calculate_row(&newTemp(p->N-1, 0), &conductivity(p->N-1,0), &oldTemp(p->N-2, 0), &oldTemp(p->N-1, 0), &bottomHalo(0), p->M);

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

    free(temperatures);
}