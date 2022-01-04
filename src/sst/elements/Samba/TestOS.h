#ifndef TESTOS_H
#define TESTOS_H

#include <sst_config.h>

#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
//#include <sst/core/timeConverter.h>
//#include <sst/core/interfaces/simpleMem.h>
//#include <sst/elements/memHierarchy/util.h>


#include "PageTableInterface.h"

namespace SST {
namespace SambaComponent {

class TestOSComponent : public SST::Component {

    public:

        // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
        SST_ELI_REGISTER_COMPONENT(
            TestOSComponent,                           // Component class
            "Samba",             // Component library (for Python/library lookup)
            "TestOS",                         // Component name (for Python/library lookup)
            SST_ELI_ELEMENT_VERSION(1,0,0),     // Version of the component (not related to SST version)
            "DEBUG ONLY: for testing page table interface",     // Description
            COMPONENT_CATEGORY_UNCATEGORIZED    // Category
        )


        // SST_ELI_DOCUMENT_STATISTICS(
        //     //{ "tlb_hits",        "Number of TLB hits", "requests", 1},   // Name, Desc, Enable Level
        // )

        SST_ELI_DOCUMENT_PARAMS(
            {"verbose", "(uint) Output verbosity for warnings/errors. 0[fatal error only], 1[warnings], 2[full state dump on fatal error]","1"},
        )

        SST_ELI_DOCUMENT_PORTS(
            //{"high_network", "Link to cpu", {"MemHierarchy.MemEventBase"}},
            // {"Port name", "Description", { "list of event types that the port can handle"} }
        )

        SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
            {"pagetable_interface", "For ease of interacting (map/unmap) with PageTable component", "SST::SambaComponent::PageTableInterface"},
        //{ "lsq", "Load-Store Queue for Memory Access", "SST::Vanadis::VanadisLoadStoreQueue" },
        )


    public:
        // Constructor. Components receive a unique ID and the set of parameters that were assigned in the Python input.
        TestOSComponent(SST::ComponentId_t id, SST::Params& params);
        ~TestOSComponent();

        void handlePageTableEvent(Event *ev);

    private:
        PageTableInterface *pt_iface;

        SST::Output* out;                                               

        //  // Event handler, called when an event is received on high or low link
        //  void handleEvent(SST::Event *ev, bool is_low);



};

} //namespace SambaComponent
} //namespace SST

#endif /* TESTOS_H */


