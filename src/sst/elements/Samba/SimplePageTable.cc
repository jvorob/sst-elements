#include <sst_config.h>
#include "SimplePageTable.h"

using SST::MemHierarchy::Addr;
using namespace SST;
using namespace SambaComponent;

#define BOTTOM_N_BITS(N) ((~0UL) >> (64-(N)))

#define PAGE_OFFSET_4K(ADDR) (((uint64_t)(ADDR)) & BOTTOM_N_BITS(12))
#define IS_4K_ALIGNED(ADDR) (PAGE_OFFSET_4K(ADDR) == 0)


#include <sst/elements/memHierarchy/util.h>
// VERBOSITIES (using definitions from memHierarchy/util.h
#define _L1_  CALL_INFO,1,0   //INFO  (this one isnt in util.h for some reason)
// #define _L2_  CALL_INFO,2,0   //warnings
// #define _L3_  CALL_INFO,3,0   //external events
// #define _L5_  CALL_INFO,5,0   //internal transitions
// #define _L10_  CALL_INFO,10,0 //everything


PageTable::PageTable() {
    //TODO: can't use this until we're a component?
    //int verbosity = params.find<int>("verbose", 1); 
    int verbosity = 1;
    out = new SST::Output("PageTable[@f:@l:@p] ", verbosity, 0, SST::Output::STDOUT);
    out->verbose(_L1_, "Creating PageTable");
}


void PageTable::handleMappingEvent(SST::Event *ev) {
    auto m_ev  = dynamic_cast<PageTable::MappingEvent*>(ev);
    
    if (!m_ev) {
        out->fatal(CALL_INFO, -1, "Error! Bad Event Type received");
    }

    
    out->verbose(_L3_, "Got mappingEvent: %s\n", m_ev->getString().c_str());
}
