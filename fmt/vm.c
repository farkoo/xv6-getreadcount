1700 #include "param.h"
1701 #include "types.h"
1702 #include "defs.h"
1703 #include "x86.h"
1704 #include "memlayout.h"
1705 #include "mmu.h"
1706 #include "proc.h"
1707 #include "elf.h"
1708 
1709 extern char data[];  // defined by kernel.ld
1710 pde_t *kpgdir;  // for use in scheduler()
1711 
1712 // Set up CPU's kernel segment descriptors.
1713 // Run once on entry on each CPU.
1714 void
1715 seginit(void)
1716 {
1717   struct cpu *c;
1718 
1719   // Map "logical" addresses to virtual addresses using identity map.
1720   // Cannot share a CODE descriptor for both kernel and user
1721   // because it would have to have DPL_USR, but the CPU forbids
1722   // an interrupt from CPL=0 to DPL=3.
1723   c = &cpus[cpuid()];
1724   c->gdt[SEG_KCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, 0);
1725   c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
1726   c->gdt[SEG_UCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER);
1727   c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);
1728   lgdt(c->gdt, sizeof(c->gdt));
1729 }
1730 
1731 // Return the address of the PTE in page table pgdir
1732 // that corresponds to virtual address va.  If alloc!=0,
1733 // create any required page table pages.
1734 static pte_t *
1735 walkpgdir(pde_t *pgdir, const void *va, int alloc)
1736 {
1737   pde_t *pde;
1738   pte_t *pgtab;
1739 
1740   pde = &pgdir[PDX(va)];
1741   if(*pde & PTE_P){
1742     pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
1743   } else {
1744     if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
1745       return 0;
1746     // Make sure all those PTE_P bits are zero.
1747     memset(pgtab, 0, PGSIZE);
1748     // The permissions here are overly generous, but they can
1749     // be further restricted by the permissions in the page table
1750     // entries, if necessary.
1751     *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
1752   }
1753   return &pgtab[PTX(va)];
1754 }
1755 
1756 // Create PTEs for virtual addresses starting at va that refer to
1757 // physical addresses starting at pa. va and size might not
1758 // be page-aligned.
1759 static int
1760 mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
1761 {
1762   char *a, *last;
1763   pte_t *pte;
1764 
1765   a = (char*)PGROUNDDOWN((uint)va);
1766   last = (char*)PGROUNDDOWN(((uint)va) + size - 1);
1767   for(;;){
1768     if((pte = walkpgdir(pgdir, a, 1)) == 0)
1769       return -1;
1770     if(*pte & PTE_P)
1771       panic("remap");
1772     *pte = pa | perm | PTE_P;
1773     if(a == last)
1774       break;
1775     a += PGSIZE;
1776     pa += PGSIZE;
1777   }
1778   return 0;
1779 }
1780 
1781 // There is one page table per process, plus one that's used when
1782 // a CPU is not running any process (kpgdir). The kernel uses the
1783 // current process's page table during system calls and interrupts;
1784 // page protection bits prevent user code from using the kernel's
1785 // mappings.
1786 //
1787 // setupkvm() and exec() set up every page table like this:
1788 //
1789 //   0..KERNBASE: user memory (text+data+stack+heap), mapped to
1790 //                phys memory allocated by the kernel
1791 //   KERNBASE..KERNBASE+EXTMEM: mapped to 0..EXTMEM (for I/O space)
1792 //   KERNBASE+EXTMEM..data: mapped to EXTMEM..V2P(data)
1793 //                for the kernel's instructions and r/o data
1794 //   data..KERNBASE+PHYSTOP: mapped to V2P(data)..PHYSTOP,
1795 //                                  rw data + free physical memory
1796 //   0xfe000000..0: mapped direct (devices such as ioapic)
1797 //
1798 // The kernel allocates physical memory for its heap and for user memory
1799 // between V2P(end) and the end of physical memory (PHYSTOP)
1800 // (directly addressable from end..P2V(PHYSTOP)).
1801 
1802 // This table defines the kernel's mappings, which are present in
1803 // every process's page table.
1804 static struct kmap {
1805   void *virt;
1806   uint phys_start;
1807   uint phys_end;
1808   int perm;
1809 } kmap[] = {
1810  { (void*)KERNBASE, 0,             EXTMEM,    PTE_W}, // I/O space
1811  { (void*)KERNLINK, V2P(KERNLINK), V2P(data), 0},     // kern text+rodata
1812  { (void*)data,     V2P(data),     PHYSTOP,   PTE_W}, // kern data+memory
1813  { (void*)DEVSPACE, DEVSPACE,      0,         PTE_W}, // more devices
1814 };
1815 
1816 // Set up kernel part of a page table.
1817 pde_t*
1818 setupkvm(void)
1819 {
1820   pde_t *pgdir;
1821   struct kmap *k;
1822 
1823   if((pgdir = (pde_t*)kalloc()) == 0)
1824     return 0;
1825   memset(pgdir, 0, PGSIZE);
1826   if (P2V(PHYSTOP) > (void*)DEVSPACE)
1827     panic("PHYSTOP too high");
1828   for(k = kmap; k < &kmap[NELEM(kmap)]; k++)
1829     if(mappages(pgdir, k->virt, k->phys_end - k->phys_start,
1830                 (uint)k->phys_start, k->perm) < 0) {
1831       freevm(pgdir);
1832       return 0;
1833     }
1834   return pgdir;
1835 }
1836 
1837 // Allocate one page table for the machine for the kernel address
1838 // space for scheduler processes.
1839 void
1840 kvmalloc(void)
1841 {
1842   kpgdir = setupkvm();
1843   switchkvm();
1844 }
1845 
1846 
1847 
1848 
1849 
1850 // Switch h/w page table register to the kernel-only page table,
1851 // for when no process is running.
1852 void
1853 switchkvm(void)
1854 {
1855   lcr3(V2P(kpgdir));   // switch to the kernel page table
1856 }
1857 
1858 // Switch TSS and h/w page table to correspond to process p.
1859 void
1860 switchuvm(struct proc *p)
1861 {
1862   if(p == 0)
1863     panic("switchuvm: no process");
1864   if(p->kstack == 0)
1865     panic("switchuvm: no kstack");
1866   if(p->pgdir == 0)
1867     panic("switchuvm: no pgdir");
1868 
1869   pushcli();
1870   mycpu()->gdt[SEG_TSS] = SEG16(STS_T32A, &mycpu()->ts,
1871                                 sizeof(mycpu()->ts)-1, 0);
1872   mycpu()->gdt[SEG_TSS].s = 0;
1873   mycpu()->ts.ss0 = SEG_KDATA << 3;
1874   mycpu()->ts.esp0 = (uint)p->kstack + KSTACKSIZE;
1875   // setting IOPL=0 in eflags *and* iomb beyond the tss segment limit
1876   // forbids I/O instructions (e.g., inb and outb) from user space
1877   mycpu()->ts.iomb = (ushort) 0xFFFF;
1878   ltr(SEG_TSS << 3);
1879   lcr3(V2P(p->pgdir));  // switch to process's address space
1880   popcli();
1881 }
1882 
1883 // Load the initcode into address 0 of pgdir.
1884 // sz must be less than a page.
1885 void
1886 inituvm(pde_t *pgdir, char *init, uint sz)
1887 {
1888   char *mem;
1889 
1890   if(sz >= PGSIZE)
1891     panic("inituvm: more than a page");
1892   mem = kalloc();
1893   memset(mem, 0, PGSIZE);
1894   mappages(pgdir, 0, PGSIZE, V2P(mem), PTE_W|PTE_U);
1895   memmove(mem, init, sz);
1896 }
1897 
1898 
1899 
1900 // Load a program segment into pgdir.  addr must be page-aligned
1901 // and the pages from addr to addr+sz must already be mapped.
1902 int
1903 loaduvm(pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz)
1904 {
1905   uint i, pa, n;
1906   pte_t *pte;
1907 
1908   if((uint) addr % PGSIZE != 0)
1909     panic("loaduvm: addr must be page aligned");
1910   for(i = 0; i < sz; i += PGSIZE){
1911     if((pte = walkpgdir(pgdir, addr+i, 0)) == 0)
1912       panic("loaduvm: address should exist");
1913     pa = PTE_ADDR(*pte);
1914     if(sz - i < PGSIZE)
1915       n = sz - i;
1916     else
1917       n = PGSIZE;
1918     if(readi(ip, P2V(pa), offset+i, n) != n)
1919       return -1;
1920   }
1921   return 0;
1922 }
1923 
1924 // Allocate page tables and physical memory to grow process from oldsz to
1925 // newsz, which need not be page aligned.  Returns new size or 0 on error.
1926 int
1927 allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
1928 {
1929   char *mem;
1930   uint a;
1931 
1932   if(newsz >= KERNBASE)
1933     return 0;
1934   if(newsz < oldsz)
1935     return oldsz;
1936 
1937   a = PGROUNDUP(oldsz);
1938   for(; a < newsz; a += PGSIZE){
1939     mem = kalloc();
1940     if(mem == 0){
1941       cprintf("allocuvm out of memory\n");
1942       deallocuvm(pgdir, newsz, oldsz);
1943       return 0;
1944     }
1945     memset(mem, 0, PGSIZE);
1946     if(mappages(pgdir, (char*)a, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
1947       cprintf("allocuvm out of memory (2)\n");
1948       deallocuvm(pgdir, newsz, oldsz);
1949       kfree(mem);
1950       return 0;
1951     }
1952   }
1953   return newsz;
1954 }
1955 
1956 // Deallocate user pages to bring the process size from oldsz to
1957 // newsz.  oldsz and newsz need not be page-aligned, nor does newsz
1958 // need to be less than oldsz.  oldsz can be larger than the actual
1959 // process size.  Returns the new process size.
1960 int
1961 deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
1962 {
1963   pte_t *pte;
1964   uint a, pa;
1965 
1966   if(newsz >= oldsz)
1967     return oldsz;
1968 
1969   a = PGROUNDUP(newsz);
1970   for(; a  < oldsz; a += PGSIZE){
1971     pte = walkpgdir(pgdir, (char*)a, 0);
1972     if(!pte)
1973       a = PGADDR(PDX(a) + 1, 0, 0) - PGSIZE;
1974     else if((*pte & PTE_P) != 0){
1975       pa = PTE_ADDR(*pte);
1976       if(pa == 0)
1977         panic("kfree");
1978       char *v = P2V(pa);
1979       kfree(v);
1980       *pte = 0;
1981     }
1982   }
1983   return newsz;
1984 }
1985 
1986 
1987 
1988 
1989 
1990 
1991 
1992 
1993 
1994 
1995 
1996 
1997 
1998 
1999 
2000 // Free a page table and all the physical memory pages
2001 // in the user part.
2002 void
2003 freevm(pde_t *pgdir)
2004 {
2005   uint i;
2006 
2007   if(pgdir == 0)
2008     panic("freevm: no pgdir");
2009   deallocuvm(pgdir, KERNBASE, 0);
2010   for(i = 0; i < NPDENTRIES; i++){
2011     if(pgdir[i] & PTE_P){
2012       char * v = P2V(PTE_ADDR(pgdir[i]));
2013       kfree(v);
2014     }
2015   }
2016   kfree((char*)pgdir);
2017 }
2018 
2019 // Clear PTE_U on a page. Used to create an inaccessible
2020 // page beneath the user stack.
2021 void
2022 clearpteu(pde_t *pgdir, char *uva)
2023 {
2024   pte_t *pte;
2025 
2026   pte = walkpgdir(pgdir, uva, 0);
2027   if(pte == 0)
2028     panic("clearpteu");
2029   *pte &= ~PTE_U;
2030 }
2031 
2032 // Given a parent process's page table, create a copy
2033 // of it for a child.
2034 pde_t*
2035 copyuvm(pde_t *pgdir, uint sz)
2036 {
2037   pde_t *d;
2038   pte_t *pte;
2039   uint pa, i, flags;
2040   char *mem;
2041 
2042   if((d = setupkvm()) == 0)
2043     return 0;
2044   for(i = 0; i < sz; i += PGSIZE){
2045     if((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
2046       panic("copyuvm: pte should exist");
2047     if(!(*pte & PTE_P))
2048       panic("copyuvm: page not present");
2049     pa = PTE_ADDR(*pte);
2050     flags = PTE_FLAGS(*pte);
2051     if((mem = kalloc()) == 0)
2052       goto bad;
2053     memmove(mem, (char*)P2V(pa), PGSIZE);
2054     if(mappages(d, (void*)i, PGSIZE, V2P(mem), flags) < 0) {
2055       kfree(mem);
2056       goto bad;
2057     }
2058   }
2059   return d;
2060 
2061 bad:
2062   freevm(d);
2063   return 0;
2064 }
2065 
2066 
2067 
2068 
2069 
2070 
2071 
2072 
2073 
2074 
2075 
2076 
2077 
2078 
2079 
2080 
2081 
2082 
2083 
2084 
2085 
2086 
2087 
2088 
2089 
2090 
2091 
2092 
2093 
2094 
2095 
2096 
2097 
2098 
2099 
2100 // Map user virtual address to kernel address.
2101 char*
2102 uva2ka(pde_t *pgdir, char *uva)
2103 {
2104   pte_t *pte;
2105 
2106   pte = walkpgdir(pgdir, uva, 0);
2107   if((*pte & PTE_P) == 0)
2108     return 0;
2109   if((*pte & PTE_U) == 0)
2110     return 0;
2111   return (char*)P2V(PTE_ADDR(*pte));
2112 }
2113 
2114 // Copy len bytes from p to user address va in page table pgdir.
2115 // Most useful when pgdir is not the current page table.
2116 // uva2ka ensures this only works for PTE_U pages.
2117 int
2118 copyout(pde_t *pgdir, uint va, void *p, uint len)
2119 {
2120   char *buf, *pa0;
2121   uint n, va0;
2122 
2123   buf = (char*)p;
2124   while(len > 0){
2125     va0 = (uint)PGROUNDDOWN(va);
2126     pa0 = uva2ka(pgdir, (char*)va0);
2127     if(pa0 == 0)
2128       return -1;
2129     n = PGSIZE - (va - va0);
2130     if(n > len)
2131       n = len;
2132     memmove(pa0 + (va - va0), buf, n);
2133     len -= n;
2134     buf += n;
2135     va = va0 + PGSIZE;
2136   }
2137   return 0;
2138 }
2139 
2140 
2141 
2142 
2143 
2144 
2145 
2146 
2147 
2148 
2149 
2150 // Blank page.
2151 
2152 
2153 
2154 
2155 
2156 
2157 
2158 
2159 
2160 
2161 
2162 
2163 
2164 
2165 
2166 
2167 
2168 
2169 
2170 
2171 
2172 
2173 
2174 
2175 
2176 
2177 
2178 
2179 
2180 
2181 
2182 
2183 
2184 
2185 
2186 
2187 
2188 
2189 
2190 
2191 
2192 
2193 
2194 
2195 
2196 
2197 
2198 
2199 
2200 // Blank page.
2201 
2202 
2203 
2204 
2205 
2206 
2207 
2208 
2209 
2210 
2211 
2212 
2213 
2214 
2215 
2216 
2217 
2218 
2219 
2220 
2221 
2222 
2223 
2224 
2225 
2226 
2227 
2228 
2229 
2230 
2231 
2232 
2233 
2234 
2235 
2236 
2237 
2238 
2239 
2240 
2241 
2242 
2243 
2244 
2245 
2246 
2247 
2248 
2249 
2250 // Blank page.
2251 
2252 
2253 
2254 
2255 
2256 
2257 
2258 
2259 
2260 
2261 
2262 
2263 
2264 
2265 
2266 
2267 
2268 
2269 
2270 
2271 
2272 
2273 
2274 
2275 
2276 
2277 
2278 
2279 
2280 
2281 
2282 
2283 
2284 
2285 
2286 
2287 
2288 
2289 
2290 
2291 
2292 
2293 
2294 
2295 
2296 
2297 
2298 
2299 
