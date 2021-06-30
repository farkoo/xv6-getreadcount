1000 # The xv6 kernel starts executing in this file. This file is linked with
1001 # the kernel C code, so it can refer to kernel symbols such as main().
1002 # The boot block (bootasm.S and bootmain.c) jumps to entry below.
1003 
1004 # Multiboot header, for multiboot boot loaders like GNU Grub.
1005 # http://www.gnu.org/software/grub/manual/multiboot/multiboot.html
1006 #
1007 # Using GRUB 2, you can boot xv6 from a file stored in a
1008 # Linux file system by copying kernel or kernelmemfs to /boot
1009 # and then adding this menu entry:
1010 #
1011 # menuentry "xv6" {
1012 # 	insmod ext2
1013 # 	set root='(hd0,msdos1)'
1014 # 	set kernel='/boot/kernel'
1015 # 	echo "Loading ${kernel}..."
1016 # 	multiboot ${kernel} ${kernel}
1017 # 	boot
1018 # }
1019 
1020 #include "asm.h"
1021 #include "memlayout.h"
1022 #include "mmu.h"
1023 #include "param.h"
1024 
1025 # Multiboot header.  Data to direct multiboot loader.
1026 .p2align 2
1027 .text
1028 .globl multiboot_header
1029 multiboot_header:
1030   #define magic 0x1badb002
1031   #define flags 0
1032   .long magic
1033   .long flags
1034   .long (-magic-flags)
1035 
1036 # By convention, the _start symbol specifies the ELF entry point.
1037 # Since we haven't set up virtual memory yet, our entry point is
1038 # the physical address of 'entry'.
1039 .globl _start
1040 _start = V2P_WO(entry)
1041 
1042 # Entering xv6 on boot processor, with paging off.
1043 .globl entry
1044 entry:
1045   # Turn on page size extension for 4Mbyte pages
1046   movl    %cr4, %eax
1047   orl     $(CR4_PSE), %eax
1048   movl    %eax, %cr4
1049   # Set page directory
1050   movl    $(V2P_WO(entrypgdir)), %eax
1051   movl    %eax, %cr3
1052   # Turn on paging.
1053   movl    %cr0, %eax
1054   orl     $(CR0_PG|CR0_WP), %eax
1055   movl    %eax, %cr0
1056 
1057   # Set up the stack pointer.
1058   movl $(stack + KSTACKSIZE), %esp
1059 
1060   # Jump to main(), and switch to executing at
1061   # high addresses. The indirect call is needed because
1062   # the assembler produces a PC-relative instruction
1063   # for a direct jump.
1064   mov $main, %eax
1065   jmp *%eax
1066 
1067 .comm stack, KSTACKSIZE
1068 
1069 
1070 
1071 
1072 
1073 
1074 
1075 
1076 
1077 
1078 
1079 
1080 
1081 
1082 
1083 
1084 
1085 
1086 
1087 
1088 
1089 
1090 
1091 
1092 
1093 
1094 
1095 
1096 
1097 
1098 
1099 
