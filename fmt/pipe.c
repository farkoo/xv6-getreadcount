6750 #include "types.h"
6751 #include "defs.h"
6752 #include "param.h"
6753 #include "mmu.h"
6754 #include "proc.h"
6755 #include "fs.h"
6756 #include "spinlock.h"
6757 #include "sleeplock.h"
6758 #include "file.h"
6759 
6760 #define PIPESIZE 512
6761 
6762 struct pipe {
6763   struct spinlock lock;
6764   char data[PIPESIZE];
6765   uint nread;     // number of bytes read
6766   uint nwrite;    // number of bytes written
6767   int readopen;   // read fd is still open
6768   int writeopen;  // write fd is still open
6769 };
6770 
6771 int
6772 pipealloc(struct file **f0, struct file **f1)
6773 {
6774   struct pipe *p;
6775 
6776   p = 0;
6777   *f0 = *f1 = 0;
6778   if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
6779     goto bad;
6780   if((p = (struct pipe*)kalloc()) == 0)
6781     goto bad;
6782   p->readopen = 1;
6783   p->writeopen = 1;
6784   p->nwrite = 0;
6785   p->nread = 0;
6786   initlock(&p->lock, "pipe");
6787   (*f0)->type = FD_PIPE;
6788   (*f0)->readable = 1;
6789   (*f0)->writable = 0;
6790   (*f0)->pipe = p;
6791   (*f1)->type = FD_PIPE;
6792   (*f1)->readable = 0;
6793   (*f1)->writable = 1;
6794   (*f1)->pipe = p;
6795   return 0;
6796 
6797 
6798 
6799 
6800  bad:
6801   if(p)
6802     kfree((char*)p);
6803   if(*f0)
6804     fileclose(*f0);
6805   if(*f1)
6806     fileclose(*f1);
6807   return -1;
6808 }
6809 
6810 void
6811 pipeclose(struct pipe *p, int writable)
6812 {
6813   acquire(&p->lock);
6814   if(writable){
6815     p->writeopen = 0;
6816     wakeup(&p->nread);
6817   } else {
6818     p->readopen = 0;
6819     wakeup(&p->nwrite);
6820   }
6821   if(p->readopen == 0 && p->writeopen == 0){
6822     release(&p->lock);
6823     kfree((char*)p);
6824   } else
6825     release(&p->lock);
6826 }
6827 
6828 
6829 int
6830 pipewrite(struct pipe *p, char *addr, int n)
6831 {
6832   int i;
6833 
6834   acquire(&p->lock);
6835   for(i = 0; i < n; i++){
6836     while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
6837       if(p->readopen == 0 || myproc()->killed){
6838         release(&p->lock);
6839         return -1;
6840       }
6841       wakeup(&p->nread);
6842       sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
6843     }
6844     p->data[p->nwrite++ % PIPESIZE] = addr[i];
6845   }
6846   wakeup(&p->nread);  //DOC: pipewrite-wakeup1
6847   release(&p->lock);
6848   return n;
6849 }
6850 int
6851 piperead(struct pipe *p, char *addr, int n)
6852 {
6853   int i;
6854 
6855   acquire(&p->lock);
6856   while(p->nread == p->nwrite && p->writeopen){  //DOC: pipe-empty
6857     if(myproc()->killed){
6858       release(&p->lock);
6859       return -1;
6860     }
6861     sleep(&p->nread, &p->lock); //DOC: piperead-sleep
6862   }
6863   for(i = 0; i < n; i++){  //DOC: piperead-copy
6864     if(p->nread == p->nwrite)
6865       break;
6866     addr[i] = p->data[p->nread++ % PIPESIZE];
6867   }
6868   wakeup(&p->nwrite);  //DOC: piperead-wakeup
6869   release(&p->lock);
6870   return i;
6871 }
6872 
6873 
6874 
6875 
6876 
6877 
6878 
6879 
6880 
6881 
6882 
6883 
6884 
6885 
6886 
6887 
6888 
6889 
6890 
6891 
6892 
6893 
6894 
6895 
6896 
6897 
6898 
6899 
