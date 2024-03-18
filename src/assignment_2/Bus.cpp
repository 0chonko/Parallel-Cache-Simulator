#include <iostream>
#include <systemc.h>

#include "bus_slave_if.h"
#include "bus_master_if.h"
#include "helpers.h"
#include "Bus.h"
#include "psa.h"



void Bus::request(uint64_t addr, bool isWrite, int id) {
    // Create a new request
    BusRequest request;
    request.addr = addr;
    request.id = id;
    request.isWrite = isWrite;
    // Queue the request
    pushRequest(request);
    current_timestamp = sc_time_stamp();
    current_addr = addr;

}

void Bus::execute() {
    while (true) {
        wait(); // Wait for the next positive edge of the clock
        log(name(), "executing ", requestQueue.size() );

        if (responseQueue.size() > 0) {
            // If there are responses queued
            BusRequest nextResponse = responseQueue.front();
            ret_memory_response(nextResponse.addr, nextResponse.id);
            // continue;
        }

       

        if (!busy() && !requestQueue.empty()) {
            // If the bus is not busy and there are requests queued
            state = OCCUPIED;

            // Get the next request from the queue
            BusRequest nextRequest = getNextRequest();

            // Process the request
            processRequest(nextRequest);

            // Once the request is processed, the bus is no longer occupied
            state = IDLE;
            continue;
        }
    }
}


void Bus::processRequest(BusRequest request) {
    // Log for debugging
    log(name(), "processing request", request.addr);

    // If it's a write request, forward it to the memory
    if (request.isWrite) {
        memory->write(request.addr, request.id);
    } else {
        // It's a read request, forward it to the memory and possibly save the data
        memory->read(request.addr, request.id);
    }

    // Log for report
    if (current_addr == request.addr) {
        sc_time acquisition_time = sc_time_stamp() - current_timestamp;
        avg_acquisition_time.push_back(acquisition_time);
    }

}

bool Bus::busy() {
    bool isOccupied = state == OCCUPIED ? true : false;
    return isOccupied;
}

Bus::BusRequest Bus::getNextRequest() {
    if (requestQueue.empty()) {
        SC_REPORT_ERROR("/Bus/getNextRequest", "Request queue is empty.");
    }

    BusRequest request = requestQueue.front();
    requestQueue.pop();
    return request;
}

void Bus::pushRequest(BusRequest request) {
    requestQueue.push(request);

}
int Bus::snoop(uint64_t addr, int src_cache, bool isWrite) {
        // cout << "Bus snoop request " << endl;
        int response_cnt = 0;
        for (uint32_t i = 0; i < num_cpus; i++) {
            if (i != (uint32_t)src_cache) { // snoop to all other than me
            response_cnt += caches[i]->read_snoop(addr, isWrite);
            }
        }

        if ((uint32_t)response_cnt < num_cpus - 1) {  // no less than num_cpus - 1 responses
            cout << "Snoop response wrong: " << response_cnt << endl;
            exit(1);
        } else {
            return 0;
        }
        return 0;
}

void Bus::ret_memory_response(uint64_t addr, int id) {
    responseQueue.pop();
}



// Responses from memory
int Bus::read(uint64_t addr, int id) {
    return 0;
}

// Implementation of the write method (required by bus_slave_if)
int Bus::write(uint64_t addr, int id) {
    BusRequest request;
    request.addr = addr;
    request.id = id;
    request.isWrite = true;
    responseQueue.push(request);
    caches[id]->memory_write_event.notify(SC_ZERO_TIME);
    log(name(), "Memory returned to bus", addr);
    return 0;
}

