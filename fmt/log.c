4700 #include "types.h"
4701 #include "defs.h"
4702 #include "param.h"
4703 #include "spinlock.h"
4704 #include "sleeplock.h"
4705 #include "fs.h"
4706 #include "buf.h"
4707 
4708 // Simple logging that allows concurrent FS system calls.
4709 //
4710 // A log transaction contains the updates of multiple FS system
4711 // calls. The logging system only commits when there are
4712 // no FS system calls active. Thus there is never
4713 // any reasoning required about whether a commit might
4714 // write an uncommitted system call's updates to disk.
4715 //
4716 // A system call should call begin_op()/end_op() to mark
4717 // its start and end. Usually begin_op() just increments
4718 // the count of in-progress FS system calls and returns.
4719 // But if it thinks the log is close to running out, it
4720 // sleeps until the last outstanding end_op() commits.
4721 //
4722 // The log is a physical re-do log containing disk blocks.
4723 // The on-disk log format:
4724 //   header block, containing block #s for block A, B, C, ...
4725 //   block A
4726 //   block B
4727 //   block C
4728 //   ...
4729 // Log appends are synchronous.
4730 
4731 // Contents of the header block, used for both the on-disk header block
4732 // and to keep track in memory of logged block# before commit.
4733 struct logheader {
4734   int n;
4735   int block[LOGSIZE];
4736 };
4737 
4738 struct log {
4739   struct spinlock lock;
4740   int start;
4741   int size;
4742   int outstanding; // how many FS sys calls are executing.
4743   int committing;  // in commit(), please wait.
4744   int dev;
4745   struct logheader lh;
4746 };
4747 
4748 
4749 
4750 struct log log;
4751 
4752 static void recover_from_log(void);
4753 static void commit();
4754 
4755 void
4756 initlog(int dev)
4757 {
4758   if (sizeof(struct logheader) >= BSIZE)
4759     panic("initlog: too big logheader");
4760 
4761   struct superblock sb;
4762   initlock(&log.lock, "log");
4763   readsb(dev, &sb);
4764   log.start = sb.logstart;
4765   log.size = sb.nlog;
4766   log.dev = dev;
4767   recover_from_log();
4768 }
4769 
4770 // Copy committed blocks from log to their home location
4771 static void
4772 install_trans(void)
4773 {
4774   int tail;
4775 
4776   for (tail = 0; tail < log.lh.n; tail++) {
4777     struct buf *lbuf = bread(log.dev, log.start+tail+1); // read log block
4778     struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // read dst
4779     memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
4780     bwrite(dbuf);  // write dst to disk
4781     brelse(lbuf);
4782     brelse(dbuf);
4783   }
4784 }
4785 
4786 // Read the log header from disk into the in-memory log header
4787 static void
4788 read_head(void)
4789 {
4790   struct buf *buf = bread(log.dev, log.start);
4791   struct logheader *lh = (struct logheader *) (buf->data);
4792   int i;
4793   log.lh.n = lh->n;
4794   for (i = 0; i < log.lh.n; i++) {
4795     log.lh.block[i] = lh->block[i];
4796   }
4797   brelse(buf);
4798 }
4799 
4800 // Write in-memory log header to disk.
4801 // This is the true point at which the
4802 // current transaction commits.
4803 static void
4804 write_head(void)
4805 {
4806   struct buf *buf = bread(log.dev, log.start);
4807   struct logheader *hb = (struct logheader *) (buf->data);
4808   int i;
4809   hb->n = log.lh.n;
4810   for (i = 0; i < log.lh.n; i++) {
4811     hb->block[i] = log.lh.block[i];
4812   }
4813   bwrite(buf);
4814   brelse(buf);
4815 }
4816 
4817 static void
4818 recover_from_log(void)
4819 {
4820   read_head();
4821   install_trans(); // if committed, copy from log to disk
4822   log.lh.n = 0;
4823   write_head(); // clear the log
4824 }
4825 
4826 // called at the start of each FS system call.
4827 void
4828 begin_op(void)
4829 {
4830   acquire(&log.lock);
4831   while(1){
4832     if(log.committing){
4833       sleep(&log, &log.lock);
4834     } else if(log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGSIZE){
4835       // this op might exhaust log space; wait for commit.
4836       sleep(&log, &log.lock);
4837     } else {
4838       log.outstanding += 1;
4839       release(&log.lock);
4840       break;
4841     }
4842   }
4843 }
4844 
4845 
4846 
4847 
4848 
4849 
4850 // called at the end of each FS system call.
4851 // commits if this was the last outstanding operation.
4852 void
4853 end_op(void)
4854 {
4855   int do_commit = 0;
4856 
4857   acquire(&log.lock);
4858   log.outstanding -= 1;
4859   if(log.committing)
4860     panic("log.committing");
4861   if(log.outstanding == 0){
4862     do_commit = 1;
4863     log.committing = 1;
4864   } else {
4865     // begin_op() may be waiting for log space,
4866     // and decrementing log.outstanding has decreased
4867     // the amount of reserved space.
4868     wakeup(&log);
4869   }
4870   release(&log.lock);
4871 
4872   if(do_commit){
4873     // call commit w/o holding locks, since not allowed
4874     // to sleep with locks.
4875     commit();
4876     acquire(&log.lock);
4877     log.committing = 0;
4878     wakeup(&log);
4879     release(&log.lock);
4880   }
4881 }
4882 
4883 // Copy modified blocks from cache to log.
4884 static void
4885 write_log(void)
4886 {
4887   int tail;
4888 
4889   for (tail = 0; tail < log.lh.n; tail++) {
4890     struct buf *to = bread(log.dev, log.start+tail+1); // log block
4891     struct buf *from = bread(log.dev, log.lh.block[tail]); // cache block
4892     memmove(to->data, from->data, BSIZE);
4893     bwrite(to);  // write the log
4894     brelse(from);
4895     brelse(to);
4896   }
4897 }
4898 
4899 
4900 static void
4901 commit()
4902 {
4903   if (log.lh.n > 0) {
4904     write_log();     // Write modified blocks from cache to log
4905     write_head();    // Write header to disk -- the real commit
4906     install_trans(); // Now install writes to home locations
4907     log.lh.n = 0;
4908     write_head();    // Erase the transaction from the log
4909   }
4910 }
4911 
4912 // Caller has modified b->data and is done with the buffer.
4913 // Record the block number and pin in the cache with B_DIRTY.
4914 // commit()/write_log() will do the disk write.
4915 //
4916 // log_write() replaces bwrite(); a typical use is:
4917 //   bp = bread(...)
4918 //   modify bp->data[]
4919 //   log_write(bp)
4920 //   brelse(bp)
4921 void
4922 log_write(struct buf *b)
4923 {
4924   int i;
4925 
4926   if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
4927     panic("too big a transaction");
4928   if (log.outstanding < 1)
4929     panic("log_write outside of trans");
4930 
4931   acquire(&log.lock);
4932   for (i = 0; i < log.lh.n; i++) {
4933     if (log.lh.block[i] == b->blockno)   // log absorbtion
4934       break;
4935   }
4936   log.lh.block[i] = b->blockno;
4937   if (i == log.lh.n)
4938     log.lh.n++;
4939   b->flags |= B_DIRTY; // prevent eviction
4940   release(&log.lock);
4941 }
4942 
4943 
4944 
4945 
4946 
4947 
4948 
4949 
