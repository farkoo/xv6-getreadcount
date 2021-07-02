# Add getreadcount SysCall to xv6 kernel
To get acquainted with the system call, we want to add the readcount SysCall to the xv6 kernel.

In the first step to change this kernel, we install qemu emulator and apply these changes in this emulator so that the whole system will not be locked in case of any problem.

### Install Qemu
```
sudo apt-get install qemu-kvm
```

### Install the xv6 kernel and launch it on qemu
```
sudo apt-get install git
git clone https://github.com/mit-pdos/xv6-public.git
cd xv6-public
make qemu-nox
```
### int getreadcount(void)
This system call returns the number of system calls from the time the system was booted to the present time.

### Test
Run this code!
```c
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
int 
main(void){
	printf(1,"Hi, the number of read syscall is %d so far!\n", getreadcount());
	exit();
}
```

## Support

**Contact me @:**

e-mail:

* farzanehkoohestani2000@gmail.com

Telegram id:

* [@farzaneh_koohestani](https://t.me/farzaneh_koohestani)

## License
[MIT](https://github.com/farkoo/xv6-getreadcount/blob/master/LICENSE)
&#0169; 
[Farzaneh Koohestani](https://github.com/farkoo)

