0900 // Format of an ELF executable file
0901 
0902 #define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian
0903 
0904 // File header
0905 struct elfhdr {
0906   uint magic;  // must equal ELF_MAGIC
0907   uchar elf[12];
0908   ushort type;
0909   ushort machine;
0910   uint version;
0911   uint entry;
0912   uint phoff;
0913   uint shoff;
0914   uint flags;
0915   ushort ehsize;
0916   ushort phentsize;
0917   ushort phnum;
0918   ushort shentsize;
0919   ushort shnum;
0920   ushort shstrndx;
0921 };
0922 
0923 // Program section header
0924 struct proghdr {
0925   uint type;
0926   uint off;
0927   uint vaddr;
0928   uint paddr;
0929   uint filesz;
0930   uint memsz;
0931   uint flags;
0932   uint align;
0933 };
0934 
0935 // Values for Proghdr type
0936 #define ELF_PROG_LOAD           1
0937 
0938 // Flag bits for Proghdr flags
0939 #define ELF_PROG_FLAG_EXEC      1
0940 #define ELF_PROG_FLAG_WRITE     2
0941 #define ELF_PROG_FLAG_READ      4
0942 
0943 
0944 
0945 
0946 
0947 
0948 
0949 
