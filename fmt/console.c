7900 // Console input and output.
7901 // Input is from the keyboard or serial port.
7902 // Output is written to the screen and serial port.
7903 
7904 #include "types.h"
7905 #include "defs.h"
7906 #include "param.h"
7907 #include "traps.h"
7908 #include "spinlock.h"
7909 #include "sleeplock.h"
7910 #include "fs.h"
7911 #include "file.h"
7912 #include "memlayout.h"
7913 #include "mmu.h"
7914 #include "proc.h"
7915 #include "x86.h"
7916 
7917 static void consputc(int);
7918 
7919 static int panicked = 0;
7920 
7921 static struct {
7922   struct spinlock lock;
7923   int locking;
7924 } cons;
7925 
7926 static void
7927 printint(int xx, int base, int sign)
7928 {
7929   static char digits[] = "0123456789abcdef";
7930   char buf[16];
7931   int i;
7932   uint x;
7933 
7934   if(sign && (sign = xx < 0))
7935     x = -xx;
7936   else
7937     x = xx;
7938 
7939   i = 0;
7940   do{
7941     buf[i++] = digits[x % base];
7942   }while((x /= base) != 0);
7943 
7944   if(sign)
7945     buf[i++] = '-';
7946 
7947   while(--i >= 0)
7948     consputc(buf[i]);
7949 }
7950 
7951 
7952 
7953 
7954 
7955 
7956 
7957 
7958 
7959 
7960 
7961 
7962 
7963 
7964 
7965 
7966 
7967 
7968 
7969 
7970 
7971 
7972 
7973 
7974 
7975 
7976 
7977 
7978 
7979 
7980 
7981 
7982 
7983 
7984 
7985 
7986 
7987 
7988 
7989 
7990 
7991 
7992 
7993 
7994 
7995 
7996 
7997 
7998 
7999 
8000 // Print to the console. only understands %d, %x, %p, %s.
8001 void
8002 cprintf(char *fmt, ...)
8003 {
8004   int i, c, locking;
8005   uint *argp;
8006   char *s;
8007 
8008   locking = cons.locking;
8009   if(locking)
8010     acquire(&cons.lock);
8011 
8012   if (fmt == 0)
8013     panic("null fmt");
8014 
8015   argp = (uint*)(void*)(&fmt + 1);
8016   for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
8017     if(c != '%'){
8018       consputc(c);
8019       continue;
8020     }
8021     c = fmt[++i] & 0xff;
8022     if(c == 0)
8023       break;
8024     switch(c){
8025     case 'd':
8026       printint(*argp++, 10, 1);
8027       break;
8028     case 'x':
8029     case 'p':
8030       printint(*argp++, 16, 0);
8031       break;
8032     case 's':
8033       if((s = (char*)*argp++) == 0)
8034         s = "(null)";
8035       for(; *s; s++)
8036         consputc(*s);
8037       break;
8038     case '%':
8039       consputc('%');
8040       break;
8041     default:
8042       // Print unknown % sequence to draw attention.
8043       consputc('%');
8044       consputc(c);
8045       break;
8046     }
8047   }
8048 
8049 
8050   if(locking)
8051     release(&cons.lock);
8052 }
8053 
8054 void
8055 panic(char *s)
8056 {
8057   int i;
8058   uint pcs[10];
8059 
8060   cli();
8061   cons.locking = 0;
8062   // use lapiccpunum so that we can call panic from mycpu()
8063   cprintf("lapicid %d: panic: ", lapicid());
8064   cprintf(s);
8065   cprintf("\n");
8066   getcallerpcs(&s, pcs);
8067   for(i=0; i<10; i++)
8068     cprintf(" %p", pcs[i]);
8069   panicked = 1; // freeze other CPU
8070   for(;;)
8071     ;
8072 }
8073 
8074 
8075 
8076 
8077 
8078 
8079 
8080 
8081 
8082 
8083 
8084 
8085 
8086 
8087 
8088 
8089 
8090 
8091 
8092 
8093 
8094 
8095 
8096 
8097 
8098 
8099 
8100 #define BACKSPACE 0x100
8101 #define CRTPORT 0x3d4
8102 static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory
8103 
8104 static void
8105 cgaputc(int c)
8106 {
8107   int pos;
8108 
8109   // Cursor position: col + 80*row.
8110   outb(CRTPORT, 14);
8111   pos = inb(CRTPORT+1) << 8;
8112   outb(CRTPORT, 15);
8113   pos |= inb(CRTPORT+1);
8114 
8115   if(c == '\n')
8116     pos += 80 - pos%80;
8117   else if(c == BACKSPACE){
8118     if(pos > 0) --pos;
8119   } else
8120     crt[pos++] = (c&0xff) | 0x0700;  // black on white
8121 
8122   if(pos < 0 || pos > 25*80)
8123     panic("pos under/overflow");
8124 
8125   if((pos/80) >= 24){  // Scroll up.
8126     memmove(crt, crt+80, sizeof(crt[0])*23*80);
8127     pos -= 80;
8128     memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
8129   }
8130 
8131   outb(CRTPORT, 14);
8132   outb(CRTPORT+1, pos>>8);
8133   outb(CRTPORT, 15);
8134   outb(CRTPORT+1, pos);
8135   crt[pos] = ' ' | 0x0700;
8136 }
8137 
8138 
8139 
8140 
8141 
8142 
8143 
8144 
8145 
8146 
8147 
8148 
8149 
8150 void
8151 consputc(int c)
8152 {
8153   if(panicked){
8154     cli();
8155     for(;;)
8156       ;
8157   }
8158 
8159   if(c == BACKSPACE){
8160     uartputc('\b'); uartputc(' '); uartputc('\b');
8161   } else
8162     uartputc(c);
8163   cgaputc(c);
8164 }
8165 
8166 #define INPUT_BUF 128
8167 struct {
8168   char buf[INPUT_BUF];
8169   uint r;  // Read index
8170   uint w;  // Write index
8171   uint e;  // Edit index
8172 } input;
8173 
8174 #define C(x)  ((x)-'@')  // Control-x
8175 
8176 void
8177 consoleintr(int (*getc)(void))
8178 {
8179   int c, doprocdump = 0;
8180 
8181   acquire(&cons.lock);
8182   while((c = getc()) >= 0){
8183     switch(c){
8184     case C('P'):  // Process listing.
8185       // procdump() locks cons.lock indirectly; invoke later
8186       doprocdump = 1;
8187       break;
8188     case C('U'):  // Kill line.
8189       while(input.e != input.w &&
8190             input.buf[(input.e-1) % INPUT_BUF] != '\n'){
8191         input.e--;
8192         consputc(BACKSPACE);
8193       }
8194       break;
8195     case C('H'): case '\x7f':  // Backspace
8196       if(input.e != input.w){
8197         input.e--;
8198         consputc(BACKSPACE);
8199       }
8200       break;
8201     default:
8202       if(c != 0 && input.e-input.r < INPUT_BUF){
8203         c = (c == '\r') ? '\n' : c;
8204         input.buf[input.e++ % INPUT_BUF] = c;
8205         consputc(c);
8206         if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
8207           input.w = input.e;
8208           wakeup(&input.r);
8209         }
8210       }
8211       break;
8212     }
8213   }
8214   release(&cons.lock);
8215   if(doprocdump) {
8216     procdump();  // now call procdump() wo. cons.lock held
8217   }
8218 }
8219 
8220 int
8221 consoleread(struct inode *ip, char *dst, int n)
8222 {
8223   uint target;
8224   int c;
8225 
8226   iunlock(ip);
8227   target = n;
8228   acquire(&cons.lock);
8229   while(n > 0){
8230     while(input.r == input.w){
8231       if(myproc()->killed){
8232         release(&cons.lock);
8233         ilock(ip);
8234         return -1;
8235       }
8236       sleep(&input.r, &cons.lock);
8237     }
8238     c = input.buf[input.r++ % INPUT_BUF];
8239     if(c == C('D')){  // EOF
8240       if(n < target){
8241         // Save ^D for next time, to make sure
8242         // caller gets a 0-byte result.
8243         input.r--;
8244       }
8245       break;
8246     }
8247     *dst++ = c;
8248     --n;
8249     if(c == '\n')
8250       break;
8251   }
8252   release(&cons.lock);
8253   ilock(ip);
8254 
8255   return target - n;
8256 }
8257 
8258 int
8259 consolewrite(struct inode *ip, char *buf, int n)
8260 {
8261   int i;
8262 
8263   iunlock(ip);
8264   acquire(&cons.lock);
8265   for(i = 0; i < n; i++)
8266     consputc(buf[i] & 0xff);
8267   release(&cons.lock);
8268   ilock(ip);
8269 
8270   return n;
8271 }
8272 
8273 void
8274 consoleinit(void)
8275 {
8276   initlock(&cons.lock, "console");
8277 
8278   devsw[CONSOLE].write = consolewrite;
8279   devsw[CONSOLE].read = consoleread;
8280   cons.locking = 1;
8281 
8282   ioapicenable(IRQ_KBD, 0);
8283 }
8284 
8285 
8286 
8287 
8288 
8289 
8290 
8291 
8292 
8293 
8294 
8295 
8296 
8297 
8298 
8299 
