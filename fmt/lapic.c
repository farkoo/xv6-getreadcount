7350 // The local APIC manages internal (non-I/O) interrupts.
7351 // See Chapter 8 & Appendix C of Intel processor manual volume 3.
7352 
7353 #include "param.h"
7354 #include "types.h"
7355 #include "defs.h"
7356 #include "date.h"
7357 #include "memlayout.h"
7358 #include "traps.h"
7359 #include "mmu.h"
7360 #include "x86.h"
7361 
7362 // Local APIC registers, divided by 4 for use as uint[] indices.
7363 #define ID      (0x0020/4)   // ID
7364 #define VER     (0x0030/4)   // Version
7365 #define TPR     (0x0080/4)   // Task Priority
7366 #define EOI     (0x00B0/4)   // EOI
7367 #define SVR     (0x00F0/4)   // Spurious Interrupt Vector
7368   #define ENABLE     0x00000100   // Unit Enable
7369 #define ESR     (0x0280/4)   // Error Status
7370 #define ICRLO   (0x0300/4)   // Interrupt Command
7371   #define INIT       0x00000500   // INIT/RESET
7372   #define STARTUP    0x00000600   // Startup IPI
7373   #define DELIVS     0x00001000   // Delivery status
7374   #define ASSERT     0x00004000   // Assert interrupt (vs deassert)
7375   #define DEASSERT   0x00000000
7376   #define LEVEL      0x00008000   // Level triggered
7377   #define BCAST      0x00080000   // Send to all APICs, including self.
7378   #define BUSY       0x00001000
7379   #define FIXED      0x00000000
7380 #define ICRHI   (0x0310/4)   // Interrupt Command [63:32]
7381 #define TIMER   (0x0320/4)   // Local Vector Table 0 (TIMER)
7382   #define X1         0x0000000B   // divide counts by 1
7383   #define PERIODIC   0x00020000   // Periodic
7384 #define PCINT   (0x0340/4)   // Performance Counter LVT
7385 #define LINT0   (0x0350/4)   // Local Vector Table 1 (LINT0)
7386 #define LINT1   (0x0360/4)   // Local Vector Table 2 (LINT1)
7387 #define ERROR   (0x0370/4)   // Local Vector Table 3 (ERROR)
7388   #define MASKED     0x00010000   // Interrupt masked
7389 #define TICR    (0x0380/4)   // Timer Initial Count
7390 #define TCCR    (0x0390/4)   // Timer Current Count
7391 #define TDCR    (0x03E0/4)   // Timer Divide Configuration
7392 
7393 volatile uint *lapic;  // Initialized in mp.c
7394 
7395 
7396 
7397 
7398 
7399 
7400 static void
7401 lapicw(int index, int value)
7402 {
7403   lapic[index] = value;
7404   lapic[ID];  // wait for write to finish, by reading
7405 }
7406 
7407 void
7408 lapicinit(void)
7409 {
7410   if(!lapic)
7411     return;
7412 
7413   // Enable local APIC; set spurious interrupt vector.
7414   lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));
7415 
7416   // The timer repeatedly counts down at bus frequency
7417   // from lapic[TICR] and then issues an interrupt.
7418   // If xv6 cared more about precise timekeeping,
7419   // TICR would be calibrated using an external time source.
7420   lapicw(TDCR, X1);
7421   lapicw(TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
7422   lapicw(TICR, 10000000);
7423 
7424   // Disable logical interrupt lines.
7425   lapicw(LINT0, MASKED);
7426   lapicw(LINT1, MASKED);
7427 
7428   // Disable performance counter overflow interrupts
7429   // on machines that provide that interrupt entry.
7430   if(((lapic[VER]>>16) & 0xFF) >= 4)
7431     lapicw(PCINT, MASKED);
7432 
7433   // Map error interrupt to IRQ_ERROR.
7434   lapicw(ERROR, T_IRQ0 + IRQ_ERROR);
7435 
7436   // Clear error status register (requires back-to-back writes).
7437   lapicw(ESR, 0);
7438   lapicw(ESR, 0);
7439 
7440   // Ack any outstanding interrupts.
7441   lapicw(EOI, 0);
7442 
7443   // Send an Init Level De-Assert to synchronise arbitration ID's.
7444   lapicw(ICRHI, 0);
7445   lapicw(ICRLO, BCAST | INIT | LEVEL);
7446   while(lapic[ICRLO] & DELIVS)
7447     ;
7448 
7449 
7450   // Enable interrupts on the APIC (but not on the processor).
7451   lapicw(TPR, 0);
7452 }
7453 
7454 int
7455 lapicid(void)
7456 {
7457   if (!lapic)
7458     return 0;
7459   return lapic[ID] >> 24;
7460 }
7461 
7462 // Acknowledge interrupt.
7463 void
7464 lapiceoi(void)
7465 {
7466   if(lapic)
7467     lapicw(EOI, 0);
7468 }
7469 
7470 // Spin for a given number of microseconds.
7471 // On real hardware would want to tune this dynamically.
7472 void
7473 microdelay(int us)
7474 {
7475 }
7476 
7477 #define CMOS_PORT    0x70
7478 #define CMOS_RETURN  0x71
7479 
7480 // Start additional processor running entry code at addr.
7481 // See Appendix B of MultiProcessor Specification.
7482 void
7483 lapicstartap(uchar apicid, uint addr)
7484 {
7485   int i;
7486   ushort *wrv;
7487 
7488   // "The BSP must initialize CMOS shutdown code to 0AH
7489   // and the warm reset vector (DWORD based at 40:67) to point at
7490   // the AP startup code prior to the [universal startup algorithm]."
7491   outb(CMOS_PORT, 0xF);  // offset 0xF is shutdown code
7492   outb(CMOS_PORT+1, 0x0A);
7493   wrv = (ushort*)P2V((0x40<<4 | 0x67));  // Warm reset vector
7494   wrv[0] = 0;
7495   wrv[1] = addr >> 4;
7496 
7497 
7498 
7499 
7500   // "Universal startup algorithm."
7501   // Send INIT (level-triggered) interrupt to reset other CPU.
7502   lapicw(ICRHI, apicid<<24);
7503   lapicw(ICRLO, INIT | LEVEL | ASSERT);
7504   microdelay(200);
7505   lapicw(ICRLO, INIT | LEVEL);
7506   microdelay(100);    // should be 10ms, but too slow in Bochs!
7507 
7508   // Send startup IPI (twice!) to enter code.
7509   // Regular hardware is supposed to only accept a STARTUP
7510   // when it is in the halted state due to an INIT.  So the second
7511   // should be ignored, but it is part of the official Intel algorithm.
7512   // Bochs complains about the second one.  Too bad for Bochs.
7513   for(i = 0; i < 2; i++){
7514     lapicw(ICRHI, apicid<<24);
7515     lapicw(ICRLO, STARTUP | (addr>>12));
7516     microdelay(200);
7517   }
7518 }
7519 
7520 #define CMOS_STATA   0x0a
7521 #define CMOS_STATB   0x0b
7522 #define CMOS_UIP    (1 << 7)        // RTC update in progress
7523 
7524 #define SECS    0x00
7525 #define MINS    0x02
7526 #define HOURS   0x04
7527 #define DAY     0x07
7528 #define MONTH   0x08
7529 #define YEAR    0x09
7530 
7531 static uint
7532 cmos_read(uint reg)
7533 {
7534   outb(CMOS_PORT,  reg);
7535   microdelay(200);
7536 
7537   return inb(CMOS_RETURN);
7538 }
7539 
7540 static void
7541 fill_rtcdate(struct rtcdate *r)
7542 {
7543   r->second = cmos_read(SECS);
7544   r->minute = cmos_read(MINS);
7545   r->hour   = cmos_read(HOURS);
7546   r->day    = cmos_read(DAY);
7547   r->month  = cmos_read(MONTH);
7548   r->year   = cmos_read(YEAR);
7549 }
7550 // qemu seems to use 24-hour GWT and the values are BCD encoded
7551 void
7552 cmostime(struct rtcdate *r)
7553 {
7554   struct rtcdate t1, t2;
7555   int sb, bcd;
7556 
7557   sb = cmos_read(CMOS_STATB);
7558 
7559   bcd = (sb & (1 << 2)) == 0;
7560 
7561   // make sure CMOS doesn't modify time while we read it
7562   for(;;) {
7563     fill_rtcdate(&t1);
7564     if(cmos_read(CMOS_STATA) & CMOS_UIP)
7565         continue;
7566     fill_rtcdate(&t2);
7567     if(memcmp(&t1, &t2, sizeof(t1)) == 0)
7568       break;
7569   }
7570 
7571   // convert
7572   if(bcd) {
7573 #define    CONV(x)     (t1.x = ((t1.x >> 4) * 10) + (t1.x & 0xf))
7574     CONV(second);
7575     CONV(minute);
7576     CONV(hour  );
7577     CONV(day   );
7578     CONV(month );
7579     CONV(year  );
7580 #undef     CONV
7581   }
7582 
7583   *r = t1;
7584   r->year += 2000;
7585 }
7586 
7587 
7588 
7589 
7590 
7591 
7592 
7593 
7594 
7595 
7596 
7597 
7598 
7599 
