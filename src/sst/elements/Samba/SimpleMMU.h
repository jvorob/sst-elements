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

class SimpleMMU : public SST::Component {

    /*     MMU Documentation:
     *
     * Answers translation requests from TLB(s) according to one or more
     * internal memory mappings (page tables)
     *
     * Stores one or more memory mappigns (i.e. page tables)
     * Can edit mappings with MappingEvents to port 'link_from_os':
     * If a value is not specified, then it is 'dont-care' for that method
     *
     * - MAP_PAGE(id, v_addr, p_addr, flags) //flags will hold if huge page
     * - UNMAP_PAGE(id, v_addr)
     *
     *  // FOR LATER:  (when we support multiple mapping, will require TLB to send mapping id
     *                  and so will require a special translationEvent with a field for that (or changing the memevents)
     * - CREATE_MAPPING (id)
     *
     *  // FOR LATER (once I add an anon page pool)
     *  // anonymous page pool is
     * - SET_ANON_POOL(id, p_addr, size?))
     * - MAP_ANON_PAGE(id, v_addr, flags)
     * - UNMAP_ANON_PAGE(id, v_addr, flags)
     *
     * Future considerations:
     * - can we remap existing pages? Should this be an error or a modify
     * - can we map a huge page over existing smaller pages? probably error
     * - Can we map pages into the anon page pool?
     * - Can we change the anon page pool?
     *
     */



    public:

        // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
        SST_ELI_REGISTER_COMPONENT(
            SimpleMMU,                          // Component class
            "Samba",                            // Component library (for Python/library lookup)
            "SimpleMMU",                        // Component name (for Python/library lookup)
            SST_ELI_ELEMENT_VERSION(1,0,0),     // Version of the component (not related to SST version)
            "SimpleMMU to go with SimpleTLB",   // Description
            COMPONENT_CATEGORY_UNCATEGORIZED    // Category
        )


        SST_ELI_DOCUMENT_PARAMS(
            {"verbose", "(uint) Output verbosity for warnings/errors. 0[fatal error only], 1[warnings], 2[full state dump on fatal error]","1"},
        )

        SST_ELI_DOCUMENT_PORTS(
            {"link_from_os", "Requests to map/unmap pages", {"SimpleMMU.MappingEvent"}},
            {"link_from_tlb%d", "Requests for translations",  {"MemHierarchy.MemEventBase"}},
            // {"Port name", "Description", { "list of event types that the port can handle"} }
        )


    //=================== Internal Classes: (defined at bottom)

    public:
        class MappingEvent; // map page, unmap page, etc
        //class TranslateEvent; // translate-virt-page;  (will we need this? rn we just take memevents)
        //enum { PAGE_FAULT, PAGE_FAULT_RESPONSE, PAGE_FAULT_SERVED, 
        //        SHOOTDOWN, PAGE_REFERENCE, SDACK } Translate_EventType ; (will we need these?)

    private:
        class PageTableEntry;
        class PageTable;

    //Flags:
    static const uint64_t PT_FL_VALID = 1 << 0;
    //TODO: come up with more of these and number them in a more sensible way



    public:
        // ???
        SimpleMMU(SST::ComponentId_t id, SST::Params& params);

        void handleMappingEvent(Event *ev); // handles event from OS: adjusts mappings, sends back response on link
                                            // (delegates body to ...Inner() function)
        void handleTranslationEvent(Event *ev, int tlb_link_index); // translate memEvents from tlbs


        MappingEvent* handleMappingEventInner(MappingEvent *map_ev); 
        // inner function to execute mapping events (called from init handler or main handler)
        // returns response mapping_event

        // = init stuff overrides
        void init(unsigned int phase) override;  //called repeatedly during init (per each phase)

    private:
    

        // === Private Members:
        SST::Output* out; // SST Output object, for printing, error messages, etc.

        SST::Link* link_from_os;  // link to whoever is creating mappings
        std::vector<SST::Link*> v_link_from_tlb; // incoming memEvent translation requests (one per link)

        std::map<int, PageTable> pagetables_map;

        // ==== Page-table data structure function: eventually will be in own class
        // TODO: split out into own class
        
        


        //void PT_mapPage(Addr v_addr, Addr p_addr, uint64_t flags);
        //void PT_unmapPage(Addr v_addr, uint64_t flags);
        //PageTableEntry PT_lookup(Addr v_addr);
        
        // ====================================

        
        // === Private Methods:


        //Transates mEv, returns new m_ev
        SST::MemHierarchy::MemEvent* translateMemEvent(uint64_t map_id, SST::MemHierarchy::MemEvent *mEv);

        //Pure function to translate just a virt addr to physaddr
        SST::MemHierarchy::Addr      translatePage(uint64_t map_id, SST::MemHierarchy::Addr virtPageAddr);


        //gets the corresponding map or returns a verbose error
        PageTable *getMap(uint64_t id) {
            auto it = pagetables_map.find(id);
            sst_assert(it != pagetables_map.end(), CALL_INFO, -1, 
                    "ERROR: Trying to access MMU mapping #%lu, which doesn't exist\n", id);
            return &(it->second);
        }


        // ==== End of SimpleMMU members

    // =========================================================================
    //
    //                        INTERNAL CLASSES
    //
    // =========================================================================
    

    public: class MappingEvent : public SST::Event  {
        public:

            // =========== EventType Enum and static helper func
            
            typedef enum { ERR_EVENT=0, CREATE_MAPPING,   MAP_PAGE,   UNMAP_PAGE,   MAP_ANON_PAGE } eventType ;

            //func to turn enum vals into strings, haven't found a cleaner way to do it automagically
            #define RETURN_CASE_STR(s)  case s: return #s
            static std::string getEventName(eventType t) {
                switch(t) {
                    RETURN_CASE_STR( ERR_EVENT      );
                    RETURN_CASE_STR( CREATE_MAPPING );
                    RETURN_CASE_STR( MAP_PAGE       );
                    RETURN_CASE_STR( UNMAP_PAGE     );
                    RETURN_CASE_STR( MAP_ANON_PAGE  );
                    default:
                        return "INVALID_EVENT_ID";
                }
            }
            #undef RETURN_CASE_STR


            // Members:
            eventType       type;
            uint64_t        map_id; //which mapping this event applies to
            Addr            v_addr;
            Addr            p_addr;
            int64_t         size;

            // Constructor
            MappingEvent() : SST::Event(),
                type(eventType::ERR_EVENT),
                v_addr(0),
                p_addr(0),
                size(0)
            {}

            MappingEvent(eventType t, uint64_t map_id, Addr v_addr, Addr p_addr) : SST::Event(),
                type(t),
                map_id(map_id),
                v_addr(v_addr),
                p_addr(p_addr),
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
            ImplementSerializable(SimpleMMU::MappingEvent);

        public:
            std::string getString() {
                char str_buff[512];
                snprintf(str_buff, sizeof(str_buff),
                        "<MappingEvent: type: %s, id%lu, VA 0x%lx, PA 0x%lx, size: 0x%lx>",
                        getEventName(type).c_str(), map_id, v_addr, p_addr, size);
                return std::string(str_buff);
            }

            std::string getTypeString() {
                return getEventName(type);
            }

    }; // end class MappingEvent ==============================================


    private: class PageTableEntry { // ===============================================
        public:

        //must be 4k aligned
        Addr v_addr;
        Addr p_addr;
        uint64_t flags; //TODO: add getters and setters? (isValid, etc)

        //Empty page table entry should be invalid
        PageTableEntry(): v_addr(-1), p_addr(-1), flags(0 & ~PT_FL_VALID) {};

        PageTableEntry(Addr v, Addr p, uint64_t flags):
            v_addr(v),
            p_addr(p),
            flags(flags)
        {}

        bool isValid() {
            return (flags & PT_FL_VALID) != 0;
        }
    }; // end class PageTableEntry =====


    private: class PageTable { //==================================================
        public:
            void mapPage(Addr v_addr, Addr p_addr, uint64_t flags);
            void unmapPage(Addr v_addr, uint64_t flags);
            PageTableEntry lookup(Addr v_addr);

        private:
            std::map<MemHierarchy::Addr, PageTableEntry> PTE_map;
    }; // end class PageTable ====
};

} //namespace SambaComponent
} //namespace SST



#endif /* PAGETABLE_H */
