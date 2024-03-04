#ifndef __types_h
#define __types_h

#include <stdio.h>


struct snoop_message {
    bool read_write[8]; // Array of bool for read/write
    uint64_t addr; // uint96_t address
    sc_module_name module_name; // sc_module_name
};

#endif
