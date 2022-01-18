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
            num_pending_requests = 0;
            parent_handler = NULL;

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

        // ==================== Init phase:
        
        void init(unsigned int phase) override { 
            if (phase == 0)
                { out->verbose(_L1_, "Init phase 0 (in setup() )\n"); }

            //TODO TEMP: let's ignore event responses at init time
            // //Need to handle incoming event responses from any mapping requests we sent
            // SST::Event *ev;
            // while ((ev = out_link->recvInitData())) {
            //     num_pending_requests--; //TODO: TEMP DEBUg
            //     //handleEvent(ev);
            // }
        }

        virtual void setup() override { 
            //Called after init phase ends: switch init mode off
            num_pending_requests = 0; // Init won't finish until all messages sent
                                      // Can reset num_pending for main phase

            out->verbose(_L5_, "Finished init phase (in setup() )\n");
        };

        // ==================== Setup response handler for parent component

        //Connect to link
        // initialize with `initialize(link_ptr, new Event::Handler<ClassFoo>(this, ClassFoo::handleEvent))`
        void initialize (SST::Link *link, Event::HandlerBase *handler = NULL) {
            parent_handler = handler;
            out->verbose(_L1_, "initialize(...) called\n");

        };


        int getNumPendingRequests() {
            //Number of requests still waiting for a response
            //Parent object should use this in callback to determine when all mappings are in place
            return num_pending_requests;
        }

        // Event handler for PageTable event responses
        void handleEvent(SST::Event *ev) {
            auto map_event = dynamic_cast<PageTable::MappingEvent*>(ev);

            if (!map_event) {
                out->fatal(CALL_INFO, -1, "Error! Bad Event Type received by %s, expecting MappingEvent\n", getName().c_str());
                return;
            }

            out->verbose(_L3_, "got event response: %s\n", map_event->getString().c_str());

            num_pending_requests--;

            //Should only happen if we get spurious incoming events from the page table?
            //TODO: if we make page table send us stuff ever, this logic will have to change
            sst_assert(num_pending_requests >= 0, CALL_INFO, -1, "ERROR: count of inflight events shouldn't be negative\n");

            //forward to parent
            if(parent_handler != NULL) {
                (*parent_handler)(ev);
            } else { //if parent doesn't want it, we need to delete it
                delete ev;
            }
        }


        // ==================== Event-sending Methods
        // These can be called either at experiment time or at init() time (TODO)
        // public functions delegate to impl funcs with `is_init` set to true or false
        
        void createMapping    (uint64_t map_id) { createMappingImpl(map_id, false); }
        void initCreateMapping(uint64_t map_id) { createMappingImpl(map_id, true); }


        void mapPage          (uint64_t map_id, Addr v_addr, Addr p_addr, uint64_t flags) 
            { mapPageImpl(map_id, v_addr, p_addr, flags, false); }

        void initMapPage      (uint64_t map_id, Addr v_addr, Addr p_addr, uint64_t flags) 
            { mapPageImpl(map_id, v_addr, p_addr, flags, true); }


        void unmapPage        (uint64_t map_id, Addr v_addr, uint64_t flags) 
            { unmapPageImpl(map_id, v_addr, flags, false); }

        void initUnmapPage    (uint64_t map_id, Addr v_addr, uint64_t flags) 
            { unmapPageImpl(map_id, v_addr, flags, true); }

        // === Implementations of event sending methods
        // (do different sends depending on if init or not)
        // don't need to track num_pending_requests at init time

        private:
        void createMappingImpl(uint64_t map_id, bool is_init) {
            out->verbose(_L3_, "Sending CREATE_MAPPING (does nothing for now)\n");
            if (!is_init) { num_pending_requests++; }

            auto type = PageTable::MappingEvent::eventType::CREATE_MAPPING;
            auto ev = new PageTable::MappingEvent(type, map_id, -1, -1);
            
            if (!is_init)  //Normal send
                { out_link->send(ev); } 
            else //init-time send
                { out_link->sendInitData(ev); }
        };


        void mapPageImpl(uint64_t map_id, Addr v_addr, Addr p_addr, uint64_t flags, bool is_init) {
            out->verbose(_L3_, "Sending MAP_PAGE\n");
            if (!is_init) { num_pending_requests++; }

            auto type = PageTable::MappingEvent::eventType::MAP_PAGE;
            auto ev = new PageTable::MappingEvent(type, map_id, v_addr, p_addr);

            if (!is_init)  //Normal send
                { out_link->send(ev); } 
            else //init-time send
                { out_link->sendInitData(ev); }
        }
        void unmapPageImpl(uint64_t map_id, Addr v_addr, uint64_t flags, bool is_init) {
            out->verbose(_L3_, "Sending UNMAP_PAGE\n");
            if (!is_init) { num_pending_requests++; }

            auto type = PageTable::MappingEvent::eventType::UNMAP_PAGE;
            auto ev = new PageTable::MappingEvent(type, map_id, v_addr, -1);

            if (!is_init)  //Normal send
                { out_link->send(ev); } 
            else //init-time send
                { out_link->sendInitData(ev); }
        }



    private:
        int num_pending_requests;
        Event::HandlerBase *parent_handler;

        // SST Output object, for printing, error messages, etc.
        SST::Output* out;

        SST::Link *out_link;

};

} //namespace SambaComponent
} //namespace SST

#endif /* PAGETABLEINTERFACE_H */
