#ifndef PAGETABLEINTERFACE_H
#define PAGETABLEINTERFACE_H


#include <sst_config.h>

#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/subcomponent.h>
//#include <sst/core/link.h>
//#include <sst/core/timeConverter.h>
//#include <sst/core/interfaces/simpleMem.h>
//#include <sst/elements/memHierarchy/util.h>

namespace SST {
namespace SambaComponent {

class PageTableInterface : public SST::SubComponent {

    public:


        SST_ELI_REGISTER_SUBCOMPONENT_API(SST::SambaComponent::PageTableInterface)


        SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(PageTableInterface, "Samba", "PageTableInterface", SST_ELI_ELEMENT_VERSION(1,0,0),
                 "interface to communicate over link to pagetable component", SST::SambaComponent::PageTableInterface)


        SST_ELI_DOCUMENT_PARAMS(
            {"verbose", "(uint) Output verbosity for warnings/errors. 0[fatal error only], 1[warnings], 2[full state dump on fatal error]","1"},
            // {"emulate_faults", "blah blah description description", "0"},
        )

        // {"Port name", "Description", { "list of event types that the port can handle"} }  
        SST_ELI_DOCUMENT_PORTS(
            //{"cpu_to_mmu%(corecount)d", "Each Samba has link to its core", {}},
            //{"mmu_to_cache%(corecount)d", "Each Samba to its corresponding cache", {}},
        )


        // ==============================
        //
        PageTableInterface(ComponentId_t id, Params &params) : SubComponent(id) {};

        //Connect to link
        //TODO: for now link is not-implemented, we're hard-wiring it into SimpleTLB for testing
        // initialize with `initialize(link_ptr, new Event::Handler<ThisClass>(this, ThisClass::handleEvent))`
        void initialize (SST::Link *link, Event::HandlerBase *handler = NULL);
        void createMapping(uint64_t mapping_id);

    

    private:

};

} //namespace SambaComponent
} //namespace SST

#endif /* PAGETABLEINTERFACE_H */
