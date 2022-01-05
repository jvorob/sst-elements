#include <sst_config.h>
#include "SimplePageTable.h"

using namespace SST;
using namespace SambaComponent;

#include <sst/elements/memHierarchy/memEventBase.h>
#include <sst/elements/memHierarchy/memEvent.h>
using SST::MemHierarchy::MemEventBase;
using SST::MemHierarchy::MemEvent;
using SST::MemHierarchy::Addr;


#define BOTTOM_N_BITS(N) ((~0UL) >> (64-(N)))
#define PAGE_OFFSET_4K(ADDR) (((uint64_t)(ADDR)) & BOTTOM_N_BITS(12))
#define IS_4K_ALIGNED(ADDR) (PAGE_OFFSET_4K(ADDR) == 0)

#define KB 1024
#define MB (KB * 1024)
#define GB (MB * 1024)


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



// ========================================================================
//
//                           EVENT HANDLERS
//
// ========================================================================


// Page mappings requests from OS
void PageTable::handleMappingEvent(SST::Event *ev) {
    auto map_ev  = dynamic_cast<PageTable::MappingEvent*>(ev);

    if (!map_ev) {
        out->fatal(CALL_INFO, -1, "Error! Bad Event Type received\n");
    }


    out->verbose(_L3_, "Got mappingEvent: %s\n", map_ev->getString().c_str());


    switch(map_ev->type) {
        case PageTable::MappingEvent::eventType::MAP_PAGE:
            out->verbose(_L3_, "Mapping page! (NOT IMPLEMENTED)\n");
            break;
        case PageTable::MappingEvent::eventType::UNMAP_PAGE:
            out->verbose(_L3_, "Unmapping page! (NOT IMPLEMENTED)\n");
            break;
        default:
            out->verbose(_L1_, "Event type %s not handled (noop)\n", map_ev->getTypeString().c_str());
    }
    out->verbose(_L3_, "Sending back response\n");
    link_from_os->send(ev);
}

//Incoming MemEvents from TLBs, we need to translate them
void PageTable::handleTranslationEvent(SST::Event *ev) {
    auto mem_ev  = dynamic_cast<MemHierarchy::MemEvent*>(ev);
    if (!mem_ev) 
        { out->fatal(CALL_INFO, -1, "Error! Bad Event Type received"); }

    out->verbose(_L3_, "Got translation request (MemEvent): %s\n", mem_ev->getVerboseString().c_str());
    
    // Translate and send back
    MemEvent *translated_mem_ev = translateMemEvent(mem_ev);
    out->verbose(_L5_, "Sending back translated MemEvent to TLB\n");
    delete mem_ev; //delete original, we send back a copy
    link_from_tlb->send(translated_mem_ev);
}




// ========================================================================
//
//            THE GOOD STUFF: (actually translating things)
//
// ========================================================================



MemEvent* PageTable::translateMemEvent(MemEvent *mEv) {
    //Returns translated copy of mEv
    //CALLER MUST DELETE ORIGINAL mEV

    Addr vAddr    = mEv->getVirtualAddress();
    Addr mainAddr = mEv->getAddr();
    Addr baseAddr = mEv->getBaseAddr();

    if(vAddr != mainAddr) {
        out->verbose(_L2_, "Unexpected: MemEvent's vAddr and Addr differ: %s\n", mEv->getVerboseString().c_str() );
    }

    Addr vPage =      mainAddr & ~BOTTOM_N_BITS(12);
    Addr pageOffset = mainAddr &  BOTTOM_N_BITS(12);

    // we'll need this to recreate translated base addr
    int64_t cacheline_offset = mainAddr - baseAddr;


    //======== Do the translation
    Addr pPage = translatePage(vPage);


    //======== Cleanup and return

    //Stitch together result
    Addr pAddr = pPage | pageOffset;
    Addr pBaseAddr = pAddr - cacheline_offset;


    MemEvent *new_mEv = mEv->clone();
    new_mEv->setAddr(pAddr);
    new_mEv->setBaseAddr(pBaseAddr);

    //out->verbose(_L10_, "Orig. MemEvent  : %s\n", mEv->getVerboseString().c_str());
    out->verbose(_L10_, "Transl. MemEvent: %s\n", new_mEv->getVerboseString().c_str());


    return new_mEv;
}


Addr PageTable::translatePage(Addr virtPageAddr) {
    // It's a pure function, yay!
    // On page fault, crash out (TODO: change this?)

    //==== TODO TEMP DEBUGGING:
    // to just check if the plumbing works, lets manually hardcode the fixed-region mapping from simpleTLB
    //
    const Addr fixed_mapping_va_start = 0x0;
    const Addr fixed_mapping_pa_start = 0xF000000;
    const Addr fixed_mapping_len      = 128 * MB;


    //check bounds
    //TODO: send proper page fault here once implmemented
    //TODO: check MemEvent size?
    //
    if(virtPageAddr < fixed_mapping_va_start || virtPageAddr >= fixed_mapping_va_start + fixed_mapping_len) {
        ////Option 1: anything not explicitly mapped is page fault
        //out->fatal(CALL_INFO, -1, "Page fault: virtual addr 0x%lx is outside of mapped range: 0x%lx - 0x%lx\n", virtPageAddr, fixed_mapping_va_start, fixed_mapping_va_start + fixed_mapping_len-1);

        ////Option 2: all other addresses get mapped to themselves (might be useful for e.g. faking multiprocess)
        return virtPageAddr;
    }


    // Add the VA->PA offset. This should work correctly whether
    // PA or VA are > or <, since everything is uint64_t
    Addr pPageAddr = virtPageAddr + (fixed_mapping_pa_start - fixed_mapping_va_start);

    //Addr pPage = virtPageAddr + SST::MemHierarchy::mebi; //offset by 1MiB, 0x10'0000
    //Addr pPage = vPage + 0x400; //offset by 1KB

    assert((pPageAddr & BOTTOM_N_BITS(12)) == 0); //we should have enforced this by the time we get here

    return pPageAddr;
}
