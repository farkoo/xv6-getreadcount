5850 //
5851 // File descriptors
5852 //
5853 
5854 #include "types.h"
5855 #include "defs.h"
5856 #include "param.h"
5857 #include "fs.h"
5858 #include "spinlock.h"
5859 #include "sleeplock.h"
5860 #include "file.h"
5861 
5862 struct devsw devsw[NDEV];
5863 struct {
5864   struct spinlock lock;
5865   struct file file[NFILE];
5866 } ftable;
5867 
5868 void
5869 fileinit(void)
5870 {
5871   initlock(&ftable.lock, "ftable");
5872 }
5873 
5874 // Allocate a file structure.
5875 struct file*
5876 filealloc(void)
5877 {
5878   struct file *f;
5879 
5880   acquire(&ftable.lock);
5881   for(f = ftable.file; f < ftable.file + NFILE; f++){
5882     if(f->ref == 0){
5883       f->ref = 1;
5884       release(&ftable.lock);
5885       return f;
5886     }
5887   }
5888   release(&ftable.lock);
5889   return 0;
5890 }
5891 
5892 
5893 
5894 
5895 
5896 
5897 
5898 
5899 
5900 // Increment ref count for file f.
5901 struct file*
5902 filedup(struct file *f)
5903 {
5904   acquire(&ftable.lock);
5905   if(f->ref < 1)
5906     panic("filedup");
5907   f->ref++;
5908   release(&ftable.lock);
5909   return f;
5910 }
5911 
5912 // Close file f.  (Decrement ref count, close when reaches 0.)
5913 void
5914 fileclose(struct file *f)
5915 {
5916   struct file ff;
5917 
5918   acquire(&ftable.lock);
5919   if(f->ref < 1)
5920     panic("fileclose");
5921   if(--f->ref > 0){
5922     release(&ftable.lock);
5923     return;
5924   }
5925   ff = *f;
5926   f->ref = 0;
5927   f->type = FD_NONE;
5928   release(&ftable.lock);
5929 
5930   if(ff.type == FD_PIPE)
5931     pipeclose(ff.pipe, ff.writable);
5932   else if(ff.type == FD_INODE){
5933     begin_op();
5934     iput(ff.ip);
5935     end_op();
5936   }
5937 }
5938 
5939 
5940 
5941 
5942 
5943 
5944 
5945 
5946 
5947 
5948 
5949 
5950 // Get metadata about file f.
5951 int
5952 filestat(struct file *f, struct stat *st)
5953 {
5954   if(f->type == FD_INODE){
5955     ilock(f->ip);
5956     stati(f->ip, st);
5957     iunlock(f->ip);
5958     return 0;
5959   }
5960   return -1;
5961 }
5962 
5963 // Read from file f.
5964 int
5965 fileread(struct file *f, char *addr, int n)
5966 {
5967   int r;
5968 
5969   if(f->readable == 0)
5970     return -1;
5971   if(f->type == FD_PIPE)
5972     return piperead(f->pipe, addr, n);
5973   if(f->type == FD_INODE){
5974     ilock(f->ip);
5975     if((r = readi(f->ip, addr, f->off, n)) > 0)
5976       f->off += r;
5977     iunlock(f->ip);
5978     return r;
5979   }
5980   panic("fileread");
5981 }
5982 
5983 
5984 
5985 
5986 
5987 
5988 
5989 
5990 
5991 
5992 
5993 
5994 
5995 
5996 
5997 
5998 
5999 
6000 // Write to file f.
6001 int
6002 filewrite(struct file *f, char *addr, int n)
6003 {
6004   int r;
6005 
6006   if(f->writable == 0)
6007     return -1;
6008   if(f->type == FD_PIPE)
6009     return pipewrite(f->pipe, addr, n);
6010   if(f->type == FD_INODE){
6011     // write a few blocks at a time to avoid exceeding
6012     // the maximum log transaction size, including
6013     // i-node, indirect block, allocation blocks,
6014     // and 2 blocks of slop for non-aligned writes.
6015     // this really belongs lower down, since writei()
6016     // might be writing a device like the console.
6017     int max = ((MAXOPBLOCKS-1-1-2) / 2) * 512;
6018     int i = 0;
6019     while(i < n){
6020       int n1 = n - i;
6021       if(n1 > max)
6022         n1 = max;
6023 
6024       begin_op();
6025       ilock(f->ip);
6026       if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
6027         f->off += r;
6028       iunlock(f->ip);
6029       end_op();
6030 
6031       if(r < 0)
6032         break;
6033       if(r != n1)
6034         panic("short filewrite");
6035       i += r;
6036     }
6037     return i == n ? n : -1;
6038   }
6039   panic("filewrite");
6040 }
6041 
6042 
6043 
6044 
6045 
6046 
6047 
6048 
6049 
