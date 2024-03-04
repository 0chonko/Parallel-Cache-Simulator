#include "Bus.h"
#include <iostream>
#include <systemc.h>

#include "bus_slave_if.h"
#include "bus_master_if.h"
#include "helpers.h"
#include "types.h"

// Constructor
Bus::Bus(sc_module_name nm) : sc_module(nm) {
    // You may register processes here and initialize ports, if necessary
}

// Incoming request from cache, spread snoop and forward to memory 
void Bus::cache_request() {
    // Implementation...
}

// Incoming request from memory
void Bus::memory_request() {
    // Implementation...
}

// Determine the next request to prioritize
void Bus::select_next_priority() {
    // Implementation...
}

// Implementation of the read method (required by bus_slave_if)
int Bus::read(uint64_t addr) {
    // Implementation, possibly interacting with memory through the memory port
}

// Implementation of the write method (required by bus_slave_if)
int Bus::write(uint64_t addr) {
    // Implementation, possibly interacting with memory through the memory port
    return 0;
}

// Implementation of the read_to_bus method (required by bus_master_if)
int Bus::read_to_bus(uint64_t addr) {
    // Implementation...
    log(name(), "bus address", addr);
    memory->read(addr);
    return 0;
}

// Implementation of the write_to_bus method (required by bus_master_if)
int Bus::write_to_bus(uint64_t addr) {
    // Implementation...
    memory->write(addr);
    return 0;
}


// ###################################################
// ###################################################
// ###################################################

// #ifndef MEMORY_H
// #define MEMORY_H

// #include <iostream>
// #include <systemc.h>

// #include "bus_slave_if.h"
// #include "bus_master_if.h"
// #include "helpers.h"
// #include "types.h"

// class Bus : public bus_slave_if, public bus_master_if, public sc_module {
//     // Input ports for requests from caches and memory
//     // sc_in<bool> cache_request;
//     // sc_in<bool> memory_request;

//     // // Output ports for selected request and snooping messages
//     // sc_out<bool> selected_request;
//     // sc_out<bool> snooping_message;

//     // // Internal variables for request arbitration
//     // bool cache_request_pending;
//     // bool memory_request_pending;
//     // sc_out<snoop_message> broadcast_channel; // Output for broadcasting messages to all caches
//     sc_port<bus_slave_if> memory; //bus to memory
//     sc_port<bus_master_if> cache; //TODO: hooke it - bus to memory
//     sc_in_clk clock;
//     // simple_bus_request_vec m_requests;
//     // simple_bus_request *m_current_request;

//     // Constructor
//     SC_CTOR(Bus) {
//         // snooping_channel.initialize(false);

//         // // Register the process for bus operation
//         // SC_METHOD(handle_request);
//         // sensitive << clock.pos(); // Trigger on rising edge

//         // SC_METHOD(handle_return);
//         // sensitive << clock.pos(); // Trigger on rising edge

//         // SC_METHOD(select_next_priority);
//         // sensitive << clock.neg(); // Trigger on falling edge
//     }

//         // Bus operation process
//     void cache_request() {
//         // Check if there is a pending cache request
//         // if (m_requests.size() > 0) {
//         //     // Return busy
//         //     return;
//         // }
//     } // incoming request from cache, spread snoop and forward to memory 

//     void memory_request() {
//         // Check if there is a pending memory request
//         // if (m_requests.size() > 0) {
//         //     // Return busy
//         //     return;
//         // }
    
//     } // incoming request from mem

//     void select_next_priority() {
//         // the slave is done with its action, m_current_request is
//         // empty, so go over the bag of request-forms and compose
//         // a set of likely requests. Pass it to the arbiter for the
//         // final selection
//         // simple_bus_request_vec Q;
//         // for (unsigned int i = 0; i < m_requests.size(); ++i)
//         // {
//         //     simple_bus_request *request = m_requests[i];
//         //     if ((request->status == SIMPLE_BUS_REQUEST) ||
//         //         (request->status == SIMPLE_BUS_WAIT))
//         //     {
//         //         if (m_verbose) 
//         //             sb_fprintf(stdout, "%g %s : request (%d) [%s]\n",
//         //                     sc_time_stamp().to_double(), name(), 
//         //                     request->priority, simple_bus_status_str[request->status]);
//         //         Q.push_back(request);
//         //     }
//         // }
//         // if (Q.size() > 0)
//         //     m_current_request = arbiter_port->arbitrate(Q);
//     }

//     // Read request (bus_master_if)
//     int read_to_bus(uint64_t addr) {
//         memory->read(addr);
//         return 0;
//     }

//     // Write request (bus_master_if)
//     int write_to_bus(uint64_t addr) {
//         memory->write(addr);
//         return 0;
//     }    
// };

// #endif


    // Bus operation process
    // void busOperation() {
    //     // Check if there is a cache request
    //     if (cache_request.read()) {
    //         // Set cache request pending
    //         cache_request_pending = true;
    //     }

    //     // Check if there is a memory request
    //     if (memory_request.read()) {
    //         // Set memory request pending
    //         memory_request_pending = true;
    //     }

    //     // Check if there is a pending cache request and no pending memory request
    //     if (cache_request_pending && !memory_request_pending) {
    //         // Select cache request
    //         selected_request.write(true);
    //         snooping_message.write(false);

    //         // Reset cache request pending
    //         cache_request_pending = false;
    //     }

    //     // Check if there is a pending memory request and no pending cache request
    //     if (memory_request_pending && !cache_request_pending) {
    //         // Select memory request
    //         selected_request.write(true);
    //         snooping_message.write(false);

    //         // Reset memory request pending
    //         memory_request_pending = false;
    //     }

    //     // Check if there are both cache and memory requests pending
    //     if (cache_request_pending && memory_request_pending) {
    //         // Select cache request and broadcast snooping message
    //         selected_request.write(true);
    //         snooping_message.write(true);

    //         // Reset cache and memory request pending
    //         cache_request_pending = false;
    //         memory_request_pending = false;
    //     }

    //     // No request selected
    //     if (!cache_request_pending && !memory_request_pending) {
    //         selected_request.write(false);
    //         snooping_message.write(false);
    //     }
    // }

    // // Read request (bus_slave_if)
    // virtual int read(uint64_t addr) {
    //     // Check if there is a pending cache request
    //     if (cache_request_pending) {
    //         // Return busy
    //         return BUSY;
    //     }

    //     // Return OK
    //     return OK;
    // }
    
    // // Write request (bus_slave_if)
    // virtual int write(uint64_t addr) {
    //     // Check if there is a pending cache request
    //     if (cache_request_pending) {
    //         // Return busy
    //         return BUSY;
    //     }

    //     // Return OK
    //     return OK;
    // }

// ###################################################
// ###################################################
// ###################################################

// typedef std::vector<simple_bus_request *> simple_bus_request_vec;
// simple_bus_request_vec m_requests;
// simple_bus_request *m_current_request;


// simple_bus_request * simple_bus::get_request(unsigned int priority)
// {
//   simple_bus_request *request = (simple_bus_request *)0;
//   for (unsigned int i = 0; i < m_requests.size(); ++i)
//     {
//       request = m_requests[i];
//       if ((request) &&
// 	  (request->priority == priority))
// 	return request;
//     }
//   request = new simple_bus_request;
//   request->priority = priority;
//   m_requests.push_back(request);
//   return request;		
// }

// simple_bus_request * simple_bus::get_next_request()
// {
//   // the slave is done with its action, m_current_request is
//   // empty, so go over the bag of request-forms and compose
//   // a set of likely requests. Pass it to the arbiter for the
//   // final selection
//   simple_bus_request_vec Q;
//   for (unsigned int i = 0; i < m_requests.size(); ++i)
//     {
//       simple_bus_request *request = m_requests[i];
//       if ((request->status == SIMPLE_BUS_REQUEST) ||
// 	  (request->status == SIMPLE_BUS_WAIT))
// 	{
// 	  if (m_verbose) 
// 	    sb_fprintf(stdout, "%g %s : request (%d) [%s]\n",
// 		       sc_time_stamp().to_double(), name(), 
// 		       request->priority, simple_bus_status_str[request->status]);
// 	  Q.push_back(request);
// 	}
//     }
//   if (Q.size() > 0)
//     return arbiter_port->arbitrate(Q);
//   return (simple_bus_request *)0;
// }