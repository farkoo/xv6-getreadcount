#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
int 
main(void){
	printf(1,"Hi, the number of read syscall is %d so far!\n", getreadcount());
	exit();
}
