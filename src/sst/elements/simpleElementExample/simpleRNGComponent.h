// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
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

#ifndef _SIMPLERNGCOMPONENT_H
#define _SIMPLERNGCOMPONENT_H

#include "sst/core/component.h"
#include <sst/core/elementinfo.h>
#include "sst/core/rng/sstrng.h"

using namespace SST;
using namespace SST::RNG;

namespace SST {
namespace SimpleRNGComponent {

class simpleRNGComponent : public SST::Component 
{
public:
    simpleRNGComponent(SST::ComponentId_t id, SST::Params& params);
    ~simpleRNGComponent();
    void setup()  { }
    void finish() { }

private:
    simpleRNGComponent();  // for serialization only
    simpleRNGComponent(const simpleRNGComponent&); // do not implement
    void operator=(const simpleRNGComponent&); // do not implement
    
    virtual bool tick(SST::Cycle_t);

    Output* output;
    SSTRandom*  rng;
    std::string rng_type;
    int rng_max_count;
    int rng_count;
    
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        simpleRNGComponent,
        "simpleElementExample",
        "simpleRNGComponent",
        "Random number generation component",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )
    
    SST_ELI_DOCUMENT_PARAMS(
        { "seed_w", "The seed to use for the random number generator", "7" },
        { "seed_z", "The seed to use for the random number generator", "5" },
        { "seed", "The seed to use for the random number generator.", "11" },
        { "rng", "The random number generator to use (Marsaglia or Mersenne), default is Mersenne", "Mersenne"},
        { "count", "The number of random numbers to generate, default is 1000", "1000" },
        { "verbose", "Sets the output verbosity of the component", "0" }
    )

    SST_ELI_DOCUMENT_STATISTICS(
    )

    SST_ELI_DOCUMENT_PORTS(
    )

};

} // namespace SimpleRNGComponent
} // namespace SST

#endif /* _SIMPLERNGCOMPONENT_H */
