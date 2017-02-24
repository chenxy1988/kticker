#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "kticker.h"
void *callback1(int argc,char **argv)
{
	printf("call 1\n");

}

void *call2()
{
	printf("call 2\n");

}

void *call3()
{
	printf("call3\n");
}

int main()
{
	unsigned int tmr1,tmr2,tmr3;
    timer_init();
	create_timer(TIMER_ONE,&tmr1);
	create_timer(TIMER_LOOP,&tmr2);
	create_timer(TIMER_LOOP,&tmr3);

	start_timer(tmr1,10,callback1,0,NULL);
	start_timer(tmr2,8,call2,0,NULL);
	start_timer(tmr3,13,call3,0,NULL);
	
	while(1){
		sleep(100);
	}
	

	return 0;
}
