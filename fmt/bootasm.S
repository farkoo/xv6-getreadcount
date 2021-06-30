9100 #include "asm.h"
9101 #include "memlayout.h"
9102 #include "mmu.h"
9103 
9104 # Start the first CPU: switch to 32-bit protected mode, jump into C.
9105 # The BIOS loads this code from the first sector of the hard disk into
9106 # memory at physical address 0x7c00 and starts executing in real mode
9107 # with %cs=0 %ip=7c00.
9108 
9109 .code16                       # Assemble for 16-bit mode
9110 .globl start
9111 start:
9112   cli                         # BIOS enabled interrupts; disable
9113 
9114   # Zero data segment registers DS, ES, and SS.
9115   xorw    %ax,%ax             # Set %ax to zero
9116   movw    %ax,%ds             # -> Data Segment
9117   movw    %ax,%es             # -> Extra Segment
9118   movw    %ax,%ss             # -> Stack Segment
9119 
9120   # Physical address line A20 is tied to zero so that the first PCs
9121   # with 2 MB would run software that assumed 1 MB.  Undo that.
9122 seta20.1:
9123   inb     $0x64,%al               # Wait for not busy
9124   testb   $0x2,%al
9125   jnz     seta20.1
9126 
9127   movb    $0xd1,%al               # 0xd1 -> port 0x64
9128   outb    %al,$0x64
9129 
9130 seta20.2:
9131   inb     $0x64,%al               # Wait for not busy
9132   testb   $0x2,%al
9133   jnz     seta20.2
9134 
9135   movb    $0xdf,%al               # 0xdf -> port 0x60
9136   outb    %al,$0x60
9137 
9138   # Switch from real to protected mode.  Use a bootstrap GDT that makes
9139   # virtual addresses map directly to physical addresses so that the
9140   # effective memory map doesn't change during the transition.
9141   lgdt    gdtdesc
9142   movl    %cr0, %eax
9143   orl     $CR0_PE, %eax
9144   movl    %eax, %cr0
9145 
9146 
9147 
9148 
9149 
9150   # Complete the transition to 32-bit protected mode by using a long jmp
9151   # to reload %cs and %eip.  The segment descriptors are set up with no
9152   # translation, so that the mapping is still the identity mapping.
9153   ljmp    $(SEG_KCODE<<3), $start32
9154 
9155 .code32  # Tell assembler to generate 32-bit code now.
9156 start32:
9157   # Set up the protected-mode data segment registers
9158   movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
9159   movw    %ax, %ds                # -> DS: Data Segment
9160   movw    %ax, %es                # -> ES: Extra Segment
9161   movw    %ax, %ss                # -> SS: Stack Segment
9162   movw    $0, %ax                 # Zero segments not ready for use
9163   movw    %ax, %fs                # -> FS
9164   movw    %ax, %gs                # -> GS
9165 
9166   # Set up the stack pointer and call into C.
9167   movl    $start, %esp
9168   call    bootmain
9169 
9170   # If bootmain returns (it shouldn't), trigger a Bochs
9171   # breakpoint if running under Bochs, then loop.
9172   movw    $0x8a00, %ax            # 0x8a00 -> port 0x8a00
9173   movw    %ax, %dx
9174   outw    %ax, %dx
9175   movw    $0x8ae0, %ax            # 0x8ae0 -> port 0x8a00
9176   outw    %ax, %dx
9177 spin:
9178   jmp     spin
9179 
9180 # Bootstrap GDT
9181 .p2align 2                                # force 4 byte alignment
9182 gdt:
9183   SEG_NULLASM                             # null seg
9184   SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)   # code seg
9185   SEG_ASM(STA_W, 0x0, 0xffffffff)         # data seg
9186 
9187 gdtdesc:
9188   .word   (gdtdesc - gdt - 1)             # sizeof(gdt) - 1
9189   .long   gdt                             # address gdt
9190 
9191 
9192 
9193 
9194 
9195 
9196 
9197 
9198 
9199 
