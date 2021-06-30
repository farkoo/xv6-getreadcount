4200 // Simple PIO-based (non-DMA) IDE driver code.
4201 
4202 #include "types.h"
4203 #include "defs.h"
4204 #include "param.h"
4205 #include "memlayout.h"
4206 #include "mmu.h"
4207 #include "proc.h"
4208 #include "x86.h"
4209 #include "traps.h"
4210 #include "spinlock.h"
4211 #include "sleeplock.h"
4212 #include "fs.h"
4213 #include "buf.h"
4214 
4215 #define SECTOR_SIZE   512
4216 #define IDE_BSY       0x80
4217 #define IDE_DRDY      0x40
4218 #define IDE_DF        0x20
4219 #define IDE_ERR       0x01
4220 
4221 #define IDE_CMD_READ  0x20
4222 #define IDE_CMD_WRITE 0x30
4223 #define IDE_CMD_RDMUL 0xc4
4224 #define IDE_CMD_WRMUL 0xc5
4225 
4226 // idequeue points to the buf now being read/written to the disk.
4227 // idequeue->qnext points to the next buf to be processed.
4228 // You must hold idelock while manipulating queue.
4229 
4230 static struct spinlock idelock;
4231 static struct buf *idequeue;
4232 
4233 static int havedisk1;
4234 static void idestart(struct buf*);
4235 
4236 // Wait for IDE disk to become ready.
4237 static int
4238 idewait(int checkerr)
4239 {
4240   int r;
4241 
4242   while(((r = inb(0x1f7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
4243     ;
4244   if(checkerr && (r & (IDE_DF|IDE_ERR)) != 0)
4245     return -1;
4246   return 0;
4247 }
4248 
4249 
4250 void
4251 ideinit(void)
4252 {
4253   int i;
4254 
4255   initlock(&idelock, "ide");
4256   ioapicenable(IRQ_IDE, ncpu - 1);
4257   idewait(0);
4258 
4259   // Check if disk 1 is present
4260   outb(0x1f6, 0xe0 | (1<<4));
4261   for(i=0; i<1000; i++){
4262     if(inb(0x1f7) != 0){
4263       havedisk1 = 1;
4264       break;
4265     }
4266   }
4267 
4268   // Switch back to disk 0.
4269   outb(0x1f6, 0xe0 | (0<<4));
4270 }
4271 
4272 // Start the request for b.  Caller must hold idelock.
4273 static void
4274 idestart(struct buf *b)
4275 {
4276   if(b == 0)
4277     panic("idestart");
4278   if(b->blockno >= FSSIZE)
4279     panic("incorrect blockno");
4280   int sector_per_block =  BSIZE/SECTOR_SIZE;
4281   int sector = b->blockno * sector_per_block;
4282   int read_cmd = (sector_per_block == 1) ? IDE_CMD_READ :  IDE_CMD_RDMUL;
4283   int write_cmd = (sector_per_block == 1) ? IDE_CMD_WRITE : IDE_CMD_WRMUL;
4284 
4285   if (sector_per_block > 7) panic("idestart");
4286 
4287   idewait(0);
4288   outb(0x3f6, 0);  // generate interrupt
4289   outb(0x1f2, sector_per_block);  // number of sectors
4290   outb(0x1f3, sector & 0xff);
4291   outb(0x1f4, (sector >> 8) & 0xff);
4292   outb(0x1f5, (sector >> 16) & 0xff);
4293   outb(0x1f6, 0xe0 | ((b->dev&1)<<4) | ((sector>>24)&0x0f));
4294   if(b->flags & B_DIRTY){
4295     outb(0x1f7, write_cmd);
4296     outsl(0x1f0, b->data, BSIZE/4);
4297   } else {
4298     outb(0x1f7, read_cmd);
4299   }
4300 }
4301 
4302 // Interrupt handler.
4303 void
4304 ideintr(void)
4305 {
4306   struct buf *b;
4307 
4308   // First queued buffer is the active request.
4309   acquire(&idelock);
4310 
4311   if((b = idequeue) == 0){
4312     release(&idelock);
4313     return;
4314   }
4315   idequeue = b->qnext;
4316 
4317   // Read data if needed.
4318   if(!(b->flags & B_DIRTY) && idewait(1) >= 0)
4319     insl(0x1f0, b->data, BSIZE/4);
4320 
4321   // Wake process waiting for this buf.
4322   b->flags |= B_VALID;
4323   b->flags &= ~B_DIRTY;
4324   wakeup(b);
4325 
4326   // Start disk on next buf in queue.
4327   if(idequeue != 0)
4328     idestart(idequeue);
4329 
4330   release(&idelock);
4331 }
4332 
4333 
4334 
4335 
4336 
4337 
4338 
4339 
4340 
4341 
4342 
4343 
4344 
4345 
4346 
4347 
4348 
4349 
4350 // Sync buf with disk.
4351 // If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
4352 // Else if B_VALID is not set, read buf from disk, set B_VALID.
4353 void
4354 iderw(struct buf *b)
4355 {
4356   struct buf **pp;
4357 
4358   if(!holdingsleep(&b->lock))
4359     panic("iderw: buf not locked");
4360   if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
4361     panic("iderw: nothing to do");
4362   if(b->dev != 0 && !havedisk1)
4363     panic("iderw: ide disk 1 not present");
4364 
4365   acquire(&idelock);  //DOC:acquire-lock
4366 
4367   // Append b to idequeue.
4368   b->qnext = 0;
4369   for(pp=&idequeue; *pp; pp=&(*pp)->qnext)  //DOC:insert-queue
4370     ;
4371   *pp = b;
4372 
4373   // Start disk if necessary.
4374   if(idequeue == b)
4375     idestart(b);
4376 
4377   // Wait for request to finish.
4378   while((b->flags & (B_VALID|B_DIRTY)) != B_VALID){
4379     sleep(b, &idelock);
4380   }
4381 
4382 
4383   release(&idelock);
4384 }
4385 
4386 
4387 
4388 
4389 
4390 
4391 
4392 
4393 
4394 
4395 
4396 
4397 
4398 
4399 
