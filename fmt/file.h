4150 struct file {
4151   enum { FD_NONE, FD_PIPE, FD_INODE } type;
4152   int ref; // reference count
4153   char readable;
4154   char writable;
4155   struct pipe *pipe;
4156   struct inode *ip;
4157   uint off;
4158 };
4159 
4160 
4161 // in-memory copy of an inode
4162 struct inode {
4163   uint dev;           // Device number
4164   uint inum;          // Inode number
4165   int ref;            // Reference count
4166   struct sleeplock lock; // protects everything below here
4167   int valid;          // inode has been read from disk?
4168 
4169   short type;         // copy of disk inode
4170   short major;
4171   short minor;
4172   short nlink;
4173   uint size;
4174   uint addrs[NDIRECT+1];
4175 };
4176 
4177 // table mapping major device number to
4178 // device functions
4179 struct devsw {
4180   int (*read)(struct inode*, char*, int);
4181   int (*write)(struct inode*, char*, int);
4182 };
4183 
4184 extern struct devsw devsw[];
4185 
4186 #define CONSOLE 1
4187 
4188 
4189 
4190 
4191 
4192 
4193 
4194 
4195 
4196 
4197 
4198 
4199 
