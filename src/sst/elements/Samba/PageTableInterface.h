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


#include "SimplePageTable.h"

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
            {"pagetable_link", "link to SimplePageTable component that this controls", {}},
            //{"cpu_to_mmu%(corecount)d", "Each Samba has link to its core", {}},
            //{"mmu_to_cache%(corecount)d", "Each Samba to its corresponding cache", {}},
        )


        // ==============================
        //
        PageTableInterface(ComponentId_t id, Params &params) : SubComponent(id) {
            printf("AAAAAAAA\n (constructor in PagetableInterface\n");

            out_link = configureLink("pagetable_link", 
                new Event::Handler<PageTableInterface>(this, &PageTableInterface::handleEvent));

            // Failure usually means the user didn't connect the port in the input file
            sst_assert(out_link, CALL_INFO, -1, "Error in %s: Failed to configure port 'out_link'\n", getName().c_str());


        } ;

        //Connect to link
        //TODO: for now link is not-implemented, we're hard-wiring it into SimpleTLB for testing
        // initialize with `initialize(link_ptr, new Event::Handler<ThisClass>(this, ThisClass::handleEvent))`
        void initialize (SST::Link *link, Event::HandlerBase *handler = NULL) {
            printf("BBBBBBBB\n (initialize in PagetableInterface\n");
        };

        void createMapping(uint64_t mapping_id) {
            std::cout << "Called PageTableInterface::createMapping; NOT IMPLEMENTED!\n";

            auto ev = new PageTable::MappingEvent();

            std::cout << ev->getString() << std::endl;
            //out_link->send(ev);
        };


        // Event handler for PageTable event responses
        void handleEvent(SST::Event *ev) {
            printf("Got event response in PageTableInterface\n");
        }
    

    private:
        SST::Link *out_link;

};

} //namespace SambaComponent
} //namespace SST

#endif /* PAGETABLEINTERFACE_H */
