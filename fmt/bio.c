4400 // Buffer cache.
4401 //
4402 // The buffer cache is a linked list of buf structures holding
4403 // cached copies of disk block contents.  Caching disk blocks
4404 // in memory reduces the number of disk reads and also provides
4405 // a synchronization point for disk blocks used by multiple processes.
4406 //
4407 // Interface:
4408 // * To get a buffer for a particular disk block, call bread.
4409 // * After changing buffer data, call bwrite to write it to disk.
4410 // * When done with the buffer, call brelse.
4411 // * Do not use the buffer after calling brelse.
4412 // * Only one process at a time can use a buffer,
4413 //     so do not keep them longer than necessary.
4414 //
4415 // The implementation uses two state flags internally:
4416 // * B_VALID: the buffer data has been read from the disk.
4417 // * B_DIRTY: the buffer data has been modified
4418 //     and needs to be written to disk.
4419 
4420 #include "types.h"
4421 #include "defs.h"
4422 #include "param.h"
4423 #include "spinlock.h"
4424 #include "sleeplock.h"
4425 #include "fs.h"
4426 #include "buf.h"
4427 
4428 struct {
4429   struct spinlock lock;
4430   struct buf buf[NBUF];
4431 
4432   // Linked list of all buffers, through prev/next.
4433   // head.next is most recently used.
4434   struct buf head;
4435 } bcache;
4436 
4437 void
4438 binit(void)
4439 {
4440   struct buf *b;
4441 
4442   initlock(&bcache.lock, "bcache");
4443 
4444 
4445 
4446 
4447 
4448 
4449 
4450   // Create linked list of buffers
4451   bcache.head.prev = &bcache.head;
4452   bcache.head.next = &bcache.head;
4453   for(b = bcache.buf; b < bcache.buf+NBUF; b++){
4454     b->next = bcache.head.next;
4455     b->prev = &bcache.head;
4456     initsleeplock(&b->lock, "buffer");
4457     bcache.head.next->prev = b;
4458     bcache.head.next = b;
4459   }
4460 }
4461 
4462 // Look through buffer cache for block on device dev.
4463 // If not found, allocate a buffer.
4464 // In either case, return locked buffer.
4465 static struct buf*
4466 bget(uint dev, uint blockno)
4467 {
4468   struct buf *b;
4469 
4470   acquire(&bcache.lock);
4471 
4472   // Is the block already cached?
4473   for(b = bcache.head.next; b != &bcache.head; b = b->next){
4474     if(b->dev == dev && b->blockno == blockno){
4475       b->refcnt++;
4476       release(&bcache.lock);
4477       acquiresleep(&b->lock);
4478       return b;
4479     }
4480   }
4481 
4482   // Not cached; recycle an unused buffer.
4483   // Even if refcnt==0, B_DIRTY indicates a buffer is in use
4484   // because log.c has modified it but not yet committed it.
4485   for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
4486     if(b->refcnt == 0 && (b->flags & B_DIRTY) == 0) {
4487       b->dev = dev;
4488       b->blockno = blockno;
4489       b->flags = 0;
4490       b->refcnt = 1;
4491       release(&bcache.lock);
4492       acquiresleep(&b->lock);
4493       return b;
4494     }
4495   }
4496   panic("bget: no buffers");
4497 }
4498 
4499 
4500 // Return a locked buf with the contents of the indicated block.
4501 struct buf*
4502 bread(uint dev, uint blockno)
4503 {
4504   struct buf *b;
4505 
4506   b = bget(dev, blockno);
4507   if((b->flags & B_VALID) == 0) {
4508     iderw(b);
4509   }
4510   return b;
4511 }
4512 
4513 // Write b's contents to disk.  Must be locked.
4514 void
4515 bwrite(struct buf *b)
4516 {
4517   if(!holdingsleep(&b->lock))
4518     panic("bwrite");
4519   b->flags |= B_DIRTY;
4520   iderw(b);
4521 }
4522 
4523 // Release a locked buffer.
4524 // Move to the head of the MRU list.
4525 void
4526 brelse(struct buf *b)
4527 {
4528   if(!holdingsleep(&b->lock))
4529     panic("brelse");
4530 
4531   releasesleep(&b->lock);
4532 
4533   acquire(&bcache.lock);
4534   b->refcnt--;
4535   if (b->refcnt == 0) {
4536     // no one is waiting for it.
4537     b->next->prev = b->prev;
4538     b->prev->next = b->next;
4539     b->next = bcache.head.next;
4540     b->prev = &bcache.head;
4541     bcache.head.next->prev = b;
4542     bcache.head.next = b;
4543   }
4544 
4545   release(&bcache.lock);
4546 }
4547 
4548 
4549 
4550 // Blank page.
4551 
4552 
4553 
4554 
4555 
4556 
4557 
4558 
4559 
4560 
4561 
4562 
4563 
4564 
4565 
4566 
4567 
4568 
4569 
4570 
4571 
4572 
4573 
4574 
4575 
4576 
4577 
4578 
4579 
4580 
4581 
4582 
4583 
4584 
4585 
4586 
4587 
4588 
4589 
4590 
4591 
4592 
4593 
4594 
4595 
4596 
4597 
4598 
4599 
