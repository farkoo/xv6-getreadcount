6050 //
6051 // File-system system calls.
6052 // Mostly argument checking, since we don't trust
6053 // user code, and calls into file.c and fs.c.
6054 //
6055 
6056 #include "types.h"
6057 #include "defs.h"
6058 #include "param.h"
6059 #include "stat.h"
6060 #include "mmu.h"
6061 #include "proc.h"
6062 #include "fs.h"
6063 #include "spinlock.h"
6064 #include "sleeplock.h"
6065 #include "file.h"
6066 #include "fcntl.h"
6067 
6068 // Fetch the nth word-sized system call argument as a file descriptor
6069 // and return both the descriptor and the corresponding struct file.
6070 static int
6071 argfd(int n, int *pfd, struct file **pf)
6072 {
6073   int fd;
6074   struct file *f;
6075 
6076   if(argint(n, &fd) < 0)
6077     return -1;
6078   if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
6079     return -1;
6080   if(pfd)
6081     *pfd = fd;
6082   if(pf)
6083     *pf = f;
6084   return 0;
6085 }
6086 
6087 
6088 
6089 
6090 
6091 
6092 
6093 
6094 
6095 
6096 
6097 
6098 
6099 
6100 // Allocate a file descriptor for the given file.
6101 // Takes over file reference from caller on success.
6102 static int
6103 fdalloc(struct file *f)
6104 {
6105   int fd;
6106   struct proc *curproc = myproc();
6107 
6108   for(fd = 0; fd < NOFILE; fd++){
6109     if(curproc->ofile[fd] == 0){
6110       curproc->ofile[fd] = f;
6111       return fd;
6112     }
6113   }
6114   return -1;
6115 }
6116 
6117 int
6118 sys_dup(void)
6119 {
6120   struct file *f;
6121   int fd;
6122 
6123   if(argfd(0, 0, &f) < 0)
6124     return -1;
6125   if((fd=fdalloc(f)) < 0)
6126     return -1;
6127   filedup(f);
6128   return fd;
6129 }
6130 
6131 int
6132 sys_read(void)
6133 {
6134   struct file *f;
6135   int n;
6136   char *p;
6137 
6138   if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
6139     return -1;
6140   return fileread(f, p, n);
6141 }
6142 
6143 
6144 
6145 
6146 
6147 
6148 
6149 
6150 int
6151 sys_write(void)
6152 {
6153   struct file *f;
6154   int n;
6155   char *p;
6156 
6157   if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
6158     return -1;
6159   return filewrite(f, p, n);
6160 }
6161 
6162 int
6163 sys_close(void)
6164 {
6165   int fd;
6166   struct file *f;
6167 
6168   if(argfd(0, &fd, &f) < 0)
6169     return -1;
6170   myproc()->ofile[fd] = 0;
6171   fileclose(f);
6172   return 0;
6173 }
6174 
6175 int
6176 sys_fstat(void)
6177 {
6178   struct file *f;
6179   struct stat *st;
6180 
6181   if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
6182     return -1;
6183   return filestat(f, st);
6184 }
6185 
6186 
6187 
6188 
6189 
6190 
6191 
6192 
6193 
6194 
6195 
6196 
6197 
6198 
6199 
6200 // Create the path new as a link to the same inode as old.
6201 int
6202 sys_link(void)
6203 {
6204   char name[DIRSIZ], *new, *old;
6205   struct inode *dp, *ip;
6206 
6207   if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
6208     return -1;
6209 
6210   begin_op();
6211   if((ip = namei(old)) == 0){
6212     end_op();
6213     return -1;
6214   }
6215 
6216   ilock(ip);
6217   if(ip->type == T_DIR){
6218     iunlockput(ip);
6219     end_op();
6220     return -1;
6221   }
6222 
6223   ip->nlink++;
6224   iupdate(ip);
6225   iunlock(ip);
6226 
6227   if((dp = nameiparent(new, name)) == 0)
6228     goto bad;
6229   ilock(dp);
6230   if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
6231     iunlockput(dp);
6232     goto bad;
6233   }
6234   iunlockput(dp);
6235   iput(ip);
6236 
6237   end_op();
6238 
6239   return 0;
6240 
6241 bad:
6242   ilock(ip);
6243   ip->nlink--;
6244   iupdate(ip);
6245   iunlockput(ip);
6246   end_op();
6247   return -1;
6248 }
6249 
6250 // Is the directory dp empty except for "." and ".." ?
6251 static int
6252 isdirempty(struct inode *dp)
6253 {
6254   int off;
6255   struct dirent de;
6256 
6257   for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
6258     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
6259       panic("isdirempty: readi");
6260     if(de.inum != 0)
6261       return 0;
6262   }
6263   return 1;
6264 }
6265 
6266 
6267 
6268 
6269 
6270 
6271 
6272 
6273 
6274 
6275 
6276 
6277 
6278 
6279 
6280 
6281 
6282 
6283 
6284 
6285 
6286 
6287 
6288 
6289 
6290 
6291 
6292 
6293 
6294 
6295 
6296 
6297 
6298 
6299 
6300 int
6301 sys_unlink(void)
6302 {
6303   struct inode *ip, *dp;
6304   struct dirent de;
6305   char name[DIRSIZ], *path;
6306   uint off;
6307 
6308   if(argstr(0, &path) < 0)
6309     return -1;
6310 
6311   begin_op();
6312   if((dp = nameiparent(path, name)) == 0){
6313     end_op();
6314     return -1;
6315   }
6316 
6317   ilock(dp);
6318 
6319   // Cannot unlink "." or "..".
6320   if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
6321     goto bad;
6322 
6323   if((ip = dirlookup(dp, name, &off)) == 0)
6324     goto bad;
6325   ilock(ip);
6326 
6327   if(ip->nlink < 1)
6328     panic("unlink: nlink < 1");
6329   if(ip->type == T_DIR && !isdirempty(ip)){
6330     iunlockput(ip);
6331     goto bad;
6332   }
6333 
6334   memset(&de, 0, sizeof(de));
6335   if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
6336     panic("unlink: writei");
6337   if(ip->type == T_DIR){
6338     dp->nlink--;
6339     iupdate(dp);
6340   }
6341   iunlockput(dp);
6342 
6343   ip->nlink--;
6344   iupdate(ip);
6345   iunlockput(ip);
6346 
6347   end_op();
6348 
6349   return 0;
6350 bad:
6351   iunlockput(dp);
6352   end_op();
6353   return -1;
6354 }
6355 
6356 static struct inode*
6357 create(char *path, short type, short major, short minor)
6358 {
6359   struct inode *ip, *dp;
6360   char name[DIRSIZ];
6361 
6362   if((dp = nameiparent(path, name)) == 0)
6363     return 0;
6364   ilock(dp);
6365 
6366   if((ip = dirlookup(dp, name, 0)) != 0){
6367     iunlockput(dp);
6368     ilock(ip);
6369     if(type == T_FILE && ip->type == T_FILE)
6370       return ip;
6371     iunlockput(ip);
6372     return 0;
6373   }
6374 
6375   if((ip = ialloc(dp->dev, type)) == 0)
6376     panic("create: ialloc");
6377 
6378   ilock(ip);
6379   ip->major = major;
6380   ip->minor = minor;
6381   ip->nlink = 1;
6382   iupdate(ip);
6383 
6384   if(type == T_DIR){  // Create . and .. entries.
6385     dp->nlink++;  // for ".."
6386     iupdate(dp);
6387     // No ip->nlink++ for ".": avoid cyclic ref count.
6388     if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
6389       panic("create dots");
6390   }
6391 
6392   if(dirlink(dp, name, ip->inum) < 0)
6393     panic("create: dirlink");
6394 
6395   iunlockput(dp);
6396 
6397   return ip;
6398 }
6399 
6400 int
6401 sys_open(void)
6402 {
6403   char *path;
6404   int fd, omode;
6405   struct file *f;
6406   struct inode *ip;
6407 
6408   if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
6409     return -1;
6410 
6411   begin_op();
6412 
6413   if(omode & O_CREATE){
6414     ip = create(path, T_FILE, 0, 0);
6415     if(ip == 0){
6416       end_op();
6417       return -1;
6418     }
6419   } else {
6420     if((ip = namei(path)) == 0){
6421       end_op();
6422       return -1;
6423     }
6424     ilock(ip);
6425     if(ip->type == T_DIR && omode != O_RDONLY){
6426       iunlockput(ip);
6427       end_op();
6428       return -1;
6429     }
6430   }
6431 
6432   if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
6433     if(f)
6434       fileclose(f);
6435     iunlockput(ip);
6436     end_op();
6437     return -1;
6438   }
6439   iunlock(ip);
6440   end_op();
6441 
6442   f->type = FD_INODE;
6443   f->ip = ip;
6444   f->off = 0;
6445   f->readable = !(omode & O_WRONLY);
6446   f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
6447   return fd;
6448 }
6449 
6450 int
6451 sys_mkdir(void)
6452 {
6453   char *path;
6454   struct inode *ip;
6455 
6456   begin_op();
6457   if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
6458     end_op();
6459     return -1;
6460   }
6461   iunlockput(ip);
6462   end_op();
6463   return 0;
6464 }
6465 
6466 int
6467 sys_mknod(void)
6468 {
6469   struct inode *ip;
6470   char *path;
6471   int major, minor;
6472 
6473   begin_op();
6474   if((argstr(0, &path)) < 0 ||
6475      argint(1, &major) < 0 ||
6476      argint(2, &minor) < 0 ||
6477      (ip = create(path, T_DEV, major, minor)) == 0){
6478     end_op();
6479     return -1;
6480   }
6481   iunlockput(ip);
6482   end_op();
6483   return 0;
6484 }
6485 
6486 
6487 
6488 
6489 
6490 
6491 
6492 
6493 
6494 
6495 
6496 
6497 
6498 
6499 
6500 int
6501 sys_chdir(void)
6502 {
6503   char *path;
6504   struct inode *ip;
6505   struct proc *curproc = myproc();
6506 
6507   begin_op();
6508   if(argstr(0, &path) < 0 || (ip = namei(path)) == 0){
6509     end_op();
6510     return -1;
6511   }
6512   ilock(ip);
6513   if(ip->type != T_DIR){
6514     iunlockput(ip);
6515     end_op();
6516     return -1;
6517   }
6518   iunlock(ip);
6519   iput(curproc->cwd);
6520   end_op();
6521   curproc->cwd = ip;
6522   return 0;
6523 }
6524 
6525 int
6526 sys_exec(void)
6527 {
6528   char *path, *argv[MAXARG];
6529   int i;
6530   uint uargv, uarg;
6531 
6532   if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
6533     return -1;
6534   }
6535   memset(argv, 0, sizeof(argv));
6536   for(i=0;; i++){
6537     if(i >= NELEM(argv))
6538       return -1;
6539     if(fetchint(uargv+4*i, (int*)&uarg) < 0)
6540       return -1;
6541     if(uarg == 0){
6542       argv[i] = 0;
6543       break;
6544     }
6545     if(fetchstr(uarg, &argv[i]) < 0)
6546       return -1;
6547   }
6548   return exec(path, argv);
6549 }
6550 int
6551 sys_pipe(void)
6552 {
6553   int *fd;
6554   struct file *rf, *wf;
6555   int fd0, fd1;
6556 
6557   if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
6558     return -1;
6559   if(pipealloc(&rf, &wf) < 0)
6560     return -1;
6561   fd0 = -1;
6562   if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
6563     if(fd0 >= 0)
6564       myproc()->ofile[fd0] = 0;
6565     fileclose(rf);
6566     fileclose(wf);
6567     return -1;
6568   }
6569   fd[0] = fd0;
6570   fd[1] = fd1;
6571   return 0;
6572 }
6573 
6574 
6575 
6576 
6577 
6578 
6579 
6580 
6581 
6582 
6583 
6584 
6585 
6586 
6587 
6588 
6589 
6590 
6591 
6592 
6593 
6594 
6595 
6596 
6597 
6598 
6599 
