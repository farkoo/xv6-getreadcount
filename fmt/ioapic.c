7600 // The I/O APIC manages hardware interrupts for an SMP system.
7601 // http://www.intel.com/design/chipsets/datashts/29056601.pdf
7602 // See also picirq.c.
7603 
7604 #include "types.h"
7605 #include "defs.h"
7606 #include "traps.h"
7607 
7608 #define IOAPIC  0xFEC00000   // Default physical address of IO APIC
7609 
7610 #define REG_ID     0x00  // Register index: ID
7611 #define REG_VER    0x01  // Register index: version
7612 #define REG_TABLE  0x10  // Redirection table base
7613 
7614 // The redirection table starts at REG_TABLE and uses
7615 // two registers to configure each interrupt.
7616 // The first (low) register in a pair contains configuration bits.
7617 // The second (high) register contains a bitmask telling which
7618 // CPUs can serve that interrupt.
7619 #define INT_DISABLED   0x00010000  // Interrupt disabled
7620 #define INT_LEVEL      0x00008000  // Level-triggered (vs edge-)
7621 #define INT_ACTIVELOW  0x00002000  // Active low (vs high)
7622 #define INT_LOGICAL    0x00000800  // Destination is CPU id (vs APIC ID)
7623 
7624 volatile struct ioapic *ioapic;
7625 
7626 // IO APIC MMIO structure: write reg, then read or write data.
7627 struct ioapic {
7628   uint reg;
7629   uint pad[3];
7630   uint data;
7631 };
7632 
7633 static uint
7634 ioapicread(int reg)
7635 {
7636   ioapic->reg = reg;
7637   return ioapic->data;
7638 }
7639 
7640 static void
7641 ioapicwrite(int reg, uint data)
7642 {
7643   ioapic->reg = reg;
7644   ioapic->data = data;
7645 }
7646 
7647 
7648 
7649 
7650 void
7651 ioapicinit(void)
7652 {
7653   int i, id, maxintr;
7654 
7655   ioapic = (volatile struct ioapic*)IOAPIC;
7656   maxintr = (ioapicread(REG_VER) >> 16) & 0xFF;
7657   id = ioapicread(REG_ID) >> 24;
7658   if(id != ioapicid)
7659     cprintf("ioapicinit: id isn't equal to ioapicid; not a MP\n");
7660 
7661   // Mark all interrupts edge-triggered, active high, disabled,
7662   // and not routed to any CPUs.
7663   for(i = 0; i <= maxintr; i++){
7664     ioapicwrite(REG_TABLE+2*i, INT_DISABLED | (T_IRQ0 + i));
7665     ioapicwrite(REG_TABLE+2*i+1, 0);
7666   }
7667 }
7668 
7669 void
7670 ioapicenable(int irq, int cpunum)
7671 {
7672   // Mark interrupt edge-triggered, active high,
7673   // enabled, and routed to the given cpunum,
7674   // which happens to be that cpu's APIC ID.
7675   ioapicwrite(REG_TABLE+2*irq, T_IRQ0 + irq);
7676   ioapicwrite(REG_TABLE+2*irq+1, cpunum << 24);
7677 }
7678 
7679 
7680 
7681 
7682 
7683 
7684 
7685 
7686 
7687 
7688 
7689 
7690 
7691 
7692 
7693 
7694 
7695 
7696 
7697 
7698 
7699 
