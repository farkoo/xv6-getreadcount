7700 // PC keyboard interface constants
7701 
7702 #define KBSTATP         0x64    // kbd controller status port(I)
7703 #define KBS_DIB         0x01    // kbd data in buffer
7704 #define KBDATAP         0x60    // kbd data port(I)
7705 
7706 #define NO              0
7707 
7708 #define SHIFT           (1<<0)
7709 #define CTL             (1<<1)
7710 #define ALT             (1<<2)
7711 
7712 #define CAPSLOCK        (1<<3)
7713 #define NUMLOCK         (1<<4)
7714 #define SCROLLLOCK      (1<<5)
7715 
7716 #define E0ESC           (1<<6)
7717 
7718 // Special keycodes
7719 #define KEY_HOME        0xE0
7720 #define KEY_END         0xE1
7721 #define KEY_UP          0xE2
7722 #define KEY_DN          0xE3
7723 #define KEY_LF          0xE4
7724 #define KEY_RT          0xE5
7725 #define KEY_PGUP        0xE6
7726 #define KEY_PGDN        0xE7
7727 #define KEY_INS         0xE8
7728 #define KEY_DEL         0xE9
7729 
7730 // C('A') == Control-A
7731 #define C(x) (x - '@')
7732 
7733 static uchar shiftcode[256] =
7734 {
7735   [0x1D] CTL,
7736   [0x2A] SHIFT,
7737   [0x36] SHIFT,
7738   [0x38] ALT,
7739   [0x9D] CTL,
7740   [0xB8] ALT
7741 };
7742 
7743 static uchar togglecode[256] =
7744 {
7745   [0x3A] CAPSLOCK,
7746   [0x45] NUMLOCK,
7747   [0x46] SCROLLLOCK
7748 };
7749 
7750 static uchar normalmap[256] =
7751 {
7752   NO,   0x1B, '1',  '2',  '3',  '4',  '5',  '6',  // 0x00
7753   '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
7754   'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  // 0x10
7755   'o',  'p',  '[',  ']',  '\n', NO,   'a',  's',
7756   'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  // 0x20
7757   '\'', '`',  NO,   '\\', 'z',  'x',  'c',  'v',
7758   'b',  'n',  'm',  ',',  '.',  '/',  NO,   '*',  // 0x30
7759   NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
7760   NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
7761   '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
7762   '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
7763   [0x9C] '\n',      // KP_Enter
7764   [0xB5] '/',       // KP_Div
7765   [0xC8] KEY_UP,    [0xD0] KEY_DN,
7766   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
7767   [0xCB] KEY_LF,    [0xCD] KEY_RT,
7768   [0x97] KEY_HOME,  [0xCF] KEY_END,
7769   [0xD2] KEY_INS,   [0xD3] KEY_DEL
7770 };
7771 
7772 static uchar shiftmap[256] =
7773 {
7774   NO,   033,  '!',  '@',  '#',  '$',  '%',  '^',  // 0x00
7775   '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
7776   'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  // 0x10
7777   'O',  'P',  '{',  '}',  '\n', NO,   'A',  'S',
7778   'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  // 0x20
7779   '"',  '~',  NO,   '|',  'Z',  'X',  'C',  'V',
7780   'B',  'N',  'M',  '<',  '>',  '?',  NO,   '*',  // 0x30
7781   NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
7782   NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
7783   '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
7784   '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
7785   [0x9C] '\n',      // KP_Enter
7786   [0xB5] '/',       // KP_Div
7787   [0xC8] KEY_UP,    [0xD0] KEY_DN,
7788   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
7789   [0xCB] KEY_LF,    [0xCD] KEY_RT,
7790   [0x97] KEY_HOME,  [0xCF] KEY_END,
7791   [0xD2] KEY_INS,   [0xD3] KEY_DEL
7792 };
7793 
7794 
7795 
7796 
7797 
7798 
7799 
7800 static uchar ctlmap[256] =
7801 {
7802   NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
7803   NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
7804   C('Q'),  C('W'),  C('E'),  C('R'),  C('T'),  C('Y'),  C('U'),  C('I'),
7805   C('O'),  C('P'),  NO,      NO,      '\r',    NO,      C('A'),  C('S'),
7806   C('D'),  C('F'),  C('G'),  C('H'),  C('J'),  C('K'),  C('L'),  NO,
7807   NO,      NO,      NO,      C('\\'), C('Z'),  C('X'),  C('C'),  C('V'),
7808   C('B'),  C('N'),  C('M'),  NO,      NO,      C('/'),  NO,      NO,
7809   [0x9C] '\r',      // KP_Enter
7810   [0xB5] C('/'),    // KP_Div
7811   [0xC8] KEY_UP,    [0xD0] KEY_DN,
7812   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
7813   [0xCB] KEY_LF,    [0xCD] KEY_RT,
7814   [0x97] KEY_HOME,  [0xCF] KEY_END,
7815   [0xD2] KEY_INS,   [0xD3] KEY_DEL
7816 };
7817 
7818 
7819 
7820 
7821 
7822 
7823 
7824 
7825 
7826 
7827 
7828 
7829 
7830 
7831 
7832 
7833 
7834 
7835 
7836 
7837 
7838 
7839 
7840 
7841 
7842 
7843 
7844 
7845 
7846 
7847 
7848 
7849 
