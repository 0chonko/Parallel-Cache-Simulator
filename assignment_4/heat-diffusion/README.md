# MCPS OpenMP assignment
This repo contains the boilerplate code for the OpenMP assignment of the course "Multi-Core Processor Systems".

## Structure:
The structure of this repo is as follows:
0) The *trace_example* directory contains the trace example that is explained in the assignment pdf.
1) the *heat_seq* and *heat_omp* directories contain the framework code for assignment part 1 and part 2.
2) There is a bit of boilerplate code for each assignment and a make file. 
3) The *include* folder contains boilerplate headers for the heat dissipation simulation.
4) The *src* folder contains boilerplate code for the heat dissipation simulation.
5) The *images* folder contains input images for the heat dissipation simulation.
    - In the images folder you can also find a makefile that can generate new images e.g. "make areas_500x500.pgm"
6) You can find reference output for Heat Dissipation in */heat_dissipation_reference_output/*. 
The outputs were generated with the following command: `./heat_seq -n 150 -m 100 -i 42 -e 0.0001 -c ../../images/pat1_100x150.pgm -t ../../images/pat1_100x150.pgm -r 1 -k 10 -L 0 -H 100`
