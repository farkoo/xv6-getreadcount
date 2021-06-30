8400 # Initial process execs /init.
8401 # This code runs in user space.
8402 
8403 #include "syscall.h"
8404 #include "traps.h"
8405 
8406 
8407 # exec(init, argv)
8408 .globl start
8409 start:
8410   pushl $argv
8411   pushl $init
8412   pushl $0  // where caller pc would be
8413   movl $SYS_exec, %eax
8414   int $T_SYSCALL
8415 
8416 # for(;;) exit();
8417 exit:
8418   movl $SYS_exit, %eax
8419   int $T_SYSCALL
8420   jmp exit
8421 
8422 # char init[] = "/init\0";
8423 init:
8424   .string "/init\0"
8425 
8426 # char *argv[] = { init, 0 };
8427 .p2align 2
8428 argv:
8429   .long init
8430   .long 0
8431 
8432 
8433 
8434 
8435 
8436 
8437 
8438 
8439 
8440 
8441 
8442 
8443 
8444 
8445 
8446 
8447 
8448 
8449 
