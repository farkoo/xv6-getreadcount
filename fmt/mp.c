7200 // Multiprocessor support
7201 // Search memory for MP description structures.
7202 // http://developer.intel.com/design/pentium/datashts/24201606.pdf
7203 
7204 #include "types.h"
7205 #include "defs.h"
7206 #include "param.h"
7207 #include "memlayout.h"
7208 #include "mp.h"
7209 #include "x86.h"
7210 #include "mmu.h"
7211 #include "proc.h"
7212 
7213 struct cpu cpus[NCPU];
7214 int ncpu;
7215 uchar ioapicid;
7216 
7217 static uchar
7218 sum(uchar *addr, int len)
7219 {
7220   int i, sum;
7221 
7222   sum = 0;
7223   for(i=0; i<len; i++)
7224     sum += addr[i];
7225   return sum;
7226 }
7227 
7228 // Look for an MP structure in the len bytes at addr.
7229 static struct mp*
7230 mpsearch1(uint a, int len)
7231 {
7232   uchar *e, *p, *addr;
7233 
7234   addr = P2V(a);
7235   e = addr+len;
7236   for(p = addr; p < e; p += sizeof(struct mp))
7237     if(memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
7238       return (struct mp*)p;
7239   return 0;
7240 }
7241 
7242 
7243 
7244 
7245 
7246 
7247 
7248 
7249 
7250 // Search for the MP Floating Pointer Structure, which according to the
7251 // spec is in one of the following three locations:
7252 // 1) in the first KB of the EBDA;
7253 // 2) in the last KB of system base memory;
7254 // 3) in the BIOS ROM between 0xE0000 and 0xFFFFF.
7255 static struct mp*
7256 mpsearch(void)
7257 {
7258   uchar *bda;
7259   uint p;
7260   struct mp *mp;
7261 
7262   bda = (uchar *) P2V(0x400);
7263   if((p = ((bda[0x0F]<<8)| bda[0x0E]) << 4)){
7264     if((mp = mpsearch1(p, 1024)))
7265       return mp;
7266   } else {
7267     p = ((bda[0x14]<<8)|bda[0x13])*1024;
7268     if((mp = mpsearch1(p-1024, 1024)))
7269       return mp;
7270   }
7271   return mpsearch1(0xF0000, 0x10000);
7272 }
7273 
7274 // Search for an MP configuration table.  For now,
7275 // don't accept the default configurations (physaddr == 0).
7276 // Check for correct signature, calculate the checksum and,
7277 // if correct, check the version.
7278 // To do: check extended table checksum.
7279 static struct mpconf*
7280 mpconfig(struct mp **pmp)
7281 {
7282   struct mpconf *conf;
7283   struct mp *mp;
7284 
7285   if((mp = mpsearch()) == 0 || mp->physaddr == 0)
7286     return 0;
7287   conf = (struct mpconf*) P2V((uint) mp->physaddr);
7288   if(memcmp(conf, "PCMP", 4) != 0)
7289     return 0;
7290   if(conf->version != 1 && conf->version != 4)
7291     return 0;
7292   if(sum((uchar*)conf, conf->length) != 0)
7293     return 0;
7294   *pmp = mp;
7295   return conf;
7296 }
7297 
7298 
7299 
7300 void
7301 mpinit(void)
7302 {
7303   uchar *p, *e;
7304   int ismp;
7305   struct mp *mp;
7306   struct mpconf *conf;
7307   struct mpproc *proc;
7308   struct mpioapic *ioapic;
7309 
7310   if((conf = mpconfig(&mp)) == 0)
7311     panic("Expect to run on an SMP");
7312   ismp = 1;
7313   lapic = (uint*)conf->lapicaddr;
7314   for(p=(uchar*)(conf+1), e=(uchar*)conf+conf->length; p<e; ){
7315     switch(*p){
7316     case MPPROC:
7317       proc = (struct mpproc*)p;
7318       if(ncpu < NCPU) {
7319         cpus[ncpu].apicid = proc->apicid;  // apicid may differ from ncpu
7320         ncpu++;
7321       }
7322       p += sizeof(struct mpproc);
7323       continue;
7324     case MPIOAPIC:
7325       ioapic = (struct mpioapic*)p;
7326       ioapicid = ioapic->apicno;
7327       p += sizeof(struct mpioapic);
7328       continue;
7329     case MPBUS:
7330     case MPIOINTR:
7331     case MPLINTR:
7332       p += 8;
7333       continue;
7334     default:
7335       ismp = 0;
7336       break;
7337     }
7338   }
7339   if(!ismp)
7340     panic("Didn't find a suitable machine");
7341 
7342   if(mp->imcrp){
7343     // Bochs doesn't support IMCR, so this doesn't run on Bochs.
7344     // But it would on real hardware.
7345     outb(0x22, 0x70);   // Select IMCR
7346     outb(0x23, inb(0x23) | 1);  // Mask external interrupts.
7347   }
7348 }
7349 
