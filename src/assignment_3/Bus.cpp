#include <iostream>
#include <systemc.h>

#include "bus_slave_if.h"
#include "bus_master_if.h"
#include "helpers.h"
#include "Bus.h"
#include "psa.h"



void Bus::request(uint64_t addr, bool isWrite, int id, bool busWB, bool isHit) { // TODO: MAYBE THE SNOOPING SHOULD BE HERE ALREADY
    bus_mutex.lock();

    // Create a new request
    BusRequest request;
    request.addr = addr;
    request.id = id;
    request.isWrite = isWrite;
    request.busWB = busWB;
    request.isHit = isHit;
    // Queue the request
    pushRequest(request);
    current_timestamp = sc_time_stamp();
    current_addr = addr;
    bus_mutex.unlock();


}

// void Bus::execute() {
//     while (true) {
//         wait(); 
//         // bus_mutex.lock();

//         if (responseQueue.size() > 0) { // priority to memory responses
//             state = OCCUPIED;
//             // If there are responses queued
//             BusRequest nextResponse = responseQueue.front();
//             ret_memory_response(nextResponse.addr, nextResponse.id);
//             wait();
//             state = IDLE;
//             // bus_mutex.unlock();
//             continue;
//         }

//         if (!busy() && !requestQueue.empty()) {
//             // If the bus is not busy and there are requests queued
//             state = OCCUPIED;
//             // Get the next request from the queue
//             BusRequest nextRequest = getNextRequest();
//             // first check if other caches containt this data address, if yes do state_update notify() and return, otherwise proceed with the rest of the code
//             if (!nextRequest.isHit) { 
//                 if (num_cpus > 1) {
//                     for (int i = 0; i < (int)num_cpus; i++)
//                     {
//                         if (i != nextRequest.id) {
//                             //TODO: should get from owned if there is one
//                             if (caches[i]->has_cacheline(nextRequest.addr) != -1) { // if other caches contain the data continue and do not go to memory
//                                 caches[nextRequest.id]->status_update_event.notify();
//                                 // bus_mutex.unlock();
//                                 state = IDLE;

//                                 wait();
//                                 continue;
//                             } else { // else go to memory
//                                 wait();
//                             }
//                             // TODO: if miss it should still read from memory 
//                         }
//                     }
//                 }
//             } else {
//                 //Let all the caches know that you are processing this request (call read_snoop of each cache instance)
//                 if (num_cpus > 1) {
//                     for (int i = 0; i < (int)num_cpus; i++)
//                     {
//                         if (i != nextRequest.id) {
//                             caches[i]->read_snoop(nextRequest.addr, nextRequest.isWrite); 
//                         }
//                     }
//                     if (!nextRequest.isHit) {
//                         wait();
//                         // bus_mutex.unlock();
//                         state = IDLE;
//                         continue; // only write hit continues to memory, read hit is there just for snooping
//                     } 
//                 }
//             }


//             // Process the request
//             processRequest(nextRequest);
//             // Once the request is processed, the bus is no longer occupied
//             state = IDLE;
//             // bus_mutex.unlock();
//             wait();
//             continue;
//         }
//         // bus_mutex.unlock();

//     }
// }

void Bus::execute() {
    while (true) {
        wait();
        // print 3 to cache[setIndex].lines[i].state in caches[0]

        for (int i = 0; i < (int)num_cpus; i++)
        {
            caches[i]->dump_cache();
            cout << "cache dumped " << i << endl;
        }
        // bus_mutex.lock();

        // PROCESS MEMORY REQUEST WITH PRIORITY
        if (responseQueue.size() > 0) { // priority to memory responses
            state = OCCUPIED;
            // If there are responses queued
            BusRequest nextResponse = responseQueue.front();
            ret_memory_response(nextResponse.addr, nextResponse.id);
            wait();
            state = IDLE;
            // bus_mutex.unlock();
            continue;
        }


        // PROCESS CACHE REQUEST
        if (!busy() && !requestQueue.empty()) {
            
            state = OCCUPIED;
            BusRequest nextRequest = getNextRequest();

            // if is write back then just writ to memory
            if (nextRequest.busWB) {
                processRequest(nextRequest);
                state = IDLE;
                wait();
                continue;
            }
            else {
                // probe other caches, if found trigger probe_hit in those caches, and return the data and the state of found cache
                // if found owned set requesting cache to shared, if found shared set requesting cache to shared, exclusive otherwise
                // iterate through all and then notify

                int cache_counts = 0; // count to see if theres owned, I OMMIT THE MODELING OF OWNED-CHECK AS IT IS TRIVIAL FOR THE SIMULATION PURPOSE
                if (num_cpus > 1) {
                    for (int i = 0; i < (int)num_cpus; i++)
                    {
                        if (i != nextRequest.id) {
                            log(name(), "Checkin on cache", i);
                            if (caches[i]->has_cacheline(nextRequest.addr, nextRequest.isWrite) != -1) { // if other caches contain the data continue and do not go to memory
                                // bus_mutex.unlock();
                                cache_counts++;
                            }
                        }
                    }
                    if (cache_counts == 0) { // get from memory
                        log(name(), "### No cache hit, going to memory", nextRequest.addr);
                        processRequest(nextRequest);
                        state = IDLE;
                        wait();
                        continue;
                    }
                    else {
                        log(name(), "### Cache hit, snooping", nextRequest.addr);
                        caches[nextRequest.id]->status_update_event.notify();
                        state = IDLE;
                        wait();
                        continue;
                    }
                }

            }
        }

    }
}



void Bus::processRequest(BusRequest request) {
    // Log for debugging

    // If it's a write request, forward it to the memory
    if (request.isWrite) {
        memory->write(request.addr, request.id);
    }
    else {
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
    }
    else {
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
    caches[id]->memory_write_event.notify();
    log(name(), "Memory returned to bus", addr);
    return 0;
}

