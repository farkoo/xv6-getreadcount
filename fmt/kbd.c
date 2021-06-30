7850 #include "types.h"
7851 #include "x86.h"
7852 #include "defs.h"
7853 #include "kbd.h"
7854 
7855 int
7856 kbdgetc(void)
7857 {
7858   static uint shift;
7859   static uchar *charcode[4] = {
7860     normalmap, shiftmap, ctlmap, ctlmap
7861   };
7862   uint st, data, c;
7863 
7864   st = inb(KBSTATP);
7865   if((st & KBS_DIB) == 0)
7866     return -1;
7867   data = inb(KBDATAP);
7868 
7869   if(data == 0xE0){
7870     shift |= E0ESC;
7871     return 0;
7872   } else if(data & 0x80){
7873     // Key released
7874     data = (shift & E0ESC ? data : data & 0x7F);
7875     shift &= ~(shiftcode[data] | E0ESC);
7876     return 0;
7877   } else if(shift & E0ESC){
7878     // Last character was an E0 escape; or with 0x80
7879     data |= 0x80;
7880     shift &= ~E0ESC;
7881   }
7882 
7883   shift |= shiftcode[data];
7884   shift ^= togglecode[data];
7885   c = charcode[shift & (CTL | SHIFT)][data];
7886   if(shift & CAPSLOCK){
7887     if('a' <= c && c <= 'z')
7888       c += 'A' - 'a';
7889     else if('A' <= c && c <= 'Z')
7890       c += 'a' - 'A';
7891   }
7892   return c;
7893 }
7894 
7895 void
7896 kbdintr(void)
7897 {
7898   consoleintr(kbdgetc);
7899 }
