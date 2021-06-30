3300 #include "mmu.h"
3301 
3302   # vectors.S sends all traps here.
3303 .globl alltraps
3304 alltraps:
3305   # Build trap frame.
3306   pushl %ds
3307   pushl %es
3308   pushl %fs
3309   pushl %gs
3310   pushal
3311 
3312   # Set up data segments.
3313   movw $(SEG_KDATA<<3), %ax
3314   movw %ax, %ds
3315   movw %ax, %es
3316 
3317   # Call trap(tf), where tf=%esp
3318   pushl %esp
3319   call trap
3320   addl $4, %esp
3321 
3322   # Return falls through to trapret...
3323 .globl trapret
3324 trapret:
3325   popal
3326   popl %gs
3327   popl %fs
3328   popl %es
3329   popl %ds
3330   addl $0x8, %esp  # trapno and errcode
3331   iret
3332 
3333 
3334 
3335 
3336 
3337 
3338 
3339 
3340 
3341 
3342 
3343 
3344 
3345 
3346 
3347 
3348 
3349 
