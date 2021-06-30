6900 #include "types.h"
6901 #include "x86.h"
6902 
6903 void*
6904 memset(void *dst, int c, uint n)
6905 {
6906   if ((int)dst%4 == 0 && n%4 == 0){
6907     c &= 0xFF;
6908     stosl(dst, (c<<24)|(c<<16)|(c<<8)|c, n/4);
6909   } else
6910     stosb(dst, c, n);
6911   return dst;
6912 }
6913 
6914 int
6915 memcmp(const void *v1, const void *v2, uint n)
6916 {
6917   const uchar *s1, *s2;
6918 
6919   s1 = v1;
6920   s2 = v2;
6921   while(n-- > 0){
6922     if(*s1 != *s2)
6923       return *s1 - *s2;
6924     s1++, s2++;
6925   }
6926 
6927   return 0;
6928 }
6929 
6930 void*
6931 memmove(void *dst, const void *src, uint n)
6932 {
6933   const char *s;
6934   char *d;
6935 
6936   s = src;
6937   d = dst;
6938   if(s < d && s + n > d){
6939     s += n;
6940     d += n;
6941     while(n-- > 0)
6942       *--d = *--s;
6943   } else
6944     while(n-- > 0)
6945       *d++ = *s++;
6946 
6947   return dst;
6948 }
6949 
6950 // memcpy exists to placate GCC.  Use memmove.
6951 void*
6952 memcpy(void *dst, const void *src, uint n)
6953 {
6954   return memmove(dst, src, n);
6955 }
6956 
6957 int
6958 strncmp(const char *p, const char *q, uint n)
6959 {
6960   while(n > 0 && *p && *p == *q)
6961     n--, p++, q++;
6962   if(n == 0)
6963     return 0;
6964   return (uchar)*p - (uchar)*q;
6965 }
6966 
6967 char*
6968 strncpy(char *s, const char *t, int n)
6969 {
6970   char *os;
6971 
6972   os = s;
6973   while(n-- > 0 && (*s++ = *t++) != 0)
6974     ;
6975   while(n-- > 0)
6976     *s++ = 0;
6977   return os;
6978 }
6979 
6980 // Like strncpy but guaranteed to NUL-terminate.
6981 char*
6982 safestrcpy(char *s, const char *t, int n)
6983 {
6984   char *os;
6985 
6986   os = s;
6987   if(n <= 0)
6988     return os;
6989   while(--n > 0 && (*s++ = *t++) != 0)
6990     ;
6991   *s = 0;
6992   return os;
6993 }
6994 
6995 
6996 
6997 
6998 
6999 
7000 int
7001 strlen(const char *s)
7002 {
7003   int n;
7004 
7005   for(n = 0; s[n]; n++)
7006     ;
7007   return n;
7008 }
7009 
7010 
7011 
7012 
7013 
7014 
7015 
7016 
7017 
7018 
7019 
7020 
7021 
7022 
7023 
7024 
7025 
7026 
7027 
7028 
7029 
7030 
7031 
7032 
7033 
7034 
7035 
7036 
7037 
7038 
7039 
7040 
7041 
7042 
7043 
7044 
7045 
7046 
7047 
7048 
7049 
