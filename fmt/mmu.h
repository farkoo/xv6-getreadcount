0700 // This file contains definitions for the
0701 // x86 memory management unit (MMU).
0702 
0703 // Eflags register
0704 #define FL_IF           0x00000200      // Interrupt Enable
0705 
0706 // Control Register flags
0707 #define CR0_PE          0x00000001      // Protection Enable
0708 #define CR0_WP          0x00010000      // Write Protect
0709 #define CR0_PG          0x80000000      // Paging
0710 
0711 #define CR4_PSE         0x00000010      // Page size extension
0712 
0713 // various segment selectors.
0714 #define SEG_KCODE 1  // kernel code
0715 #define SEG_KDATA 2  // kernel data+stack
0716 #define SEG_UCODE 3  // user code
0717 #define SEG_UDATA 4  // user data+stack
0718 #define SEG_TSS   5  // this process's task state
0719 
0720 // cpu->gdt[NSEGS] holds the above segments.
0721 #define NSEGS     6
0722 
0723 #ifndef __ASSEMBLER__
0724 // Segment Descriptor
0725 struct segdesc {
0726   uint lim_15_0 : 16;  // Low bits of segment limit
0727   uint base_15_0 : 16; // Low bits of segment base address
0728   uint base_23_16 : 8; // Middle bits of segment base address
0729   uint type : 4;       // Segment type (see STS_ constants)
0730   uint s : 1;          // 0 = system, 1 = application
0731   uint dpl : 2;        // Descriptor Privilege Level
0732   uint p : 1;          // Present
0733   uint lim_19_16 : 4;  // High bits of segment limit
0734   uint avl : 1;        // Unused (available for software use)
0735   uint rsv1 : 1;       // Reserved
0736   uint db : 1;         // 0 = 16-bit segment, 1 = 32-bit segment
0737   uint g : 1;          // Granularity: limit scaled by 4K when set
0738   uint base_31_24 : 8; // High bits of segment base address
0739 };
0740 
0741 
0742 
0743 
0744 
0745 
0746 
0747 
0748 
0749 
0750 // Normal segment
0751 #define SEG(type, base, lim, dpl) (struct segdesc)    \
0752 { ((lim) >> 12) & 0xffff, (uint)(base) & 0xffff,      \
0753   ((uint)(base) >> 16) & 0xff, type, 1, dpl, 1,       \
0754   (uint)(lim) >> 28, 0, 0, 1, 1, (uint)(base) >> 24 }
0755 #define SEG16(type, base, lim, dpl) (struct segdesc)  \
0756 { (lim) & 0xffff, (uint)(base) & 0xffff,              \
0757   ((uint)(base) >> 16) & 0xff, type, 1, dpl, 1,       \
0758   (uint)(lim) >> 16, 0, 0, 1, 0, (uint)(base) >> 24 }
0759 #endif
0760 
0761 #define DPL_USER    0x3     // User DPL
0762 
0763 // Application segment type bits
0764 #define STA_X       0x8     // Executable segment
0765 #define STA_W       0x2     // Writeable (non-executable segments)
0766 #define STA_R       0x2     // Readable (executable segments)
0767 
0768 // System segment type bits
0769 #define STS_T32A    0x9     // Available 32-bit TSS
0770 #define STS_IG32    0xE     // 32-bit Interrupt Gate
0771 #define STS_TG32    0xF     // 32-bit Trap Gate
0772 
0773 // A virtual address 'la' has a three-part structure as follows:
0774 //
0775 // +--------10------+-------10-------+---------12----------+
0776 // | Page Directory |   Page Table   | Offset within Page  |
0777 // |      Index     |      Index     |                     |
0778 // +----------------+----------------+---------------------+
0779 //  \--- PDX(va) --/ \--- PTX(va) --/
0780 
0781 // page directory index
0782 #define PDX(va)         (((uint)(va) >> PDXSHIFT) & 0x3FF)
0783 
0784 // page table index
0785 #define PTX(va)         (((uint)(va) >> PTXSHIFT) & 0x3FF)
0786 
0787 // construct virtual address from indexes and offset
0788 #define PGADDR(d, t, o) ((uint)((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))
0789 
0790 // Page directory and page table constants.
0791 #define NPDENTRIES      1024    // # directory entries per page directory
0792 #define NPTENTRIES      1024    // # PTEs per page table
0793 #define PGSIZE          4096    // bytes mapped by a page
0794 
0795 #define PTXSHIFT        12      // offset of PTX in a linear address
0796 #define PDXSHIFT        22      // offset of PDX in a linear address
0797 
0798 #define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
0799 #define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))
0800 // Page table/directory entry flags.
0801 #define PTE_P           0x001   // Present
0802 #define PTE_W           0x002   // Writeable
0803 #define PTE_U           0x004   // User
0804 #define PTE_PS          0x080   // Page Size
0805 
0806 // Address in page table or page directory entry
0807 #define PTE_ADDR(pte)   ((uint)(pte) & ~0xFFF)
0808 #define PTE_FLAGS(pte)  ((uint)(pte) &  0xFFF)
0809 
0810 #ifndef __ASSEMBLER__
0811 typedef uint pte_t;
0812 
0813 // Task state segment format
0814 struct taskstate {
0815   uint link;         // Old ts selector
0816   uint esp0;         // Stack pointers and segment selectors
0817   ushort ss0;        //   after an increase in privilege level
0818   ushort padding1;
0819   uint *esp1;
0820   ushort ss1;
0821   ushort padding2;
0822   uint *esp2;
0823   ushort ss2;
0824   ushort padding3;
0825   void *cr3;         // Page directory base
0826   uint *eip;         // Saved state from last task switch
0827   uint eflags;
0828   uint eax;          // More saved state (registers)
0829   uint ecx;
0830   uint edx;
0831   uint ebx;
0832   uint *esp;
0833   uint *ebp;
0834   uint esi;
0835   uint edi;
0836   ushort es;         // Even more saved state (segment selectors)
0837   ushort padding4;
0838   ushort cs;
0839   ushort padding5;
0840   ushort ss;
0841   ushort padding6;
0842   ushort ds;
0843   ushort padding7;
0844   ushort fs;
0845   ushort padding8;
0846   ushort gs;
0847   ushort padding9;
0848   ushort ldt;
0849   ushort padding10;
0850   ushort t;          // Trap on task switch
0851   ushort iomb;       // I/O map base address
0852 };
0853 
0854 // Gate descriptors for interrupts and traps
0855 struct gatedesc {
0856   uint off_15_0 : 16;   // low 16 bits of offset in segment
0857   uint cs : 16;         // code segment selector
0858   uint args : 5;        // # args, 0 for interrupt/trap gates
0859   uint rsv1 : 3;        // reserved(should be zero I guess)
0860   uint type : 4;        // type(STS_{IG32,TG32})
0861   uint s : 1;           // must be 0 (system)
0862   uint dpl : 2;         // descriptor(meaning new) privilege level
0863   uint p : 1;           // Present
0864   uint off_31_16 : 16;  // high bits of offset in segment
0865 };
0866 
0867 // Set up a normal interrupt/trap gate descriptor.
0868 // - istrap: 1 for a trap (= exception) gate, 0 for an interrupt gate.
0869 //   interrupt gate clears FL_IF, trap gate leaves FL_IF alone
0870 // - sel: Code segment selector for interrupt/trap handler
0871 // - off: Offset in code segment for interrupt/trap handler
0872 // - dpl: Descriptor Privilege Level -
0873 //        the privilege level required for software to invoke
0874 //        this interrupt/trap gate explicitly using an int instruction.
0875 #define SETGATE(gate, istrap, sel, off, d)                \
0876 {                                                         \
0877   (gate).off_15_0 = (uint)(off) & 0xffff;                \
0878   (gate).cs = (sel);                                      \
0879   (gate).args = 0;                                        \
0880   (gate).rsv1 = 0;                                        \
0881   (gate).type = (istrap) ? STS_TG32 : STS_IG32;           \
0882   (gate).s = 0;                                           \
0883   (gate).dpl = (d);                                       \
0884   (gate).p = 1;                                           \
0885   (gate).off_31_16 = (uint)(off) >> 16;                  \
0886 }
0887 
0888 #endif
0889 
0890 
0891 
0892 
0893 
0894 
0895 
0896 
0897 
0898 
0899 
