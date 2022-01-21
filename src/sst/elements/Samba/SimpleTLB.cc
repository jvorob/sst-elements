// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

/* Author: Janet Vorobyeva
 * E-mail: jvoroby@sandia.gov
 */



#include <sst_config.h>
#include "SimpleTLB.h"

//#include <string>

using namespace SST::Interfaces;
using namespace SST;
using namespace SST::SambaComponent;

#include <sst/elements/memHierarchy/memEventBase.h>
#include <sst/elements/memHierarchy/memEvent.h>
using SST::MemHierarchy::MemEventBase;
using SST::MemHierarchy::MemEvent;
using SST::MemHierarchy::Addr;
#include <sst/elements/memHierarchy/util.h>


// VERBOSITIES (using definitions from memHierarchy/util.h
#define _L1_  CALL_INFO,1,0   //INFO  (this one isnt in util.h for some reason)
// #define _L2_  CALL_INFO,2,0   //warnings
// #define _L3_  CALL_INFO,3,0   //external events
// #define _L5_  CALL_INFO,5,0   //internal transitions
// #define _L10_  CALL_INFO,10,0 //everything

#define BOTTOM_N_BITS(N) ((~0UL) >> (64-(N)))
#define PAGE_OFFSET_4K(ADDR) ((ADDR) & BOTTOM_N_BITS(12))



SimpleTLB::SimpleTLB(SST::ComponentId_t id, SST::Params& params): Component(id) {

    int verbosity = params.find<int>("verbose", 1);
    out = new SST::Output("SimpleTLB[@f:@l:@p] ", verbosity, 0, SST::Output::STDOUT);
    out->verbose(_L1_, "Creating SimpleTLB component...\n");

	//int page_walk_latency = ((uint32_t) params.find<uint32_t>("page_walk_latency", 50));

	int64_t simple_translation_offset_addr = params.find<int64_t>("offset_addr", 0);
	if (simple_translation_offset_addr != 0) {
        out->verbose(_L1_, "Creating SimpleTLB component...\n");
    }


    // ============== Parse fixed mapping params
	fixed_mapping_va_start = params.find<Addr>("fixed_mapping_va_start", -1);
	fixed_mapping_pa_start = params.find<Addr>("fixed_mapping_pa_start", -1);


    //do unit nonsense for 'fixed_mapping_len'
    //TODO: put this logic in its own function
	std::string fixed_mapping_len_str =  params.find<std::string>("fixed_mapping_len", "0KiB");
    //pass through fixByteUnits to change MB to MiB (and so on), need powers of 2
    SST::MemHierarchy::fixByteUnits(fixed_mapping_len_str); //turn 'MB' to 'MiB'
	UnitAlgebra fixed_mapping_len_ua = UnitAlgebra(fixed_mapping_len_str); //parse it

    fixed_mapping_len = fixed_mapping_len_ua.getRoundedValue(); //extract int val

    //NOTE: printed string won't be exactly what user entered: we modified it in-place with fixByteUnits
    if (!fixed_mapping_len_ua.hasUnits("") && !fixed_mapping_len_ua.hasUnits("B")) {
        out->fatal(CALL_INFO, -1, "Invalid param: fixed_mapping_len - must have units of bytes (B). "
                                    "Ex: '8B', '40MB'; You entered: '%s'", fixed_mapping_len_str.c_str());
    }



    // if len == 0, mapping is disabled. Else: enable and sanity-check
	if (fixed_mapping_len != 0) {
        if(PAGE_OFFSET_4K(fixed_mapping_va_start) != 0)
            { out->fatal(CALL_INFO, -1, "Error! 'fixed_mapping_va_start' not 4k aligned in %s. Got value '0x%lx'\n",
                    getName().c_str(), fixed_mapping_va_start); }
        if(PAGE_OFFSET_4K(fixed_mapping_pa_start) != 0)
            { out->fatal(CALL_INFO, -1, "Error! 'fixed_mapping_pa_start' not 4k aligned in %s. Got value '0x%lx'\n",
                    getName().c_str(), fixed_mapping_pa_start); }
        if(PAGE_OFFSET_4K(fixed_mapping_len) != 0)
            { out->fatal(CALL_INFO, -1, "Error! 'fixed_mapping_len' not 4k aligned in %s. Got value: '0x%lx'\n",
                    getName().c_str(), fixed_mapping_len); }

        if(fixed_mapping_len < 0) {
            out->fatal(CALL_INFO, -1, "Error! 'fixed_mapping_len' must not be negative in %s\n", getName().c_str());
        }

        out->verbose(_L1_, "Setting SimpleTLB with fixed mapping: %ldKB region at VA:0x%lx-0x%lx maps to PA:0x%lx\n",
                        fixed_mapping_len / 1024, fixed_mapping_va_start,
                        fixed_mapping_va_start+fixed_mapping_len-1, fixed_mapping_pa_start);
    }



    // Wire up our links to event handlers
    // One function for each link (since we're always moving memevents around, so pass in enum for source)
    link_high = configureLink("high_network",
            new Event::Handler<SimpleTLB, enum_event_src>(this, &SimpleTLB::handleEvent, FROM_HIGH));
    link_low = configureLink("low_network",
            new Event::Handler<SimpleTLB, enum_event_src>(this, &SimpleTLB::handleEvent, FROM_LOW));
    link_pagetable = configureLink("pagetable_link",
            new Event::Handler<SimpleTLB, enum_event_src>(this, &SimpleTLB::handleEvent, FROM_PT));

    // Failure usually means the user didn't connect the port in the input file
    sst_assert(link_low,       CALL_INFO, -1, "Error in %s: Failed to configure port 'link_low'\n", getName().c_str());
    sst_assert(link_high,      CALL_INFO, -1, "Error in %s: Failed to configure port 'link_high'\n", getName().c_str());
    sst_assert(link_pagetable, CALL_INFO, -1, "Error in %s: Failed to configure port 'link_pagetable'\n", getName().c_str());


    // Setup clock for delaying things
	std::string cpu_clock = params.find<std::string>("clock", "1GHz");
	registerClock( cpu_clock, new Clock::Handler<SimpleTLB>(this, &SimpleTLB::clockTick ) );


}

SimpleTLB::~SimpleTLB() {
    delete out;
}



void SimpleTLB::init(unsigned int phase) {
	// Pass-through init events between CPU and Memory/Caches
    // (Before main simulation begins)
    // (NO TRANSLATIONS FOR INIT-TIME WRITES)

    out->verbose(_L2_, "SimpleTLB::init(%d) called\n", phase);

    SST::Event * ev;
    while ((ev = link_high->recvInitData())) { //incoming from CPU, forward down
        link_low->sendInitData(ev);
    }

    while ((ev = link_low->recvInitData())) { //incoming from mem/caches, forward up
        link_high->sendInitData(ev);
    }

    /*  // == CODE COPIED FROM SAMBA: to snoop on cache-line size from mem init events
     *
        // snoop on requests coming up from from mem
        SST::MemHierarchy::MemEventInit * mEv = dynamic_cast<SST::MemHierarchy::MemEventInit*>(ev);
        if (mEv && mEv->getInitCmd() == SST::MemHierarchy::MemEventInit::InitCommand::Coherence) {
            SST::MemHierarchy::MemEventInitCoherence * mEvC = static_cast<SST::MemHierarchy::MemEventInitCoherence*>(mEv);
            TLB[i]->setLineSize(mEvC->getLineSize());
    }
    */
}

void SimpleTLB::handleEvent(SST::Event *ev, enum_event_src from) {
    // Shared handler for all links
    // Current strategy is:  mem events from above are sent to page table for translation,
    //                       translated events from page table are sent down to mem (link_low)
    //                       filled memevents from below are return up to cpu (link_high)
    //

    MemEventBase* m_event_base = dynamic_cast<MemEventBase*>(ev);

    if (!m_event_base) {
        out->fatal(CALL_INFO, -1, "Error! Bad Event Type received by %s, expecting MemEventBase\n", getName().c_str());
        return;
    }



    out->verbose(_L3_, "Got MemEventBase from link %d: %s\n", (int)from, m_event_base->getVerboseString().c_str());

    if (from == FROM_LOW) { //event from cache going back up, just forward it
        out->verbose(_L3_, "Got mem event in SimpleTLB, forwarding to link_high\n");
        link_high->send(m_event_base);




    } else if (from == FROM_HIGH) { //event going down from cpu
        //For now, just forward it to page table for translation
        MemEvent* m_event = dynamic_cast<MemEvent*>(ev);

        if (!m_event) {
            out->verbose(_L5_, "Sending down MemEventBase (not MemEvent), forwarding to link_low\n");
            link_low->send(ev);

        } else { //Able to cast to a MemEvent
            //TODO: TEMP TESTING STUFF
            out->verbose(_L5_, "TEMP:sending memevent to pagetable\n");
            link_pagetable->send(ev);


            // MemEvent *translated_m_event = translateMemEvent(m_event);
            // out->verbose(_L5_, "Sending down translated MemEvent in SimpleTLB to link_low\n");
            //
            // delete m_event;
            // link_low->send(translated_m_event);

            //out->verbose(_L3_, "Sending down memevent in SimpleTLB, vAddr 0x%lx, translating to 0x%lx, translating and forwarding to link_low\n", vAddr, transAddr);

        }

    } else if (from == FROM_PT) { // translated response from pagetable
        out->verbose(_L5_, "Got translated memevent back from pagetable: %s, sending down to mem\n", 
                            m_event_base->getVerboseString().c_str());
        link_low->send(ev);


    } else { // bad enum val, shouldn't happen unless I typoed something
        sst_assert(false, CALL_INFO, -1, "Error in %s: bad enum value in handleEvent", getName().c_str());
    }

    // Receiver has the responsiblity for deleting events
    // don't delete it since we're forwarding it
}




//noop for now, but we'll need it later when delays are a thing
bool SimpleTLB::clockTick(SST::Cycle_t x)
{
    //debug output first few times
    static int debug_print_i = 0;
    if (debug_print_i < 20) {
        out->verbose(_L10_, "Calling SimpleTLB tick on cycle %ld...\n", x);
        debug_print_i++;
    }

	// We tick the MMU hierarchy of each core
	//for(uint32_t i = 0; i < core_count; ++i)
		//TLB[i]->tick(x);


    // return false to indicate clock handler shouldn't be disabled
	return false;
}



// ========================== THE MEAT AND POTATOES =======================

MemEvent* SimpleTLB::translateMemEvent(MemEvent *mEv) {
    //TODO: this will eventually have to be async, returning some sort of status handle on a miss
    //Creates translated copy of mEv
    //does not delete mEv

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
    //TODO: this will eventually have to be async, returning some sort of status handle on a miss
    //
    Addr pPage = translatePage(vPage); //offset by 1MiB, 0x10'0000



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


Addr SimpleTLB::translatePage(Addr virtPageAddr) {
    //TODO: later this will have to be async
    //TODO: return status on page fault, stall

    if(fixed_mapping_len == 0) { // If mapping disabled, just keep same addrs
        return virtPageAddr;
    }

    //Else, we have a proper mapping
    //check bounds
    //TODO: send proper page fault here once implmemented
    //TODO: check MemEvent size?
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
