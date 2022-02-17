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
#include "TestOS.h"

//#include <string>

using namespace SST;
//using namespace SST::Interfaces;
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


#define DEFAULT_MAPPING_ID 0

TestOSComponent::TestOSComponent(SST::ComponentId_t id, SST::Params& params): Component(id) {
    int verbosity = params.find<int>("verbose", 1);
    out = new SST::Output("TestOS[@f:@l:@p] ", verbosity, 0, SST::Output::STDOUT);
    out->verbose(_L1_, "Creating TestOS component...\n");


    // Set up our interface to the page table
    pt_iface = loadUserSubComponent<PageTableInterface>("pagetable_interface");
    if (pt_iface == NULL)
        { out->fatal(CALL_INFO, -1, "Error - unable to load 'pagetable_interface' subcomponent"); }
    pt_iface->initialize(NULL, new Event::Handler<TestOSComponent>(this, &TestOSComponent::handlePageTableEvent));


    //Check noop param, disables creating any mappings
    noop = (bool) params.find<int>("noop", 0);

    if(noop) { return; }

    //ELSE: Tell the simulation not to end until we're ready
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // Setup clock for running state machine
	std::string cpu_clock = params.find<std::string>("clock", "1GHz");
	registerClock( cpu_clock, new Clock::Handler<TestOSComponent>(this, &TestOSComponent::clockTick ) );
}

TestOSComponent::~TestOSComponent() {
    delete out;
}


void TestOSComponent::handlePageTableEvent(Event *ev) {
    out->verbose(CALL_INFO, 1, 0, "Request completed in pagetable_interface, %d more requests in-flight\n", 
                pt_iface->getNumPendingRequests());

    delete ev; //receiver has responsibility for deleting events
}



// ================= Manual coding / state machine stuff

void TestOSComponent::init(unsigned int phase) {
    //Make sure we can map pages at init time
    
    pt_iface->init(phase);

    //If noop, create no mapping
    if(noop) { return; }

    out->verbose(CALL_INFO, 1, 0, "Init phase %d!\n", phase);

    if(phase == 0) {
        //Lets map pages 0x4000 and 0x5000 to 0XF4000 and 0xF500
        pt_iface->initCreateMapping(DEFAULT_MAPPING_ID);
        pt_iface->initMapPage(DEFAULT_MAPPING_ID, 0x4000, 0xF4000, 0);
        pt_iface->initMapPage(DEFAULT_MAPPING_ID, 0x5000, 0xF5000, 0);
    }
}

void TestOSComponent::setup() {
    pt_iface->setup();

    //If noop, create no mappings
    if(noop) { return; }
}

//noop for now, but we'll need it later when delays are a thing
bool TestOSComponent::clockTick(SST::Cycle_t x)
{
    //If noop, create no mappings
    if(noop) { return true; } //return true to disable this clock

    //we want to run a quick-and-dirty little state machine here
    //lets make some variables:
    static int tick_counter = 0;
    static int state = 0;
    static int delay = 0;

    tick_counter++;
    delay = (delay <= 0)?  0 : delay-1; //counts down

    switch(state) {
        case 0: {
            //Just send out one event

            //TEMP: don't do this, we already createmapping-ed in init()
            //not that createMapping() does anything yet
            //pt_iface->createMapping(1);

            state++;
            delay = 5;
            break;
        }

        case 1: { // Wait until delay ends, then send a mapPage
            if (delay > 0)
                { break; }

            // 4k pages are at 0x0000, 0x1000, 0x2000, 0x3000
            // lets map pages 4-7 to address 0xF4000 - 0xF7000
            out->verbose(CALL_INFO, 1, 0, "tick %d!\n", tick_counter);
            //pt_iface->mapPage(1, 0x4000, 0xF4000, 0); //mapped at init
            //pt_iface->mapPage(1, 0x5000, 0xF5000, 0); //mapped at init
            pt_iface->mapPage(DEFAULT_MAPPING_ID, 0x6000, 0xF6000, 0);
            pt_iface->mapPage(DEFAULT_MAPPING_ID, 0x7000, 0xF7000, 0);
            state++;
            break;
        }

        case 2: { // wait until map-pages return
            if(pt_iface->getNumPendingRequests() > 0)
                { break; }

            out->verbose(CALL_INFO, 1, 0, "All page-map requests completed, continuing\n");
            delay = 60; // leave some time before we unmap in the next stage
            state++;
            break;
        }

        case 3: { //Wait for a while, then unmap that page
            if (delay > 0)
                { break; }

            //Unmap 4 and 5, leave 6 and 7 intact
            out->verbose(CALL_INFO, 1, 0, "tick %d!\n", tick_counter);
            pt_iface->unmapPage(DEFAULT_MAPPING_ID, 0x4000, 0);
            pt_iface->unmapPage(DEFAULT_MAPPING_ID, 0x5000, 0);
            state++;
            break;
        }

        case 4: { //Wait for unmap requests to complete
            if(pt_iface->getNumPendingRequests() > 0)
                { break; }

            out->verbose(CALL_INFO, 1, 0, "All unmap requests completed, continuing\n");

            delay = 60; // leave some time before we shut down in the next stage
            state++;
            break;
        }


        case 5: {
            if (delay > 0)
                { break; }

            out->verbose(CALL_INFO, 1, 0, "TestOSComponent Done!\n");
            state++;

            // Tell SST that it's OK to end the simulation (once all primary components agree, simulation will end)
            primaryComponentOKToEndSim();

            // Retrun true to indicate that this clock handler should be disabled
            return true;
        }

    }


	// We tick the MMU hierarchy of each core
	//for(uint32_t i = 0; i < core_count; ++i)
		//TLB[i]->tick(x);


    // return false to indicate clock handler shouldn't be disabled
	return false;
}
