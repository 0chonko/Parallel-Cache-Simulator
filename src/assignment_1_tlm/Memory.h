#ifndef MEMORY_H
#define MEMORY_H

#include <iostream>
#include <systemc.h>

#include "bus_slave_if.h"

#include "helpers.h"

class Memory : public bus_slave_if, public sc_module {
    public:
    sc_port<bus_slave_if> bus; // Connection to bus
    int access_read=0;
    int access_write=0;

    SC_CTOR(Memory) {
        // nothing to do here right now.
    }

    ~Memory() {
        // print memory accesses
        std::cout << "Memory accesses: " << access_read << " reads, " << access_write << " writes" << std::endl;
    }

    int read(uint64_t addr, int id) {
        assert((addr & 0x3) == 0);
        log(name(), "read from address", addr);
        wait(100);
        bus->write(addr, id);
        access_read++;
        return 0;
    }

    int write(uint64_t addr, int id) {
        assert((addr & 0x3) == 0);
        log(name(), "write to address", addr);
        wait(100);
        bus->write(addr, id);
        access_write++;
        return 0;
    }
};
#endif
