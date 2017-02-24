#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "kticker.h"
void *callback1(int argc,char **argv)
{
	printf("call 1 argc = %d,argv=%s\n",argc,argv[0]);

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
	char *h="help";
    timer_init();
	unsigned int c=0;
	while(1){
		create_timer(TIMER_ONE,&tmr1);
		create_timer(TIMER_LOOP,&tmr2);
		create_timer(TIMER_LOOP,&tmr3);
		printf("timer id 1 =%d 2=%d 3= %d \n",tmr1,tmr2,tmr3);

		start_timer(tmr1,50,callback1,1,&h);
		start_timer(tmr2,80,call2,0,NULL);
		start_timer(tmr3,100,call3,0,NULL);
		sleep(10);
		delete_timer(tmr1);
		sleep(20);
		delete_timer(tmr2);
		sleep(30);
		delete_timer(tmr3);
		c++;
		printf("All timer delete 1s restart %d times\n",c);
		sleep(1);
		
	}
	

	return 0;
}
