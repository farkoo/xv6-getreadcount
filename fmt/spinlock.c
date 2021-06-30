1550 // Mutual exclusion spin locks.
1551 
1552 #include "types.h"
1553 #include "defs.h"
1554 #include "param.h"
1555 #include "x86.h"
1556 #include "memlayout.h"
1557 #include "mmu.h"
1558 #include "proc.h"
1559 #include "spinlock.h"
1560 
1561 void
1562 initlock(struct spinlock *lk, char *name)
1563 {
1564   lk->name = name;
1565   lk->locked = 0;
1566   lk->cpu = 0;
1567 }
1568 
1569 // Acquire the lock.
1570 // Loops (spins) until the lock is acquired.
1571 // Holding a lock for a long time may cause
1572 // other CPUs to waste time spinning to acquire it.
1573 void
1574 acquire(struct spinlock *lk)
1575 {
1576   pushcli(); // disable interrupts to avoid deadlock.
1577   if(holding(lk))
1578     panic("acquire");
1579 
1580   // The xchg is atomic.
1581   while(xchg(&lk->locked, 1) != 0)
1582     ;
1583 
1584   // Tell the C compiler and the processor to not move loads or stores
1585   // past this point, to ensure that the critical section's memory
1586   // references happen after the lock is acquired.
1587   __sync_synchronize();
1588 
1589   // Record info about lock acquisition for debugging.
1590   lk->cpu = mycpu();
1591   getcallerpcs(&lk, lk->pcs);
1592 }
1593 
1594 
1595 
1596 
1597 
1598 
1599 
1600 // Release the lock.
1601 void
1602 release(struct spinlock *lk)
1603 {
1604   if(!holding(lk))
1605     panic("release");
1606 
1607   lk->pcs[0] = 0;
1608   lk->cpu = 0;
1609 
1610   // Tell the C compiler and the processor to not move loads or stores
1611   // past this point, to ensure that all the stores in the critical
1612   // section are visible to other cores before the lock is released.
1613   // Both the C compiler and the hardware may re-order loads and
1614   // stores; __sync_synchronize() tells them both not to.
1615   __sync_synchronize();
1616 
1617   // Release the lock, equivalent to lk->locked = 0.
1618   // This code can't use a C assignment, since it might
1619   // not be atomic. A real OS would use C atomics here.
1620   asm volatile("movl $0, %0" : "+m" (lk->locked) : );
1621 
1622   popcli();
1623 }
1624 
1625 // Record the current call stack in pcs[] by following the %ebp chain.
1626 void
1627 getcallerpcs(void *v, uint pcs[])
1628 {
1629   uint *ebp;
1630   int i;
1631 
1632   ebp = (uint*)v - 2;
1633   for(i = 0; i < 10; i++){
1634     if(ebp == 0 || ebp < (uint*)KERNBASE || ebp == (uint*)0xffffffff)
1635       break;
1636     pcs[i] = ebp[1];     // saved %eip
1637     ebp = (uint*)ebp[0]; // saved %ebp
1638   }
1639   for(; i < 10; i++)
1640     pcs[i] = 0;
1641 }
1642 
1643 
1644 
1645 
1646 
1647 
1648 
1649 
1650 // Check whether this cpu is holding the lock.
1651 int
1652 holding(struct spinlock *lock)
1653 {
1654   int r;
1655   pushcli();
1656   r = lock->locked && lock->cpu == mycpu();
1657   popcli();
1658   return r;
1659 }
1660 
1661 
1662 // Pushcli/popcli are like cli/sti except that they are matched:
1663 // it takes two popcli to undo two pushcli.  Also, if interrupts
1664 // are off, then pushcli, popcli leaves them off.
1665 
1666 void
1667 pushcli(void)
1668 {
1669   int eflags;
1670 
1671   eflags = readeflags();
1672   cli();
1673   if(mycpu()->ncli == 0)
1674     mycpu()->intena = eflags & FL_IF;
1675   mycpu()->ncli += 1;
1676 }
1677 
1678 void
1679 popcli(void)
1680 {
1681   if(readeflags()&FL_IF)
1682     panic("popcli - interruptible");
1683   if(--mycpu()->ncli < 0)
1684     panic("popcli");
1685   if(mycpu()->ncli == 0 && mycpu()->intena)
1686     sti();
1687 }
1688 
1689 
1690 
1691 
1692 
1693 
1694 
1695 
1696 
1697 
1698 
1699 
