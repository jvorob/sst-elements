#include <sst_config.h>
#include <stdexcept>
#include "SimpleMMU.h"

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



template <typename ...Args>
std::string strPrintf(const char *format, Args && ...args)
{
    char buff[512];
    snprintf(buff, sizeof(buff), format, std::forward<Args>(args)...);
    return std::string(buff); //good enough, if we want to autoallocate stuff we can check retval on sprintf

    // // precalculate size
    // auto size = std::snprintf(nullptr, 0, format, std::forward<Args>(args)...);

    // // Sprintf and create string
    // char *buff = (char*) calloc(1, size + 1); //allocate and zero
    // std::snprintf(buff, size, format, std::forward<Args>(args)...);
    // std::string str = std::string(buff);
    // delete buff;

    // return str;
}




SimpleMMU::SimpleMMU(SST::ComponentId_t id, SST::Params& params): Component(id) {

    int verbosity = params.find<int>("verbose", 1);
    out = new SST::Output("SimpleMMU[@f:@l:@p>] ", verbosity, 0, SST::Output::STDOUT);
    out->verbose(_L1_, "Creating SimpleMMU\n");



    link_from_os =  configureLink("link_from_os", 
            new Event::Handler<SimpleMMU>(this, &SimpleMMU::handleMappingEvent));
    sst_assert(link_from_os,  CALL_INFO, -1, "Error in SimpleMMU: Failed to configure port 'link_from_os'\n");


    // == Load ports "link_from_tlb%d": we can have multiple so loop over them
    
    char linkname_sprintf_buff[256];
    int tlb_i = 0;
	for(tlb_i = 0; ; tlb_i++) {  // loop until configureLink() fails
        //Grab link of right-numbered name
		snprintf(linkname_sprintf_buff, 256, "link_from_tlb%d", tlb_i);
        Link *new_link = configureLink(linkname_sprintf_buff, 
                new Event::Handler<SimpleMMU, int>(this, &SimpleMMU::handleTranslationEvent, tlb_i));

        if(new_link == NULL) 
            { break; } // Stop when no more links

        // Store link
        assert(tlb_i == (int)v_link_from_tlb.size()); //"link0" should be going into vec[0] (should always be true)
		v_link_from_tlb.push_back(new_link); 
    }
    //We should have at least 1?
    sst_assert(tlb_i > 0, CALL_INFO, -1, "Error in SimpleMMU: Failed to configure port 'link_from_tlb0'\n");
    out->verbose(_L1_, "Loaded %d ports named 'link_from_tlb*' \n", tlb_i);


    // == Load mapping_ids for the tlb links, check length
    params.find_array<uint64_t>("tlb_mapping_ids", v_tlb_mapping_ids); //appends to end
    int num_ids = v_tlb_mapping_ids.size();
    sst_assert(num_ids != 0, CALL_INFO, -1, 
            "Error in SimpleMMU: Parameter 'tlb_mapping_ids' not specified\n");
    sst_assert(num_ids == tlb_i, CALL_INFO, -1, 
            "Error in SimpleMMU: Parameter 'tlb_mapping_ids' should have 1 id for each link_from_tlb. "
            "Expected %d, got %d\n", tlb_i, num_ids);
}



// ========================================================================
//
//                           EVENT HANDLERS
//
// ========================================================================

void SimpleMMU::init(unsigned int phase) {
	// We need to handle mapping events during the init phase
    // (Before main simulation begins)

    out->verbose(_L3_, "init(%d) called\n",phase);

    SST::Event * ev;
    while ((ev = link_from_os->recvInitData())) { //incoming from CPU, forward down
        auto map_ev  = dynamic_cast<SimpleMMU::MappingEvent*>(ev);
        if (!map_ev) 
            { out->fatal(CALL_INFO, -1, "Error! Bad Event Type received at init time\n"); }
        out->verbose(_L3_, "Got mappingEvent at init time: %s\n", map_ev->getString().c_str());
        
        //delegate execution to inner function
        SimpleMMU::MappingEvent* resp_ev = handleMappingEventInner(map_ev);

        link_from_os->sendInitData(resp_ev); //send back response
    }


}


// Page mappings requests from OS
void SimpleMMU::handleMappingEvent(SST::Event *ev) {
    auto map_ev  = dynamic_cast<SimpleMMU::MappingEvent*>(ev);
    if (!map_ev) 
        { out->fatal(CALL_INFO, -1, "Error! Bad Event Type received\n"); }

    out->verbose(_L3_, "Got mappingEvent: %s\n", map_ev->getString().c_str());

    //delegate body to inner function
    SimpleMMU::MappingEvent* resp_ev = handleMappingEventInner(map_ev);

    out->verbose(_L3_, "Sending back response\n");
    link_from_os->send(resp_ev);
}



// Handle page-mappings requests from OS
// inner logic: doesn't send or receive anything:
// returns a response mapping-event
SimpleMMU::MappingEvent* SimpleMMU::handleMappingEventInner(SimpleMMU::MappingEvent* map_ev) {
    //map_ev should be non_null
    //if requested mapping doesn't exist, exit with error

    switch(map_ev->type) {
        case SimpleMMU::MappingEvent::eventType::CREATE_MAPPING:
            out->verbose(_L3_, "Creating mapping with id=%lu\n", map_ev->map_id);

            // crash out if already exists
            sst_assert(pagetables_map.find(map_ev->map_id) == pagetables_map.end(), CALL_INFO, -1, 
                    "ERROR: Trying to create page mapping #%lu which already exists\n", map_ev->map_id);

            pagetables_map[map_ev->map_id] = PageTable();
            break;

        case SimpleMMU::MappingEvent::eventType::MAP_PAGE:
            out->verbose(_L3_, "Mapping page at VA=0x%lx, PA=0x%lx\n", map_ev->v_addr, map_ev->p_addr);
            getMap(map_ev->map_id)->mapPage(map_ev->v_addr, map_ev->p_addr, 0); //no flags for now
            break;

        case SimpleMMU::MappingEvent::eventType::UNMAP_PAGE:
            out->verbose(_L3_, "Unmapping page at VA=0x%lx\n", map_ev->v_addr);
            getMap(map_ev->map_id)->unmapPage(map_ev->v_addr, 0); //no flags for now
            break;
        default:
            out->verbose(_L1_, "Event type %s not handled (noop)\n", map_ev->getTypeString().c_str());
    }

    out->verbose(_L3_, "Responding with same event\n");
    return map_ev;
}



//Incoming MemEvents from TLBs, we need to translate them
void SimpleMMU::handleTranslationEvent(SST::Event *ev, int tlb_link_index) {
    auto mem_ev  = dynamic_cast<MemHierarchy::MemEvent*>(ev);
    if (!mem_ev) 
        { out->fatal(CALL_INFO, -1, "Error! Bad Event Type received"); }

    out->verbose(_L3_, "Got translation req. (MemEvent) on link_from_tlb%d: %s\n", 
            tlb_link_index, mem_ev->getVerboseString().c_str());
    
    // Translate and send back
    uint64_t map_id = v_tlb_mapping_ids[tlb_link_index]; // lookup id for given link (should always exist)
    MemEvent *translated_mem_ev = translateMemEvent(map_id, mem_ev);

    out->verbose(_L5_, "Sending back translated MemEvent to TLB\n");
    delete mem_ev; //delete original, we send back a copy

    v_link_from_tlb[tlb_link_index]->send(translated_mem_ev);
}




// ========================================================================
//
//            THE GOOD STUFF: (actually translating things)
//
// ========================================================================



MemEvent* SimpleMMU::translateMemEvent(uint64_t map_id, MemEvent *mEv) {
    //Returns translated copy of mEv
    //CALLER MUST DELETE ORIGINAL mEV

    Addr vAddr    = mEv->getVirtualAddress();
    Addr mainAddr = mEv->getAddr();
    Addr baseAddr = mEv->getBaseAddr(); //base of the cache line I think?

    if(vAddr == 0) {
        //Some components just don't set it?
        //we should set it for legibility
        mEv->setVirtualAddress(mainAddr);
    } else if(vAddr != mainAddr) {
        out->verbose(_L2_, "Unexpected: MemEvent's vAddr and Addr differ: %s\n", mEv->getVerboseString().c_str() );
    }

    Addr vPage =          mainAddr & ~BOTTOM_N_BITS(12);
    Addr pageOffset =     mainAddr &  BOTTOM_N_BITS(12);
    Addr baseAddrOffset = baseAddr &  BOTTOM_N_BITS(12);


    //======== Do the translation
    Addr pPage = translatePage(map_id, vPage);


    //======== Cleanup and return

    //Stitch together result
    Addr pAddr = pPage | pageOffset;
    Addr pBaseAddr = pPage | baseAddrOffset; //should always be in same page


    MemEvent *new_mEv = mEv->clone();
    new_mEv->setAddr(pAddr);
    new_mEv->setBaseAddr(pBaseAddr);

    //out->verbose(_L10_, "Orig. MemEvent  : %s\n", mEv->getVerboseString().c_str());
    out->verbose(_L10_, "Transl. MemEvent: %s\n", new_mEv->getVerboseString().c_str());


    return new_mEv;
}


Addr SimpleMMU::translatePage(uint64_t map_id, Addr virtPageAddr) {
    // Returns physical page addr
    // On page fault, crash out (TODO: add page fault support)
    
    PageTable *pt = getMap(map_id);
    PageTableEntry ent = pt->lookup(virtPageAddr);

    
    if(!ent.isValid()) {
        //// unmapped pages are a page fault
        //sst_assert(ent.isValid(), CALL_INFO, -1, "ERROR: Pagetable page fault (no mapping at VA=0x%lx)\n", ent.v_addr);

        //TODO TEMP: for now we let the default mapping play out if no entry found instead of faulting
        return virtPageAddr;

    } else {
        return ent.p_addr;
    }
    
    
    //     //==== TODO TEMP: hardcoded fixed-offset translation
    //     // to just check if the plumbing works, lets manually hardcode the fixed-region mapping from simpleTLB
    //     //
    //     const Addr fixed_mapping_va_start = 0x0;
    //     const Addr fixed_mapping_pa_start = 0xF000000;
    //     const Addr fixed_mapping_len      = 128 * MB;


    //     //check bounds
    //     //TODO: send proper page fault here once implmemented
    //     //TODO: check MemEvent size?
    //     //
    //     if(virtPageAddr < fixed_mapping_va_start || virtPageAddr >= fixed_mapping_va_start + fixed_mapping_len) {
    //         ////Option 1: anything not explicitly mapped is page fault
    //         //out->fatal(CALL_INFO, -1, "Page fault: virtual addr 0x%lx is outside of mapped range: 0x%lx - 0x%lx\n", virtPageAddr, fixed_mapping_va_start, fixed_mapping_va_start + fixed_mapping_len-1);

    //         ////Option 2: all other addresses get mapped to themselves (might be useful for e.g. faking multiprocess)
    //         return virtPageAddr;
    //     }


    //     // Add the VA->PA offset. This should work correctly whether
    //     // PA or VA are > or <, since everything is uint64_t
    //     Addr pPageAddr = virtPageAddr + (fixed_mapping_pa_start - fixed_mapping_va_start);

    //     //Addr pPage = virtPageAddr + SST::MemHierarchy::mebi; //offset by 1MiB, 0x10'0000
    //     //Addr pPage = vPage + 0x400; //offset by 1KB

    //     assert((pPageAddr & BOTTOM_N_BITS(12)) == 0); //we should have enforced this by the time we get here

    //     return pPageAddr;
}



// ========================================================================
//
//     PAGE TABLE DATA STRUCTURE (TODO: THIS SHOULD BE IT'S OWN, NON-COMPONENT CLASS)
//      (especially since we'll need more than one of it)
//
// ========================================================================



//typedef struct { //must be 4k aliged
//    Addr v_addr;
//    Addr p_addr;
//    uint64_t flags; //TODO: add getters and setters? (isValid, etc)
//} PageTableEntry;
//
//std::map<MemHierarchy::Addr, PageTableEntry> PTE_map;

void SimpleMMU::PageTable::mapPage(Addr v_addr, Addr p_addr, uint64_t flags) {
    if (!(IS_4K_ALIGNED(v_addr) && IS_4K_ALIGNED(p_addr)))
        { throw std::invalid_argument("ERROR: addresses must be 4K aligned\n"); }

    //error if ALREADY DOES exist (QUESTION: should this more permissive?)
    auto it = PTE_map.find(v_addr);
    if(it != PTE_map.end())
            { throw std::invalid_argument(strPrintf(
            "ERROR: PageTable mapping page over an existing mapping at VA=0x%lx\n", v_addr)); }

    PTE_map[v_addr] = PageTableEntry(v_addr, p_addr, flags | PT_FL_VALID);
}

void SimpleMMU::PageTable::unmapPage(Addr v_addr, uint64_t flags) {
    if (!IS_4K_ALIGNED(v_addr))
        { throw std::invalid_argument("ERROR: addresses must be 4K aligned\n"); }

    //error if it DOES NOT exist (QUESTION: should this more permissive?)   
    auto it = PTE_map.find(v_addr);
    if(it == PTE_map.end())
            { throw std::invalid_argument(strPrintf(
            "ERROR: PageTable unmap called when no mapping exists at VA=0x%lx\n", v_addr)); }

    PTE_map.erase(it);
}

//TODO: add a PT exists call? because we can have either the PTE doesn't exist in PT_map or it exists but is invalid

SimpleMMU::PageTableEntry SimpleMMU::PageTable::lookup(Addr v_addr) {
    //returns (by value) the pagetable entry
    //if no mapped page, returns a pageTableEntry with isValid() false and addresses set to -1)
    
    if (!IS_4K_ALIGNED(v_addr))
        { throw std::invalid_argument("ERROR: addresses must be 4K aligned\n"); }
    //TODO: with hugepages, should this be made more lenient? probably
    
    auto it = PTE_map.find(v_addr);

    if (it == PTE_map.end()) { //if not found
        return PageTableEntry(v_addr, -1, 0 & ~PT_FL_VALID); //just to be explicit, 
    } else {
        PageTableEntry ent = it->second;
        return ent;
    }

}
