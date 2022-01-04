#ifndef PAGETABLE_H
#define PAGETABLE_H

#include <sst_config.h>

#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
//#include <sst/core/timeConverter.h>
//#include <sst/core/interfaces/simpleMem.h>

#include <sst/elements/memHierarchy/memEventBase.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/elements/memHierarchy/util.h>

using SST::MemHierarchy::Addr;

namespace SST {
namespace SambaComponent {

//class PageTable : public SST::Component {
class PageTable : public SST::Component {
    public:

        // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
        SST_ELI_REGISTER_COMPONENT(
            PageTable,                          // Component class
            "Samba",                            // Component library (for Python/library lookup)
            "SimplePageTable",                  // Component name (for Python/library lookup)
            SST_ELI_ELEMENT_VERSION(1,0,0),     // Version of the component (not related to SST version)
            "PageTable to go with SimpleTLB",   // Description
            COMPONENT_CATEGORY_UNCATEGORIZED    // Category
        )


        SST_ELI_DOCUMENT_PARAMS(
            {"verbose", "(uint) Output verbosity for warnings/errors. 0[fatal error only], 1[warnings], 2[full state dump on fatal error]","1"},
        )

        SST_ELI_DOCUMENT_PORTS(
            {"link_from_os", "Requests to map/unmap pages", {"PageTable.MappingEvent"}},
            {"link_from_tlb", "Requests for translations",  {"MemHierarchy.MemEventBase"}},
            // {"Port name", "Description", { "list of event types that the port can handle"} }
        )



    typedef enum { ERR_EVENT=0, CREATE_MAPPING, MAP_PAGE, UNMAP_PAGE, MAP_ANON_PAGE } Map_EventType ;
    enum { PAGE_FAULT, PAGE_FAULT_RESPONSE, PAGE_FAULT_SERVED, SHOOTDOWN, PAGE_REFERENCE, SDACK } Translate_EventType ;

    class MappingEvent; // map page, unmap page, etc
    class TranslateEvent; // translate-virt-page;  page faults?; 



    class MappingEvent : public SST::Event  {
        public:
            // Members:
            Map_EventType   type;
            Addr            v_addr;
            Addr            p_addr;
            int64_t         size;
            
            // Constructor
            MappingEvent() : SST::Event(), 
                type(Map_EventType::ERR_EVENT),
                v_addr(0),
                p_addr(0),
                size(0)
            {}


            // Events must provide a serialization function that serializes
            // all data members of the event
            void serialize_order(SST::Core::Serialization::serializer &ser)  override {
                Event::serialize_order(ser);
                ser & type;
                ser & v_addr;
                ser & p_addr;
                ser & size;
            }

            // Register this event as serializable
            ImplementSerializable(PageTable::MappingEvent);


        public:
            std::string getString() {
                char str_buff[512];
                snprintf(str_buff, sizeof(str_buff), 
                        "<MappingEvent: type: %d, VA 0x%lx, PA 0x%lx, size: 0x%lx>", 
                        type, v_addr, p_addr, size);
                return std::string(str_buff);
            }

    }; //class MappingEvent



    public:
        // ???
        PageTable(SST::ComponentId_t id, SST::Params& params);


        void handleMappingEvent(Event *ev); // handles event from OS: adjusts mappings, sends back response on link
        void handleTranslationEvent(Event *ev); // translate memEvents from tlbs

    private:
        // SST Output object, for printing, error messages, etc.        
        SST::Output* out;                                               

        SST::Link* link_from_os;  // link to whoever is creating mappings
        SST::Link* link_from_tlb; // incoming memEvent translation requests


        // void handleTranslateEvent(){} //TODO
};

} //namespace SambaComponent
} //namespace SST



#endif /* PAGETABLE_H */








// ============================= CODE FOR LATER::::
//
//
//    // Thie defines a class for events of Samba
//    class PTEvent : public SST::Event
//    {
//
//        private:
//        SambaEvent() { } // For serialization
//
//            int ev;
//            uint64_t address;
//            uint64_t paddress;
//            uint64_t size;
//        public:
//
//            SambaEvent(EventType y) : SST::Event()
//        { ev = y;}
//
//            void setType(int ev1) { ev = static_cast<EventType>(ev1);}
//            int getType() { return ev; }
//
//            void setResp(uint64_t add, uint64_t padd, uint64_t sz) { address = add; paddress = padd; size = sz;}
//            uint64_t getAddress() { return address; }
//            uint64_t getPaddress() { return paddress; }
//            uint64_t getSize() { return size; }
//
//            void serialize_order(SST::Core::Serialization::serializer &ser) override {
//                Event::serialize_order(ser);
//            }
//
//
//        ImplementSerializable(SambaEvent);
//
//    };
