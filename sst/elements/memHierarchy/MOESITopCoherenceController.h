// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * File:   MESITopCoherenceController.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#ifndef __SST__MOESITopCoherenceController__
#define __SST__MOESITopCoherenceController__

#include <iostream>
#include "MESITopCoherenceController.h"


namespace SST { namespace MemHierarchy {



/*--------------------------------------------------------------------------------------------
 * Top Controller that implements the MOESI protocol, should be use by any cache that is not L1
 * keeps sharers state, and handles downgrades and invalidates to lower level hierarchies
  -------------------------------------------------------------------------------------------*/
class MOESITopCC : public MESITopCC{
private:




public:
    MOESITopCC(const SST::MemHierarchy::Cache* _cache, Output* _dbg, uint _protocol, uint _numLines,
               uint _lineSize, uint64_t _accessLatency, uint64_t _mshrLatency, vector<Link*>* _childrenLinks) :
               MESITopCC(_cache, _dbg, _protocol, _numLines, _lineSize, _accessLatency, _mshrLatency,  _childrenLinks){}
    
    bool handleEviction(int lineIndex, BCC_MESIState _state);
    void handleFetchInvalidate(CacheLine* _cacheLine, Command _cmd);
    bool handleRequest(MemEvent* event, CacheLine* cacheLine);
    void handleInvalidate(int lineIndex, Command cmd);
    void handleInvAck(MemEvent* event, CCLine* ccLine);

};

}}


#endif /* defined(__SST__MOESITopCoherenceController__) */
