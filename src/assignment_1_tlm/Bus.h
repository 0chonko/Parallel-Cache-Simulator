#ifndef BUS_H
#define BUS_H

#include <iostream>
#include <systemc.h>

#include "bus_slave_if.h"
#include "bus_master_if.h"
#include "helpers.h"
#include "types.h"
#include "global_vars.h"


class Bus : public bus_master_if, public bus_slave_if, public sc_module {
public:
    sc_port<bus_slave_if> memory; // Bus to memory
    sc_in_clk clock;
    sc_port<bus_master_if> cache_ports[2];

    // Constructor
    SC_CTOR(Bus);

    // Bus operation process
    void cache_request(); // Incoming request from cache, spread snoop and forward to memory 
    void memory_request(); // Incoming request from memory
    void select_next_priority();

    // Read request (bus_slave_if)
    int read(uint64_t addr) override;

    // Write request (bus_slave_if)
    int write(uint64_t addr) override;

    // Read request (bus_master_if)
    int read_to_bus(uint64_t addr) override;

    // Write request (bus_master_if)
    int write_to_bus(uint64_t addr) override;
};

#endif // BUS_H
/* ach cache controller connected to the bus receives every memory request
* on the bus made by other caches, and makes the proper modification on its local cache line state.
*/

/*he bus can only serve one request at any certain time. If one cache occupies the bus, other
caches have to wait for the current operation to complete before they can utilize the bus. The
cache does not occupy the bus while it waits for the memory to respond. This means that you
will need to implement a split-transaction bus (). The responses from memory should have 
priority on the bus.
*/
/* the bus can receive multiple requests at the same time, so it needs to arbitrate between them.
*/
/* allow the bus to receive requests from the caches and memory on the rising bus edge, and select the highest priority request on the falling clock edge. 
*/
/* broadcast snooping messages to all caches
*/
/* The work of a module such as the Cache, Bus or Memory may consist of multiple separate tasks. 
*In that case you can create multiple SC_THREAD's or SC_METHOD's in a single module, one for each task.  Since SystemC uses cooperative multi-tasking where sc_threads and sc_methods are run sequentially you do not have to worry about race conditions when accessing shared resources.
*/

// private:
//     struct BusRequest
//     {
//         uint64_t addr;
//         bool isWrite;
//         bool isSnooping;
//         int id;
//     };

//     sc_event busRequestEvent;
//     sc_event busResponseEvent;
//     BusRequest currentRequest;
//     bool busOccupied;

// public:
//     SC_HAS_PROCESS(Bus);

//     Bus(sc_module_name name) : sc_module(name), busOccupied(false) {
//         SC_THREAD(busArbitration);
//         sensitive << busRequestEvent;
//         dont_initialize();
//     }

//     void busArbitration() {
//         while (true) {
//             wait(busRequestEvent);

//             // Select the highest priority request
//             // TODO: Implement the bus arbitration logic here

//             // Process the selected request
//             // TODO: Implement the bus request processing logic here

//             // Notify the cache/memory about the response
//             busResponseEvent.notify();
//         }
//     }

//     // Implement the bus_slave_if methods here
//     // ...

// };