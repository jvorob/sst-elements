#include <sst_config.h>
#include "SimplePageTable.h"

using SST::MemHierarchy::Addr;
using namespace SST;
using namespace SambaComponent;

#include <sst/elements/memHierarchy/memEventBase.h>
#include <sst/elements/memHierarchy/memEvent.h>

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


PageTable::PageTable(SST::ComponentId_t id, SST::Params& params): Component(id) {

    int verbosity = params.find<int>("verbose", 1);
    out = new SST::Output("PageTable[@f:@l:@p>] ", verbosity, 0, SST::Output::STDOUT);
    out->verbose(_L1_, "Creating PageTable\n");



    link_from_os =  configureLink("link_from_os",  new Event::Handler<PageTable>(this, &PageTable::handleMappingEvent));
    link_from_tlb = configureLink("link_from_tlb", new Event::Handler<PageTable>(this, &PageTable::handleTranslationEvent));

    sst_assert(link_from_os,  CALL_INFO, -1, "Error in SimplePageTable: Failed to configure port 'link_from_os'\n");
    sst_assert(link_from_tlb, CALL_INFO, -1, "Error in SimplePageTable: Failed to configure port 'link_from_tlb'\n");
}


// Page mappings requests from OS
void PageTable::handleMappingEvent(SST::Event *ev) {
    auto m_ev  = dynamic_cast<PageTable::MappingEvent*>(ev);

    if (!m_ev) {
        out->fatal(CALL_INFO, -1, "Error! Bad Event Type received\n");
    }


    out->verbose(_L3_, "Got mappingEvent: %s\n", m_ev->getString().c_str());
}

//Incoming MemEvents from TLBs, we need to translate them
void PageTable::handleTranslationEvent(SST::Event *ev) {
    auto mem_ev  = dynamic_cast<MemHierarchy::MemEventBase*>(ev);

    if (!mem_ev) {
        out->fatal(CALL_INFO, -1, "Error! Bad Event Type received");
    }


    out->verbose(_L3_, "Got translation request (MemEventBase): %s\n", mem_ev->getVerboseString().c_str());

}
