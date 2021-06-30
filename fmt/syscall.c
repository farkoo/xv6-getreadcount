3550 #include "types.h"
3551 #include "defs.h"
3552 #include "param.h"
3553 #include "memlayout.h"
3554 #include "mmu.h"
3555 #include "proc.h"
3556 #include "x86.h"
3557 #include "syscall.h"
3558 
3559 // User code makes a system call with INT T_SYSCALL.
3560 // System call number in %eax.
3561 // Arguments on the stack, from the user call to the C
3562 // library system call function. The saved user %esp points
3563 // to a saved program counter, and then the first argument.
3564 
3565 // Fetch the int at addr from the current process.
3566 int
3567 fetchint(uint addr, int *ip)
3568 {
3569   struct proc *curproc = myproc();
3570 
3571   if(addr >= curproc->sz || addr+4 > curproc->sz)
3572     return -1;
3573   *ip = *(int*)(addr);
3574   return 0;
3575 }
3576 
3577 // Fetch the nul-terminated string at addr from the current process.
3578 // Doesn't actually copy the string - just sets *pp to point at it.
3579 // Returns length of string, not including nul.
3580 int
3581 fetchstr(uint addr, char **pp)
3582 {
3583   char *s, *ep;
3584   struct proc *curproc = myproc();
3585 
3586   if(addr >= curproc->sz)
3587     return -1;
3588   *pp = (char*)addr;
3589   ep = (char*)curproc->sz;
3590   for(s = *pp; s < ep; s++){
3591     if(*s == 0)
3592       return s - *pp;
3593   }
3594   return -1;
3595 }
3596 
3597 
3598 
3599 
3600 // Fetch the nth 32-bit system call argument.
3601 int
3602 argint(int n, int *ip)
3603 {
3604   return fetchint((myproc()->tf->esp) + 4 + 4*n, ip);
3605 }
3606 
3607 // Fetch the nth word-sized system call argument as a pointer
3608 // to a block of memory of size bytes.  Check that the pointer
3609 // lies within the process address space.
3610 int
3611 argptr(int n, char **pp, int size)
3612 {
3613   int i;
3614   struct proc *curproc = myproc();
3615 
3616   if(argint(n, &i) < 0)
3617     return -1;
3618   if(size < 0 || (uint)i >= curproc->sz || (uint)i+size > curproc->sz)
3619     return -1;
3620   *pp = (char*)i;
3621   return 0;
3622 }
3623 
3624 // Fetch the nth word-sized system call argument as a string pointer.
3625 // Check that the pointer is valid and the string is nul-terminated.
3626 // (There is no shared writable memory, so the string can't change
3627 // between this check and being used by the kernel.)
3628 int
3629 argstr(int n, char **pp)
3630 {
3631   int addr;
3632   if(argint(n, &addr) < 0)
3633     return -1;
3634   return fetchstr(addr, pp);
3635 }
3636 
3637 
3638 
3639 
3640 
3641 
3642 
3643 
3644 
3645 
3646 
3647 
3648 
3649 
3650 extern int sys_chdir(void);
3651 extern int sys_close(void);
3652 extern int sys_dup(void);
3653 extern int sys_exec(void);
3654 extern int sys_exit(void);
3655 extern int sys_fork(void);
3656 extern int sys_fstat(void);
3657 extern int sys_getpid(void);
3658 extern int sys_kill(void);
3659 extern int sys_link(void);
3660 extern int sys_mkdir(void);
3661 extern int sys_mknod(void);
3662 extern int sys_open(void);
3663 extern int sys_pipe(void);
3664 extern int sys_read(void);
3665 extern int sys_sbrk(void);
3666 extern int sys_sleep(void);
3667 extern int sys_unlink(void);
3668 extern int sys_wait(void);
3669 extern int sys_write(void);
3670 extern int sys_uptime(void);
3671 
3672 static int (*syscalls[])(void) = {
3673 [SYS_fork]    sys_fork,
3674 [SYS_exit]    sys_exit,
3675 [SYS_wait]    sys_wait,
3676 [SYS_pipe]    sys_pipe,
3677 [SYS_read]    sys_read,
3678 [SYS_kill]    sys_kill,
3679 [SYS_exec]    sys_exec,
3680 [SYS_fstat]   sys_fstat,
3681 [SYS_chdir]   sys_chdir,
3682 [SYS_dup]     sys_dup,
3683 [SYS_getpid]  sys_getpid,
3684 [SYS_sbrk]    sys_sbrk,
3685 [SYS_sleep]   sys_sleep,
3686 [SYS_uptime]  sys_uptime,
3687 [SYS_open]    sys_open,
3688 [SYS_write]   sys_write,
3689 [SYS_mknod]   sys_mknod,
3690 [SYS_unlink]  sys_unlink,
3691 [SYS_link]    sys_link,
3692 [SYS_mkdir]   sys_mkdir,
3693 [SYS_close]   sys_close,
3694 };
3695 
3696 
3697 
3698 
3699 
3700 void
3701 syscall(void)
3702 {
3703   int num;
3704   struct proc *curproc = myproc();
3705 
3706   num = curproc->tf->eax;
3707   if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
3708     curproc->tf->eax = syscalls[num]();
3709   } else {
3710     cprintf("%d %s: unknown sys call %d\n",
3711             curproc->pid, curproc->name, num);
3712     curproc->tf->eax = -1;
3713   }
3714 }
3715 
3716 
3717 
3718 
3719 
3720 
3721 
3722 
3723 
3724 
3725 
3726 
3727 
3728 
3729 
3730 
3731 
3732 
3733 
3734 
3735 
3736 
3737 
3738 
3739 
3740 
3741 
3742 
3743 
3744 
3745 
3746 
3747 
3748 
3749 
