// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// Copyright 2015 IBM Corporation

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//   http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include "sst_config.h"

// std includes
#include <iostream>
#include <assert.h>

// local includes
#include "c_TxnScheduler.hpp"

using namespace SST;
using namespace SST::n_Bank;
using namespace std;

c_TxnScheduler::c_TxnScheduler(SST::Component *owner, SST::Params& x_params) :  c_CtrlSubComponent <c_Transaction*,c_Transaction*> (owner, x_params) {
    m_nextSubComponent = dynamic_cast<c_Controller *>(owner)->getTxnConverter();
}

c_TxnScheduler::~c_TxnScheduler() {

}


bool c_TxnScheduler::clockTic(SST::Cycle_t){

    run();
    send();
    return false;
}

void c_TxnScheduler::run(){

    //FCFS
    if(m_outputQ.empty())
        if(!m_inputQ.empty()) {
            m_outputQ.push_back(m_inputQ.front());
            m_inputQ.pop_front();
        }
}

void c_TxnScheduler::send()
{
    int token=m_nextSubComponent->getToken();

    while(token>0 && !m_outputQ.empty()) {
        m_nextSubComponent->push(m_outputQ.front());
//        m_outputQ.front()->print(m_debugOutput);
        m_outputQ.pop_front();
        token--;
    }
}

