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


#ifndef C_TXNSCHEDULER_HPP
#define C_TXNSCHEDULER_HPP

#include "c_Transaction.hpp"
#include "c_TxnUnit.hpp"

namespace SST {
    namespace n_Bank {
        class c_TxnConverter;

        class c_TxnScheduler: public c_CtrlSubComponent <c_Transaction*,c_Transaction*>  {
        public:

            c_TxnScheduler(SST::Component *comp, SST::Params &x_params);
            ~c_TxnScheduler();

            bool clockTic(SST::Cycle_t);

            void print() const; // print internals

        private:

            c_TxnConverter *m_nextSubComponent;

            virtual void run();
            virtual void send();

        };
    }
}


#endif //C_TRANSACTIONSCHEDULER_HPP
