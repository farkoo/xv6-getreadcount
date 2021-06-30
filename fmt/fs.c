4950 // File system implementation.  Five layers:
4951 //   + Blocks: allocator for raw disk blocks.
4952 //   + Log: crash recovery for multi-step updates.
4953 //   + Files: inode allocator, reading, writing, metadata.
4954 //   + Directories: inode with special contents (list of other inodes!)
4955 //   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
4956 //
4957 // This file contains the low-level file system manipulation
4958 // routines.  The (higher-level) system call implementations
4959 // are in sysfile.c.
4960 
4961 #include "types.h"
4962 #include "defs.h"
4963 #include "param.h"
4964 #include "stat.h"
4965 #include "mmu.h"
4966 #include "proc.h"
4967 #include "spinlock.h"
4968 #include "sleeplock.h"
4969 #include "fs.h"
4970 #include "buf.h"
4971 #include "file.h"
4972 
4973 #define min(a, b) ((a) < (b) ? (a) : (b))
4974 static void itrunc(struct inode*);
4975 // there should be one superblock per disk device, but we run with
4976 // only one device
4977 struct superblock sb;
4978 
4979 // Read the super block.
4980 void
4981 readsb(int dev, struct superblock *sb)
4982 {
4983   struct buf *bp;
4984 
4985   bp = bread(dev, 1);
4986   memmove(sb, bp->data, sizeof(*sb));
4987   brelse(bp);
4988 }
4989 
4990 
4991 
4992 
4993 
4994 
4995 
4996 
4997 
4998 
4999 
5000 // Zero a block.
5001 static void
5002 bzero(int dev, int bno)
5003 {
5004   struct buf *bp;
5005 
5006   bp = bread(dev, bno);
5007   memset(bp->data, 0, BSIZE);
5008   log_write(bp);
5009   brelse(bp);
5010 }
5011 
5012 // Blocks.
5013 
5014 // Allocate a zeroed disk block.
5015 static uint
5016 balloc(uint dev)
5017 {
5018   int b, bi, m;
5019   struct buf *bp;
5020 
5021   bp = 0;
5022   for(b = 0; b < sb.size; b += BPB){
5023     bp = bread(dev, BBLOCK(b, sb));
5024     for(bi = 0; bi < BPB && b + bi < sb.size; bi++){
5025       m = 1 << (bi % 8);
5026       if((bp->data[bi/8] & m) == 0){  // Is block free?
5027         bp->data[bi/8] |= m;  // Mark block in use.
5028         log_write(bp);
5029         brelse(bp);
5030         bzero(dev, b + bi);
5031         return b + bi;
5032       }
5033     }
5034     brelse(bp);
5035   }
5036   panic("balloc: out of blocks");
5037 }
5038 
5039 
5040 
5041 
5042 
5043 
5044 
5045 
5046 
5047 
5048 
5049 
5050 // Free a disk block.
5051 static void
5052 bfree(int dev, uint b)
5053 {
5054   struct buf *bp;
5055   int bi, m;
5056 
5057   bp = bread(dev, BBLOCK(b, sb));
5058   bi = b % BPB;
5059   m = 1 << (bi % 8);
5060   if((bp->data[bi/8] & m) == 0)
5061     panic("freeing free block");
5062   bp->data[bi/8] &= ~m;
5063   log_write(bp);
5064   brelse(bp);
5065 }
5066 
5067 // Inodes.
5068 //
5069 // An inode describes a single unnamed file.
5070 // The inode disk structure holds metadata: the file's type,
5071 // its size, the number of links referring to it, and the
5072 // list of blocks holding the file's content.
5073 //
5074 // The inodes are laid out sequentially on disk at
5075 // sb.startinode. Each inode has a number, indicating its
5076 // position on the disk.
5077 //
5078 // The kernel keeps a cache of in-use inodes in memory
5079 // to provide a place for synchronizing access
5080 // to inodes used by multiple processes. The cached
5081 // inodes include book-keeping information that is
5082 // not stored on disk: ip->ref and ip->valid.
5083 //
5084 // An inode and its in-memory representation go through a
5085 // sequence of states before they can be used by the
5086 // rest of the file system code.
5087 //
5088 // * Allocation: an inode is allocated if its type (on disk)
5089 //   is non-zero. ialloc() allocates, and iput() frees if
5090 //   the reference and link counts have fallen to zero.
5091 //
5092 // * Referencing in cache: an entry in the inode cache
5093 //   is free if ip->ref is zero. Otherwise ip->ref tracks
5094 //   the number of in-memory pointers to the entry (open
5095 //   files and current directories). iget() finds or
5096 //   creates a cache entry and increments its ref; iput()
5097 //   decrements ref.
5098 //
5099 // * Valid: the information (type, size, &c) in an inode
5100 //   cache entry is only correct when ip->valid is 1.
5101 //   ilock() reads the inode from
5102 //   the disk and sets ip->valid, while iput() clears
5103 //   ip->valid if ip->ref has fallen to zero.
5104 //
5105 // * Locked: file system code may only examine and modify
5106 //   the information in an inode and its content if it
5107 //   has first locked the inode.
5108 //
5109 // Thus a typical sequence is:
5110 //   ip = iget(dev, inum)
5111 //   ilock(ip)
5112 //   ... examine and modify ip->xxx ...
5113 //   iunlock(ip)
5114 //   iput(ip)
5115 //
5116 // ilock() is separate from iget() so that system calls can
5117 // get a long-term reference to an inode (as for an open file)
5118 // and only lock it for short periods (e.g., in read()).
5119 // The separation also helps avoid deadlock and races during
5120 // pathname lookup. iget() increments ip->ref so that the inode
5121 // stays cached and pointers to it remain valid.
5122 //
5123 // Many internal file system functions expect the caller to
5124 // have locked the inodes involved; this lets callers create
5125 // multi-step atomic operations.
5126 //
5127 // The icache.lock spin-lock protects the allocation of icache
5128 // entries. Since ip->ref indicates whether an entry is free,
5129 // and ip->dev and ip->inum indicate which i-node an entry
5130 // holds, one must hold icache.lock while using any of those fields.
5131 //
5132 // An ip->lock sleep-lock protects all ip-> fields other than ref,
5133 // dev, and inum.  One must hold ip->lock in order to
5134 // read or write that inode's ip->valid, ip->size, ip->type, &c.
5135 
5136 struct {
5137   struct spinlock lock;
5138   struct inode inode[NINODE];
5139 } icache;
5140 
5141 void
5142 iinit(int dev)
5143 {
5144   int i = 0;
5145 
5146   initlock(&icache.lock, "icache");
5147   for(i = 0; i < NINODE; i++) {
5148     initsleeplock(&icache.inode[i].lock, "inode");
5149   }
5150   readsb(dev, &sb);
5151   cprintf("sb: size %d nblocks %d ninodes %d nlog %d logstart %d\
5152  inodestart %d bmap start %d\n", sb.size, sb.nblocks,
5153           sb.ninodes, sb.nlog, sb.logstart, sb.inodestart,
5154           sb.bmapstart);
5155 }
5156 
5157 static struct inode* iget(uint dev, uint inum);
5158 
5159 
5160 
5161 
5162 
5163 
5164 
5165 
5166 
5167 
5168 
5169 
5170 
5171 
5172 
5173 
5174 
5175 
5176 
5177 
5178 
5179 
5180 
5181 
5182 
5183 
5184 
5185 
5186 
5187 
5188 
5189 
5190 
5191 
5192 
5193 
5194 
5195 
5196 
5197 
5198 
5199 
5200 // Allocate an inode on device dev.
5201 // Mark it as allocated by  giving it type type.
5202 // Returns an unlocked but allocated and referenced inode.
5203 struct inode*
5204 ialloc(uint dev, short type)
5205 {
5206   int inum;
5207   struct buf *bp;
5208   struct dinode *dip;
5209 
5210   for(inum = 1; inum < sb.ninodes; inum++){
5211     bp = bread(dev, IBLOCK(inum, sb));
5212     dip = (struct dinode*)bp->data + inum%IPB;
5213     if(dip->type == 0){  // a free inode
5214       memset(dip, 0, sizeof(*dip));
5215       dip->type = type;
5216       log_write(bp);   // mark it allocated on the disk
5217       brelse(bp);
5218       return iget(dev, inum);
5219     }
5220     brelse(bp);
5221   }
5222   panic("ialloc: no inodes");
5223 }
5224 
5225 // Copy a modified in-memory inode to disk.
5226 // Must be called after every change to an ip->xxx field
5227 // that lives on disk, since i-node cache is write-through.
5228 // Caller must hold ip->lock.
5229 void
5230 iupdate(struct inode *ip)
5231 {
5232   struct buf *bp;
5233   struct dinode *dip;
5234 
5235   bp = bread(ip->dev, IBLOCK(ip->inum, sb));
5236   dip = (struct dinode*)bp->data + ip->inum%IPB;
5237   dip->type = ip->type;
5238   dip->major = ip->major;
5239   dip->minor = ip->minor;
5240   dip->nlink = ip->nlink;
5241   dip->size = ip->size;
5242   memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
5243   log_write(bp);
5244   brelse(bp);
5245 }
5246 
5247 
5248 
5249 
5250 // Find the inode with number inum on device dev
5251 // and return the in-memory copy. Does not lock
5252 // the inode and does not read it from disk.
5253 static struct inode*
5254 iget(uint dev, uint inum)
5255 {
5256   struct inode *ip, *empty;
5257 
5258   acquire(&icache.lock);
5259 
5260   // Is the inode already cached?
5261   empty = 0;
5262   for(ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++){
5263     if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
5264       ip->ref++;
5265       release(&icache.lock);
5266       return ip;
5267     }
5268     if(empty == 0 && ip->ref == 0)    // Remember empty slot.
5269       empty = ip;
5270   }
5271 
5272   // Recycle an inode cache entry.
5273   if(empty == 0)
5274     panic("iget: no inodes");
5275 
5276   ip = empty;
5277   ip->dev = dev;
5278   ip->inum = inum;
5279   ip->ref = 1;
5280   ip->valid = 0;
5281   release(&icache.lock);
5282 
5283   return ip;
5284 }
5285 
5286 // Increment reference count for ip.
5287 // Returns ip to enable ip = idup(ip1) idiom.
5288 struct inode*
5289 idup(struct inode *ip)
5290 {
5291   acquire(&icache.lock);
5292   ip->ref++;
5293   release(&icache.lock);
5294   return ip;
5295 }
5296 
5297 
5298 
5299 
5300 // Lock the given inode.
5301 // Reads the inode from disk if necessary.
5302 void
5303 ilock(struct inode *ip)
5304 {
5305   struct buf *bp;
5306   struct dinode *dip;
5307 
5308   if(ip == 0 || ip->ref < 1)
5309     panic("ilock");
5310 
5311   acquiresleep(&ip->lock);
5312 
5313   if(ip->valid == 0){
5314     bp = bread(ip->dev, IBLOCK(ip->inum, sb));
5315     dip = (struct dinode*)bp->data + ip->inum%IPB;
5316     ip->type = dip->type;
5317     ip->major = dip->major;
5318     ip->minor = dip->minor;
5319     ip->nlink = dip->nlink;
5320     ip->size = dip->size;
5321     memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
5322     brelse(bp);
5323     ip->valid = 1;
5324     if(ip->type == 0)
5325       panic("ilock: no type");
5326   }
5327 }
5328 
5329 // Unlock the given inode.
5330 void
5331 iunlock(struct inode *ip)
5332 {
5333   if(ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
5334     panic("iunlock");
5335 
5336   releasesleep(&ip->lock);
5337 }
5338 
5339 
5340 
5341 
5342 
5343 
5344 
5345 
5346 
5347 
5348 
5349 
5350 // Drop a reference to an in-memory inode.
5351 // If that was the last reference, the inode cache entry can
5352 // be recycled.
5353 // If that was the last reference and the inode has no links
5354 // to it, free the inode (and its content) on disk.
5355 // All calls to iput() must be inside a transaction in
5356 // case it has to free the inode.
5357 void
5358 iput(struct inode *ip)
5359 {
5360   acquiresleep(&ip->lock);
5361   if(ip->valid && ip->nlink == 0){
5362     acquire(&icache.lock);
5363     int r = ip->ref;
5364     release(&icache.lock);
5365     if(r == 1){
5366       // inode has no links and no other references: truncate and free.
5367       itrunc(ip);
5368       ip->type = 0;
5369       iupdate(ip);
5370       ip->valid = 0;
5371     }
5372   }
5373   releasesleep(&ip->lock);
5374 
5375   acquire(&icache.lock);
5376   ip->ref--;
5377   release(&icache.lock);
5378 }
5379 
5380 // Common idiom: unlock, then put.
5381 void
5382 iunlockput(struct inode *ip)
5383 {
5384   iunlock(ip);
5385   iput(ip);
5386 }
5387 
5388 
5389 
5390 
5391 
5392 
5393 
5394 
5395 
5396 
5397 
5398 
5399 
5400 // Inode content
5401 //
5402 // The content (data) associated with each inode is stored
5403 // in blocks on the disk. The first NDIRECT block numbers
5404 // are listed in ip->addrs[].  The next NINDIRECT blocks are
5405 // listed in block ip->addrs[NDIRECT].
5406 
5407 // Return the disk block address of the nth block in inode ip.
5408 // If there is no such block, bmap allocates one.
5409 static uint
5410 bmap(struct inode *ip, uint bn)
5411 {
5412   uint addr, *a;
5413   struct buf *bp;
5414 
5415   if(bn < NDIRECT){
5416     if((addr = ip->addrs[bn]) == 0)
5417       ip->addrs[bn] = addr = balloc(ip->dev);
5418     return addr;
5419   }
5420   bn -= NDIRECT;
5421 
5422   if(bn < NINDIRECT){
5423     // Load indirect block, allocating if necessary.
5424     if((addr = ip->addrs[NDIRECT]) == 0)
5425       ip->addrs[NDIRECT] = addr = balloc(ip->dev);
5426     bp = bread(ip->dev, addr);
5427     a = (uint*)bp->data;
5428     if((addr = a[bn]) == 0){
5429       a[bn] = addr = balloc(ip->dev);
5430       log_write(bp);
5431     }
5432     brelse(bp);
5433     return addr;
5434   }
5435 
5436   panic("bmap: out of range");
5437 }
5438 
5439 
5440 
5441 
5442 
5443 
5444 
5445 
5446 
5447 
5448 
5449 
5450 // Truncate inode (discard contents).
5451 // Only called when the inode has no links
5452 // to it (no directory entries referring to it)
5453 // and has no in-memory reference to it (is
5454 // not an open file or current directory).
5455 static void
5456 itrunc(struct inode *ip)
5457 {
5458   int i, j;
5459   struct buf *bp;
5460   uint *a;
5461 
5462   for(i = 0; i < NDIRECT; i++){
5463     if(ip->addrs[i]){
5464       bfree(ip->dev, ip->addrs[i]);
5465       ip->addrs[i] = 0;
5466     }
5467   }
5468 
5469   if(ip->addrs[NDIRECT]){
5470     bp = bread(ip->dev, ip->addrs[NDIRECT]);
5471     a = (uint*)bp->data;
5472     for(j = 0; j < NINDIRECT; j++){
5473       if(a[j])
5474         bfree(ip->dev, a[j]);
5475     }
5476     brelse(bp);
5477     bfree(ip->dev, ip->addrs[NDIRECT]);
5478     ip->addrs[NDIRECT] = 0;
5479   }
5480 
5481   ip->size = 0;
5482   iupdate(ip);
5483 }
5484 
5485 // Copy stat information from inode.
5486 // Caller must hold ip->lock.
5487 void
5488 stati(struct inode *ip, struct stat *st)
5489 {
5490   st->dev = ip->dev;
5491   st->ino = ip->inum;
5492   st->type = ip->type;
5493   st->nlink = ip->nlink;
5494   st->size = ip->size;
5495 }
5496 
5497 
5498 
5499 
5500 // Read data from inode.
5501 // Caller must hold ip->lock.
5502 int
5503 readi(struct inode *ip, char *dst, uint off, uint n)
5504 {
5505   uint tot, m;
5506   struct buf *bp;
5507 
5508   if(ip->type == T_DEV){
5509     if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read)
5510       return -1;
5511     return devsw[ip->major].read(ip, dst, n);
5512   }
5513 
5514   if(off > ip->size || off + n < off)
5515     return -1;
5516   if(off + n > ip->size)
5517     n = ip->size - off;
5518 
5519   for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
5520     bp = bread(ip->dev, bmap(ip, off/BSIZE));
5521     m = min(n - tot, BSIZE - off%BSIZE);
5522     memmove(dst, bp->data + off%BSIZE, m);
5523     brelse(bp);
5524   }
5525   return n;
5526 }
5527 
5528 
5529 
5530 
5531 
5532 
5533 
5534 
5535 
5536 
5537 
5538 
5539 
5540 
5541 
5542 
5543 
5544 
5545 
5546 
5547 
5548 
5549 
5550 // Write data to inode.
5551 // Caller must hold ip->lock.
5552 int
5553 writei(struct inode *ip, char *src, uint off, uint n)
5554 {
5555   uint tot, m;
5556   struct buf *bp;
5557 
5558   if(ip->type == T_DEV){
5559     if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write)
5560       return -1;
5561     return devsw[ip->major].write(ip, src, n);
5562   }
5563 
5564   if(off > ip->size || off + n < off)
5565     return -1;
5566   if(off + n > MAXFILE*BSIZE)
5567     return -1;
5568 
5569   for(tot=0; tot<n; tot+=m, off+=m, src+=m){
5570     bp = bread(ip->dev, bmap(ip, off/BSIZE));
5571     m = min(n - tot, BSIZE - off%BSIZE);
5572     memmove(bp->data + off%BSIZE, src, m);
5573     log_write(bp);
5574     brelse(bp);
5575   }
5576 
5577   if(n > 0 && off > ip->size){
5578     ip->size = off;
5579     iupdate(ip);
5580   }
5581   return n;
5582 }
5583 
5584 
5585 
5586 
5587 
5588 
5589 
5590 
5591 
5592 
5593 
5594 
5595 
5596 
5597 
5598 
5599 
5600 // Directories
5601 
5602 int
5603 namecmp(const char *s, const char *t)
5604 {
5605   return strncmp(s, t, DIRSIZ);
5606 }
5607 
5608 // Look for a directory entry in a directory.
5609 // If found, set *poff to byte offset of entry.
5610 struct inode*
5611 dirlookup(struct inode *dp, char *name, uint *poff)
5612 {
5613   uint off, inum;
5614   struct dirent de;
5615 
5616   if(dp->type != T_DIR)
5617     panic("dirlookup not DIR");
5618 
5619   for(off = 0; off < dp->size; off += sizeof(de)){
5620     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5621       panic("dirlookup read");
5622     if(de.inum == 0)
5623       continue;
5624     if(namecmp(name, de.name) == 0){
5625       // entry matches path element
5626       if(poff)
5627         *poff = off;
5628       inum = de.inum;
5629       return iget(dp->dev, inum);
5630     }
5631   }
5632 
5633   return 0;
5634 }
5635 
5636 
5637 
5638 
5639 
5640 
5641 
5642 
5643 
5644 
5645 
5646 
5647 
5648 
5649 
5650 // Write a new directory entry (name, inum) into the directory dp.
5651 int
5652 dirlink(struct inode *dp, char *name, uint inum)
5653 {
5654   int off;
5655   struct dirent de;
5656   struct inode *ip;
5657 
5658   // Check that name is not present.
5659   if((ip = dirlookup(dp, name, 0)) != 0){
5660     iput(ip);
5661     return -1;
5662   }
5663 
5664   // Look for an empty dirent.
5665   for(off = 0; off < dp->size; off += sizeof(de)){
5666     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5667       panic("dirlink read");
5668     if(de.inum == 0)
5669       break;
5670   }
5671 
5672   strncpy(de.name, name, DIRSIZ);
5673   de.inum = inum;
5674   if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5675     panic("dirlink");
5676 
5677   return 0;
5678 }
5679 
5680 
5681 
5682 
5683 
5684 
5685 
5686 
5687 
5688 
5689 
5690 
5691 
5692 
5693 
5694 
5695 
5696 
5697 
5698 
5699 
5700 // Paths
5701 
5702 // Copy the next path element from path into name.
5703 // Return a pointer to the element following the copied one.
5704 // The returned path has no leading slashes,
5705 // so the caller can check *path=='\0' to see if the name is the last one.
5706 // If no name to remove, return 0.
5707 //
5708 // Examples:
5709 //   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
5710 //   skipelem("///a//bb", name) = "bb", setting name = "a"
5711 //   skipelem("a", name) = "", setting name = "a"
5712 //   skipelem("", name) = skipelem("////", name) = 0
5713 //
5714 static char*
5715 skipelem(char *path, char *name)
5716 {
5717   char *s;
5718   int len;
5719 
5720   while(*path == '/')
5721     path++;
5722   if(*path == 0)
5723     return 0;
5724   s = path;
5725   while(*path != '/' && *path != 0)
5726     path++;
5727   len = path - s;
5728   if(len >= DIRSIZ)
5729     memmove(name, s, DIRSIZ);
5730   else {
5731     memmove(name, s, len);
5732     name[len] = 0;
5733   }
5734   while(*path == '/')
5735     path++;
5736   return path;
5737 }
5738 
5739 
5740 
5741 
5742 
5743 
5744 
5745 
5746 
5747 
5748 
5749 
5750 // Look up and return the inode for a path name.
5751 // If parent != 0, return the inode for the parent and copy the final
5752 // path element into name, which must have room for DIRSIZ bytes.
5753 // Must be called inside a transaction since it calls iput().
5754 static struct inode*
5755 namex(char *path, int nameiparent, char *name)
5756 {
5757   struct inode *ip, *next;
5758 
5759   if(*path == '/')
5760     ip = iget(ROOTDEV, ROOTINO);
5761   else
5762     ip = idup(myproc()->cwd);
5763 
5764   while((path = skipelem(path, name)) != 0){
5765     ilock(ip);
5766     if(ip->type != T_DIR){
5767       iunlockput(ip);
5768       return 0;
5769     }
5770     if(nameiparent && *path == '\0'){
5771       // Stop one level early.
5772       iunlock(ip);
5773       return ip;
5774     }
5775     if((next = dirlookup(ip, name, 0)) == 0){
5776       iunlockput(ip);
5777       return 0;
5778     }
5779     iunlockput(ip);
5780     ip = next;
5781   }
5782   if(nameiparent){
5783     iput(ip);
5784     return 0;
5785   }
5786   return ip;
5787 }
5788 
5789 struct inode*
5790 namei(char *path)
5791 {
5792   char name[DIRSIZ];
5793   return namex(path, 0, name);
5794 }
5795 
5796 
5797 
5798 
5799 
5800 struct inode*
5801 nameiparent(char *path, char *name)
5802 {
5803   return namex(path, 1, name);
5804 }
5805 
5806 
5807 
5808 
5809 
5810 
5811 
5812 
5813 
5814 
5815 
5816 
5817 
5818 
5819 
5820 
5821 
5822 
5823 
5824 
5825 
5826 
5827 
5828 
5829 
5830 
5831 
5832 
5833 
5834 
5835 
5836 
5837 
5838 
5839 
5840 
5841 
5842 
5843 
5844 
5845 
5846 
5847 
5848 
5849 
