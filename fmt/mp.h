7050 // See MultiProcessor Specification Version 1.[14]
7051 
7052 struct mp {             // floating pointer
7053   uchar signature[4];           // "_MP_"
7054   void *physaddr;               // phys addr of MP config table
7055   uchar length;                 // 1
7056   uchar specrev;                // [14]
7057   uchar checksum;               // all bytes must add up to 0
7058   uchar type;                   // MP system config type
7059   uchar imcrp;
7060   uchar reserved[3];
7061 };
7062 
7063 struct mpconf {         // configuration table header
7064   uchar signature[4];           // "PCMP"
7065   ushort length;                // total table length
7066   uchar version;                // [14]
7067   uchar checksum;               // all bytes must add up to 0
7068   uchar product[20];            // product id
7069   uint *oemtable;               // OEM table pointer
7070   ushort oemlength;             // OEM table length
7071   ushort entry;                 // entry count
7072   uint *lapicaddr;              // address of local APIC
7073   ushort xlength;               // extended table length
7074   uchar xchecksum;              // extended table checksum
7075   uchar reserved;
7076 };
7077 
7078 struct mpproc {         // processor table entry
7079   uchar type;                   // entry type (0)
7080   uchar apicid;                 // local APIC id
7081   uchar version;                // local APIC verison
7082   uchar flags;                  // CPU flags
7083     #define MPBOOT 0x02           // This proc is the bootstrap processor.
7084   uchar signature[4];           // CPU signature
7085   uint feature;                 // feature flags from CPUID instruction
7086   uchar reserved[8];
7087 };
7088 
7089 struct mpioapic {       // I/O APIC table entry
7090   uchar type;                   // entry type (2)
7091   uchar apicno;                 // I/O APIC id
7092   uchar version;                // I/O APIC version
7093   uchar flags;                  // I/O APIC flags
7094   uint *addr;                  // I/O APIC address
7095 };
7096 
7097 
7098 
7099 
7100 // Table entry types
7101 #define MPPROC    0x00  // One per processor
7102 #define MPBUS     0x01  // One per bus
7103 #define MPIOAPIC  0x02  // One per I/O APIC
7104 #define MPIOINTR  0x03  // One per bus interrupt source
7105 #define MPLINTR   0x04  // One per system interrupt source
7106 
7107 
7108 
7109 
7110 
7111 
7112 
7113 
7114 
7115 
7116 
7117 
7118 
7119 
7120 
7121 
7122 
7123 
7124 
7125 
7126 
7127 
7128 
7129 
7130 
7131 
7132 
7133 
7134 
7135 
7136 
7137 
7138 
7139 
7140 
7141 
7142 
7143 
7144 
7145 
7146 
7147 
7148 
7149 
7150 // Blank page.
7151 
7152 
7153 
7154 
7155 
7156 
7157 
7158 
7159 
7160 
7161 
7162 
7163 
7164 
7165 
7166 
7167 
7168 
7169 
7170 
7171 
7172 
7173 
7174 
7175 
7176 
7177 
7178 
7179 
7180 
7181 
7182 
7183 
7184 
7185 
7186 
7187 
7188 
7189 
7190 
7191 
7192 
7193 
7194 
7195 
7196 
7197 
7198 
7199 
