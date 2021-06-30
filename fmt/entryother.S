1100 #include "asm.h"
1101 #include "memlayout.h"
1102 #include "mmu.h"
1103 
1104 # Each non-boot CPU ("AP") is started up in response to a STARTUP
1105 # IPI from the boot CPU.  Section B.4.2 of the Multi-Processor
1106 # Specification says that the AP will start in real mode with CS:IP
1107 # set to XY00:0000, where XY is an 8-bit value sent with the
1108 # STARTUP. Thus this code must start at a 4096-byte boundary.
1109 #
1110 # Because this code sets DS to zero, it must sit
1111 # at an address in the low 2^16 bytes.
1112 #
1113 # Startothers (in main.c) sends the STARTUPs one at a time.
1114 # It copies this code (start) at 0x7000.  It puts the address of
1115 # a newly allocated per-core stack in start-4,the address of the
1116 # place to jump to (mpenter) in start-8, and the physical address
1117 # of entrypgdir in start-12.
1118 #
1119 # This code combines elements of bootasm.S and entry.S.
1120 
1121 .code16
1122 .globl start
1123 start:
1124   cli
1125 
1126   # Zero data segment registers DS, ES, and SS.
1127   xorw    %ax,%ax
1128   movw    %ax,%ds
1129   movw    %ax,%es
1130   movw    %ax,%ss
1131 
1132   # Switch from real to protected mode.  Use a bootstrap GDT that makes
1133   # virtual addresses map directly to physical addresses so that the
1134   # effective memory map doesn't change during the transition.
1135   lgdt    gdtdesc
1136   movl    %cr0, %eax
1137   orl     $CR0_PE, %eax
1138   movl    %eax, %cr0
1139 
1140   # Complete the transition to 32-bit protected mode by using a long jmp
1141   # to reload %cs and %eip.  The segment descriptors are set up with no
1142   # translation, so that the mapping is still the identity mapping.
1143   ljmpl    $(SEG_KCODE<<3), $(start32)
1144 
1145 
1146 
1147 
1148 
1149 
1150 .code32  # Tell assembler to generate 32-bit code now.
1151 start32:
1152   # Set up the protected-mode data segment registers
1153   movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
1154   movw    %ax, %ds                # -> DS: Data Segment
1155   movw    %ax, %es                # -> ES: Extra Segment
1156   movw    %ax, %ss                # -> SS: Stack Segment
1157   movw    $0, %ax                 # Zero segments not ready for use
1158   movw    %ax, %fs                # -> FS
1159   movw    %ax, %gs                # -> GS
1160 
1161   # Turn on page size extension for 4Mbyte pages
1162   movl    %cr4, %eax
1163   orl     $(CR4_PSE), %eax
1164   movl    %eax, %cr4
1165   # Use entrypgdir as our initial page table
1166   movl    (start-12), %eax
1167   movl    %eax, %cr3
1168   # Turn on paging.
1169   movl    %cr0, %eax
1170   orl     $(CR0_PE|CR0_PG|CR0_WP), %eax
1171   movl    %eax, %cr0
1172 
1173   # Switch to the stack allocated by startothers()
1174   movl    (start-4), %esp
1175   # Call mpenter()
1176   call	 *(start-8)
1177 
1178   movw    $0x8a00, %ax
1179   movw    %ax, %dx
1180   outw    %ax, %dx
1181   movw    $0x8ae0, %ax
1182   outw    %ax, %dx
1183 spin:
1184   jmp     spin
1185 
1186 .p2align 2
1187 gdt:
1188   SEG_NULLASM
1189   SEG_ASM(STA_X|STA_R, 0, 0xffffffff)
1190   SEG_ASM(STA_W, 0, 0xffffffff)
1191 
1192 
1193 gdtdesc:
1194   .word   (gdtdesc - gdt - 1)
1195   .long   gdt
1196 
1197 
1198 
1199 
