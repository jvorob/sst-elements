import sst


# Running simpleTLB with TestOS component:
# - try using pageTableInterface in TestOS.h to communicate with tlb
#


# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "1 ns")


memory_mb = 1024 #1GB
virt_region_size_mb = 256
KB=1024
MB=1024 * KB
GB=1024 * MB



C_test_os = sst.Component("testos", "Samba.TestOS")
SC_pt_interface = C_test_os.setSubComponent("pagetable_interface", "Samba.PageTableInterface")

# Define the simulation components
C_cpu = sst.Component("cpu", "miranda.BaseCPU")
C_cpu.addParams({
	"verbose" : 1,
})
SC_cpugen = C_cpu.setSubComponent("generator", "miranda.GUPSGenerator")
SC_cpugen.addParams({
	"verbose" : 0,
	"count" : 100,
	"max_address" : str(virt_region_size_mb * MB) + "B",
})


C_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
C_l1cache.addParams({
        "access_latency_cycles" : "2",
        "cache_frequency" : "2 Ghz",
        "replacement_policy" : "lru",
        "coherence_protocol" : "MESI",
        "associativity" : "4",
        "cache_line_size" : "64",
        "prefetcher" : "cassini.StridePrefetcher",
        "L1" : "1",
        "cache_size" : "8KB",
        "debug" : "1",          
        "debug_level" : "10",    
        "debug_addresses": "[]",
        "verbose": "1",
})

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(2)
# Enable statistics outputs
C_cpu.enableAllStatistics({"type":"sst.AccumulatorStatistic"})
C_l1cache.enableAllStatistics({"type":"sst.AccumulatorStatistic"})
#comp_l1cache.enableStatistics(["GetS_recv","GetX_recv"])



# ====== Memory
C_memctrl = sst.Component("memory", "memHierarchy.MemController")
C_memctrl.addParams({
      "clock" : "1GHz"
})
SC_memory = C_memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
SC_memory.addParams({
    "access_time" : "50 ns",
    "mem_size" : str(memory_mb * 1024 * 1024) + "B",
})



# ====== Custom TLB component
C_tlb = sst.Component("TLB", "Samba.SimpleTLB")
C_tlb.addParams({
    "verbose": 3,
    "fixed_mapping_va_start": "0x0",
    "fixed_mapping_pa_start": "0xF000000",
    "fixed_mapping_len": "128MB",
})

C_pagetable = sst.Component("pagetable", "Samba.SimplePageTable")





#=========== SetupLinks
# Define the simulation links
#link_cpu_mmu_link = sst.Link("link_cpu_mmu_link")
#link_cpu_mmu_link.connect( (comp_cpu, "cache_link", "50ps"), (mmu, "cpu_to_mmu0", "50ps") )
#
#link_mmu_cache_link = sst.Link("link_mmu_cache_link")
#link_mmu_cache_link.connect( (mmu, "mmu_to_cache0", "50ps"), (comp_l1cache, "high_network_0", "50ps") )


def makeLink(name, portA, portB):
    'ports should be tuple like `(component, "port_name", "50ps")`'
    link = sst.Link(name)
    link.connect(portA,portB)
    return link;


#===== Interpose TLB between cpu and cache
makeLink("link_cpu_tlb",
        (C_cpu, "cache_link", "50ps"), (C_tlb, "high_network", "50ps"))

makeLink("link_tlb_l1cache",
        (C_tlb, "low_network", "50ps"), (C_l1cache, "high_network_0", "50ps"))


##===== Alternately, self-loop the TLB and connect cpu to cache
#makeLink("link_cpu_l1cache",
#        (C_cpu, "cache_link", "50ps"), (C_l1cache, "high_network_0", "50ps"))
#
#makeLink("linkself_tlb",
#        (C_tlb, "link_high", "50ps"), (C_tlb, "link_low", "50ps"))


# === CPU to mem
makeLink("link_l1cache_membus",
        (C_l1cache, "low_network_0", "50ps"), (C_memctrl, "direct_link", "50ps"))



# ==== Link OS and TLB to pagetable
makeLink("link_os_pagetable_simpletlb",
    (SC_pt_interface, "pagetable_link", "50ps"), (C_pagetable, "link_from_os", "50ps"))

makeLink("link_tlb_pagetable",
    (C_tlb, "pagetable_link", "50ps"), (C_pagetable, "link_from_tlb", "50ps"))

