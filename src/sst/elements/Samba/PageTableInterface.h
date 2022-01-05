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




#include <sst/elements/memHierarchy/util.h>


// VERBOSITIES (using definitions from memHierarchy/util.h
#define _L1_  CALL_INFO,1,0   //INFO  (this one isnt in util.h for some reason)
// #define _L2_  CALL_INFO,2,0   //warnings
// #define _L3_  CALL_INFO,3,0   //external events
// #define _L5_  CALL_INFO,5,0   //internal transitions
// #define _L10_  CALL_INFO,10,0 //everything


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


        // ========================== Constructor
        //
        PageTableInterface(ComponentId_t id, Params &params) : SubComponent(id) {
            int verbosity = params.find<int>("verbose", 5);
            out = new SST::Output("PageTableInterface[@f:@l:@p>] ", verbosity, 0, SST::Output::STDOUT);
            out->verbose(_L1_, "Creating PageTableInterface\n");



            out_link = configureLink("pagetable_link",
                new Event::Handler<PageTableInterface>(this, &PageTableInterface::handleEvent));

            // Failure usually means the user didn't connect the port in the input file
            sst_assert(out_link, CALL_INFO, -1, "Error in PageTableInterface: Failed to configure port 'out_link'\n");


        } ;

        ~PageTableInterface() {
            delete out;
        }

        // ==================== Event-sending Methods and Response handlers
        //

        //Connect to link
        //TODO: for now link is not-implemented, we're hard-wiring it into SimpleTLB for testing
        // initialize with `initialize(link_ptr, new Event::Handler<ThisClass>(this, ThisClass::handleEvent))`
        void initialize (SST::Link *link, Event::HandlerBase *handler = NULL) {
            out->verbose(_L1_, "initialize(...) called\n");
        };



        void createMapping(uint64_t map_id) {
            out->verbose(_L3_, "createMapping() called, sending blank mappingEvent\n");

            auto type = PageTable::MappingEvent::eventType::CREATE_MAPPING;
            auto ev = new PageTable::MappingEvent(type, map_id, -1, -1);
            out_link->send(ev);
        };


        void mapPage(uint64_t map_id, Addr v_addr, Addr p_addr, uint64_t flags) {
            auto type = PageTable::MappingEvent::eventType::MAP_PAGE;
            auto ev = new PageTable::MappingEvent(type, map_id, v_addr, p_addr);
            out_link->send(ev);
        }
        void unmapPage(uint64_t map_id, Addr v_addr, uint64_t flags) {
            auto type = PageTable::MappingEvent::eventType::UNMAP_PAGE;
            auto ev = new PageTable::MappingEvent(type, map_id, v_addr, -1);
            out_link->send(ev);
        }

        // Event handler for PageTable event responses
        void handleEvent(SST::Event *ev) {
            auto map_event = dynamic_cast<PageTable::MappingEvent*>(ev);

            if (!map_event) {
                out->fatal(CALL_INFO, -1, "Error! Bad Event Type received by %s, expecting MappingEvent\n", getName().c_str());
                return;
            }

            out->verbose(_L3_, "got event response: %s\n", map_event->getString().c_str());
        }


    private:
        // SST Output object, for printing, error messages, etc.
        SST::Output* out;

        SST::Link *out_link;

};

} //namespace SambaComponent
} //namespace SST

#endif /* PAGETABLEINTERFACE_H */
