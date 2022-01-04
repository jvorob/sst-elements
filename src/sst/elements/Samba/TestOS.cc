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


TestOSComponent::TestOSComponent(SST::ComponentId_t id, SST::Params& params) {
    int verbosity = params.find<int>("verbose", 1);
    out = new SST::Output("TestOS[@f:@l:@p] ", verbosity, 0, SST::Output::STDOUT);
    out->verbose(_L1_, "Creating TestOS component...\n");


    //  pt_iface = loadUserSubComponent<PageTableInterface>("pagetable_interface");
    //  if (pt_iface == NULL) 
    //      { out->fatal(CALL_INFO, -1, "Error - unable to load 'pagetable_interface' subcomponent"); }

    //  //
    //  pt_iface->initialize(NULL, new Event::Handler<TestOSComponent>(this, &TestOSComponent::handlePageTableEvent));
}


void TestOSComponent::handlePageTableEvent(Event *ev) {
    out->verbose(CALL_INFO, 1, 0, "Got event from pagetable_interface\n");

    delete ev; //receiver has responsibility for deleting events
}


