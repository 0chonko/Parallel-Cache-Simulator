#include <systemc.h>

#ifndef BUS_MASTER_IF_H
#define BUS_MASTER_IF_H

/* NOTE: Although this model does not have a bus, this bus slave interface is
 * implemented by the memory. The Cache uses this bus slave interface to
 * communicate directly with the memory. */
class bus_master_if : public virtual sc_interface {
    public:
    virtual void request(uint64_t addr, bool isWrite, int id, bool isHit) = 0;
    virtual int snoop(uint64_t addr, int src_cache, bool isWrite) = 0;
    virtual bool busy() = 0;
};

#endif
