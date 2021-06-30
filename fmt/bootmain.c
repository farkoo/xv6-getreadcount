9200 // Boot loader.
9201 //
9202 // Part of the boot block, along with bootasm.S, which calls bootmain().
9203 // bootasm.S has put the processor into protected 32-bit mode.
9204 // bootmain() loads an ELF kernel image from the disk starting at
9205 // sector 1 and then jumps to the kernel entry routine.
9206 
9207 #include "types.h"
9208 #include "elf.h"
9209 #include "x86.h"
9210 #include "memlayout.h"
9211 
9212 #define SECTSIZE  512
9213 
9214 void readseg(uchar*, uint, uint);
9215 
9216 void
9217 bootmain(void)
9218 {
9219   struct elfhdr *elf;
9220   struct proghdr *ph, *eph;
9221   void (*entry)(void);
9222   uchar* pa;
9223 
9224   elf = (struct elfhdr*)0x10000;  // scratch space
9225 
9226   // Read 1st page off disk
9227   readseg((uchar*)elf, 4096, 0);
9228 
9229   // Is this an ELF executable?
9230   if(elf->magic != ELF_MAGIC)
9231     return;  // let bootasm.S handle error
9232 
9233   // Load each program segment (ignores ph flags).
9234   ph = (struct proghdr*)((uchar*)elf + elf->phoff);
9235   eph = ph + elf->phnum;
9236   for(; ph < eph; ph++){
9237     pa = (uchar*)ph->paddr;
9238     readseg(pa, ph->filesz, ph->off);
9239     if(ph->memsz > ph->filesz)
9240       stosb(pa + ph->filesz, 0, ph->memsz - ph->filesz);
9241   }
9242 
9243   // Call the entry point from the ELF header.
9244   // Does not return!
9245   entry = (void(*)(void))(elf->entry);
9246   entry();
9247 }
9248 
9249 
9250 void
9251 waitdisk(void)
9252 {
9253   // Wait for disk ready.
9254   while((inb(0x1F7) & 0xC0) != 0x40)
9255     ;
9256 }
9257 
9258 // Read a single sector at offset into dst.
9259 void
9260 readsect(void *dst, uint offset)
9261 {
9262   // Issue command.
9263   waitdisk();
9264   outb(0x1F2, 1);   // count = 1
9265   outb(0x1F3, offset);
9266   outb(0x1F4, offset >> 8);
9267   outb(0x1F5, offset >> 16);
9268   outb(0x1F6, (offset >> 24) | 0xE0);
9269   outb(0x1F7, 0x20);  // cmd 0x20 - read sectors
9270 
9271   // Read data.
9272   waitdisk();
9273   insl(0x1F0, dst, SECTSIZE/4);
9274 }
9275 
9276 // Read 'count' bytes at 'offset' from kernel into physical address 'pa'.
9277 // Might copy more than asked.
9278 void
9279 readseg(uchar* pa, uint count, uint offset)
9280 {
9281   uchar* epa;
9282 
9283   epa = pa + count;
9284 
9285   // Round down to sector boundary.
9286   pa -= offset % SECTSIZE;
9287 
9288   // Translate from bytes to sectors; kernel starts at sector 1.
9289   offset = (offset / SECTSIZE) + 1;
9290 
9291   // If this is too slow, we could read lots of sectors at a time.
9292   // We'd write more to memory than asked, but it doesn't matter --
9293   // we load in increasing order.
9294   for(; pa < epa; pa += SECTSIZE, offset++)
9295     readsect(pa, offset);
9296 }
9297 
9298 
9299 
