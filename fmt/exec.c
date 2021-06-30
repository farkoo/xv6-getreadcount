6600 #include "types.h"
6601 #include "param.h"
6602 #include "memlayout.h"
6603 #include "mmu.h"
6604 #include "proc.h"
6605 #include "defs.h"
6606 #include "x86.h"
6607 #include "elf.h"
6608 
6609 int
6610 exec(char *path, char **argv)
6611 {
6612   char *s, *last;
6613   int i, off;
6614   uint argc, sz, sp, ustack[3+MAXARG+1];
6615   struct elfhdr elf;
6616   struct inode *ip;
6617   struct proghdr ph;
6618   pde_t *pgdir, *oldpgdir;
6619   struct proc *curproc = myproc();
6620 
6621   begin_op();
6622 
6623   if((ip = namei(path)) == 0){
6624     end_op();
6625     cprintf("exec: fail\n");
6626     return -1;
6627   }
6628   ilock(ip);
6629   pgdir = 0;
6630 
6631   // Check ELF header
6632   if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
6633     goto bad;
6634   if(elf.magic != ELF_MAGIC)
6635     goto bad;
6636 
6637   if((pgdir = setupkvm()) == 0)
6638     goto bad;
6639 
6640   // Load program into memory.
6641   sz = 0;
6642   for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
6643     if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
6644       goto bad;
6645     if(ph.type != ELF_PROG_LOAD)
6646       continue;
6647     if(ph.memsz < ph.filesz)
6648       goto bad;
6649     if(ph.vaddr + ph.memsz < ph.vaddr)
6650       goto bad;
6651     if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
6652       goto bad;
6653     if(ph.vaddr % PGSIZE != 0)
6654       goto bad;
6655     if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
6656       goto bad;
6657   }
6658   iunlockput(ip);
6659   end_op();
6660   ip = 0;
6661 
6662   // Allocate two pages at the next page boundary.
6663   // Make the first inaccessible.  Use the second as the user stack.
6664   sz = PGROUNDUP(sz);
6665   if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
6666     goto bad;
6667   clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
6668   sp = sz;
6669 
6670   // Push argument strings, prepare rest of stack in ustack.
6671   for(argc = 0; argv[argc]; argc++) {
6672     if(argc >= MAXARG)
6673       goto bad;
6674     sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
6675     if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
6676       goto bad;
6677     ustack[3+argc] = sp;
6678   }
6679   ustack[3+argc] = 0;
6680 
6681   ustack[0] = 0xffffffff;  // fake return PC
6682   ustack[1] = argc;
6683   ustack[2] = sp - (argc+1)*4;  // argv pointer
6684 
6685   sp -= (3+argc+1) * 4;
6686   if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
6687     goto bad;
6688 
6689   // Save program name for debugging.
6690   for(last=s=path; *s; s++)
6691     if(*s == '/')
6692       last = s+1;
6693   safestrcpy(curproc->name, last, sizeof(curproc->name));
6694 
6695   // Commit to the user image.
6696   oldpgdir = curproc->pgdir;
6697   curproc->pgdir = pgdir;
6698   curproc->sz = sz;
6699   curproc->tf->eip = elf.entry;  // main
6700   curproc->tf->esp = sp;
6701   switchuvm(curproc);
6702   freevm(oldpgdir);
6703   return 0;
6704 
6705  bad:
6706   if(pgdir)
6707     freevm(pgdir);
6708   if(ip){
6709     iunlockput(ip);
6710     end_op();
6711   }
6712   return -1;
6713 }
6714 
6715 
6716 
6717 
6718 
6719 
6720 
6721 
6722 
6723 
6724 
6725 
6726 
6727 
6728 
6729 
6730 
6731 
6732 
6733 
6734 
6735 
6736 
6737 
6738 
6739 
6740 
6741 
6742 
6743 
6744 
6745 
6746 
6747 
6748 
6749 
