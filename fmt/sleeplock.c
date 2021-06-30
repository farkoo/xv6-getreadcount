4600 // Sleeping locks
4601 
4602 #include "types.h"
4603 #include "defs.h"
4604 #include "param.h"
4605 #include "x86.h"
4606 #include "memlayout.h"
4607 #include "mmu.h"
4608 #include "proc.h"
4609 #include "spinlock.h"
4610 #include "sleeplock.h"
4611 
4612 void
4613 initsleeplock(struct sleeplock *lk, char *name)
4614 {
4615   initlock(&lk->lk, "sleep lock");
4616   lk->name = name;
4617   lk->locked = 0;
4618   lk->pid = 0;
4619 }
4620 
4621 void
4622 acquiresleep(struct sleeplock *lk)
4623 {
4624   acquire(&lk->lk);
4625   while (lk->locked) {
4626     sleep(lk, &lk->lk);
4627   }
4628   lk->locked = 1;
4629   lk->pid = myproc()->pid;
4630   release(&lk->lk);
4631 }
4632 
4633 void
4634 releasesleep(struct sleeplock *lk)
4635 {
4636   acquire(&lk->lk);
4637   lk->locked = 0;
4638   lk->pid = 0;
4639   wakeup(lk);
4640   release(&lk->lk);
4641 }
4642 
4643 
4644 
4645 
4646 
4647 
4648 
4649 
4650 int
4651 holdingsleep(struct sleeplock *lk)
4652 {
4653   int r;
4654 
4655   acquire(&lk->lk);
4656   r = lk->locked && (lk->pid == myproc()->pid);
4657   release(&lk->lk);
4658   return r;
4659 }
4660 
4661 
4662 
4663 
4664 
4665 
4666 
4667 
4668 
4669 
4670 
4671 
4672 
4673 
4674 
4675 
4676 
4677 
4678 
4679 
4680 
4681 
4682 
4683 
4684 
4685 
4686 
4687 
4688 
4689 
4690 
4691 
4692 
4693 
4694 
4695 
4696 
4697 
4698 
4699 
