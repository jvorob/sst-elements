#ifndef PAGETABLE_H
#define PAGETABLE_H

#include <sst_config.h>

#include <sst/core/sst_types.h>
#include <sst/core/event.h>
//#include <sst/core/component.h>
//#include <sst/core/link.h>
//#include <sst/core/timeConverter.h>
//#include <sst/core/interfaces/simpleMem.h>
#include <sst/elements/memHierarchy/util.h>

using SST::MemHierarchy::Addr;

namespace SST {
namespace SambaComponent {

//class PageTable : public SST::Component {
class PageTable {
    public:


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
        PageTable();

    private:
        // SST Output object, for printing, error messages, etc.        
        SST::Output* out;                                               

        SST::Link* os_link; //link to whoever is creating mappings

        // handles incoming event: sends back response on link
        void handleMappingEvent(Event *ev);

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
