3750 #include "types.h"
3751 #include "x86.h"
3752 #include "defs.h"
3753 #include "date.h"
3754 #include "param.h"
3755 #include "memlayout.h"
3756 #include "mmu.h"
3757 #include "proc.h"
3758 
3759 int
3760 sys_fork(void)
3761 {
3762   return fork();
3763 }
3764 
3765 int
3766 sys_exit(void)
3767 {
3768   exit();
3769   return 0;  // not reached
3770 }
3771 
3772 int
3773 sys_wait(void)
3774 {
3775   return wait();
3776 }
3777 
3778 int
3779 sys_kill(void)
3780 {
3781   int pid;
3782 
3783   if(argint(0, &pid) < 0)
3784     return -1;
3785   return kill(pid);
3786 }
3787 
3788 int
3789 sys_getpid(void)
3790 {
3791   return myproc()->pid;
3792 }
3793 
3794 
3795 
3796 
3797 
3798 
3799 
3800 int
3801 sys_sbrk(void)
3802 {
3803   int addr;
3804   int n;
3805 
3806   if(argint(0, &n) < 0)
3807     return -1;
3808   addr = myproc()->sz;
3809   if(growproc(n) < 0)
3810     return -1;
3811   return addr;
3812 }
3813 
3814 int
3815 sys_sleep(void)
3816 {
3817   int n;
3818   uint ticks0;
3819 
3820   if(argint(0, &n) < 0)
3821     return -1;
3822   acquire(&tickslock);
3823   ticks0 = ticks;
3824   while(ticks - ticks0 < n){
3825     if(myproc()->killed){
3826       release(&tickslock);
3827       return -1;
3828     }
3829     sleep(&ticks, &tickslock);
3830   }
3831   release(&tickslock);
3832   return 0;
3833 }
3834 
3835 // return how many clock tick interrupts have occurred
3836 // since start.
3837 int
3838 sys_uptime(void)
3839 {
3840   uint xticks;
3841 
3842   acquire(&tickslock);
3843   xticks = ticks;
3844   release(&tickslock);
3845   return xticks;
3846 }
3847 
3848 
3849 
