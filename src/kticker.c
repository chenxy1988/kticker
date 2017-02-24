#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#include "kticker.h"


#define TMR_ALREADY_INIT	1
#define TMR_NOT_INIT	0
#define DIV			2000

static pthread_t timer_task_id = 0;

struct itimerval timerval;
static pthread_mutex_t	tmr_mutex_val = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t	tmr_mutex_tick = PTHREAD_MUTEX_INITIALIZER;

/**P/V**/
static pthread_cond_t	tmr_tick_cond;
static unsigned int		counter = 0x0;


static unsigned int cur_tick;
static unsigned int old_tick;
static unsigned char isTmrinit = TMR_NOT_INIT;

tmr_list_t	timer_list_table;
//tmr_node_t	timer_node[MAX_TIMER_COUNT];
tmr_node_t	*timer_node = NULL;

static void sig_handler(int timer_sig)
{

	pthread_mutex_lock(&tmr_mutex_tick);
	cur_tick++;
	old_tick = cur_tick;
	counter++;
	if(counter == 1){
		pthread_cond_signal(&tmr_tick_cond);
	}
	pthread_mutex_unlock(&tmr_mutex_tick);
	
}

static void *task_timer()
{
	tmr_node_t *node;

	signal(SIGALRM,sig_handler);
	timerval.it_interval.tv_sec = 0x0;
	timerval.it_interval.tv_usec = DIV * 200;
	timerval.it_value.tv_sec = 0x0;
	timerval.it_value.tv_usec = DIV * 200;
	setitimer(ITIMER_REAL,&timerval,NULL);


	while(1)
	{
		pthread_mutex_lock(&tmr_mutex_tick);
		if(counter == 0){
			pthread_cond_wait(&tmr_tick_cond,&tmr_mutex_tick);
		}
		if(counter > 0){
			counter--;
		}
		pthread_mutex_unlock(&tmr_mutex_tick);

		pthread_mutex_lock(&tmr_mutex_val); //lock list
		node = timer_list_table.tmr_use;
		while(node != NULL)
		{
			if (node->timer_stats == TIMER_START){
				if (old_tick >= node->tick_limit){
					node->callbackfunction(node->argc,node->argv);
					if(node->timer_type ==TIMER_ONE){
						node->timer_stats = TIMER_STOP;
					}else{
						node->tick_limit = old_tick+node->tick_offset;
					}
				
				}
			}

			node = node->p_next;
		}
		pthread_mutex_unlock(&tmr_mutex_val);
		node = NULL;

	}

	printf("task_terminated\n");
	return NULL;
}


/*
* Timer init function
*
*/

int timer_init()
{
	int i=0;

	if(isTmrinit != TMR_NOT_INIT){ //Seems someone called it already, exit
		return TIMER_RET_FAIL;
	}

	timer_node = (tmr_node_t *)malloc(sizeof(tmr_node_t) * MAX_TIMER_COUNT);
	if(timer_node == NULL){ //seems oom
		return TIMER_RET_FAIL;
	}

	old_tick = 0x0;
	
	pthread_mutex_lock(&tmr_mutex_val);
	memset(timer_node,0x0,(sizeof(timer_node) * MAX_TIMER_COUNT));
	for(i = 0;i< MAX_TIMER_COUNT;i++)
	{
		timer_node[i].timer_id = i; //add timer id
		if((MAX_TIMER_COUNT - 1 ) == i){
			timer_node[i].p_next = NULL;
		}
		else
		{
			timer_node[i].p_next = (tmr_node_t *)&timer_node[i+1];
		}

	}

	timer_list_table.idle_count = MAX_TIMER_COUNT;
	timer_list_table.use_count = 0x0;
	timer_list_table.tmr_idle = timer_node;
	timer_list_table.tmr_use = NULL;
	pthread_mutex_unlock(&tmr_mutex_val);

	pthread_cond_init(&tmr_tick_cond,NULL);

	pthread_create(&timer_task_id,NULL,task_timer,NULL);
	isTmrinit = TMR_ALREADY_INIT;

	return TIMER_RET_OK;
}


/*
*	Create a timer
*/
int create_timer(e_timer_type type,unsigned int *t_id)
{
	tmr_node_t *node=NULL;

	if(!t_id)
		return TIMER_RET_FAIL;

	pthread_mutex_lock(&tmr_mutex_val);
	
	if(timer_list_table.idle_count == 0){ //No idle timer node...
		pthread_mutex_unlock(&tmr_mutex_val);
		return TIMER_RET_FAIL;
	}

	node = timer_list_table.tmr_idle;

	timer_list_table.idle_count--;
	if(timer_list_table.idle_count == 0)
	{
		timer_list_table.tmr_idle = NULL;
	}//list is full
	else
	{
		timer_list_table.tmr_idle = node->p_next; //idle list pointer point to next node.
	}

	timer_list_table.use_count++;

	if(timer_list_table.tmr_use == NULL){ //I am the first :)
		node->p_pre = NULL;
		node->p_next = NULL;
	}
	else
	{
		node->p_pre = NULL;
		node->p_next = timer_list_table.tmr_use;
		timer_list_table.tmr_use->p_pre = node;
	}

	timer_list_table.tmr_use = node;

	pthread_mutex_unlock(&tmr_mutex_val);

	node->timer_type = type;
	node->timer_stats = TIMER_STOP;
	node->tick_offset = 0x0;
	node->tick_limit = 0x0;
	node->callbackfunction = NULL;
	*t_id = node->timer_id;

	return TIMER_RET_OK;
}

int delete_timer(unsigned int timer_id)
{
	tmr_node_t *node = &timer_node[timer_id];

	if(timer_id > MAX_TIMER_COUNT){
		return TIMER_RET_FAIL;//timer id invalid
	}

	if(timer_node[timer_id].timer_stats == TIMER_IDLE){
		return TIMER_RET_FAIL; //timer stopped already
	}

	pthread_mutex_lock(&tmr_mutex_val);

	node = &timer_node[timer_id];

	node->timer_stats = TIMER_IDLE;

	if((node->p_next == NULL) && (node->p_pre == NULL)){
		timer_list_table.tmr_use = NULL;
	}else if((node->p_next != NULL) && (node->p_pre != NULL)){
		node->p_pre->p_next = node->p_next;
		node->p_next->p_pre = node->p_pre; //remove node from use list
	}else{
		if(node->p_next == NULL){
			node->p_pre->p_next = NULL;
		}else{
			node->p_next->p_pre = NULL;
		}
	}

	if(timer_list_table.tmr_idle != NULL){
		node->p_next = timer_list_table.tmr_idle;
		timer_list_table.tmr_idle->p_pre = node;
	}else{
		node->p_next = NULL;
		node->p_pre = NULL;
	}
	timer_list_table.tmr_idle = node;

	timer_list_table.use_count--;
	timer_list_table.idle_count++;
	pthread_mutex_unlock(&tmr_mutex_val);

	return TIMER_RET_OK;
}

int start_timer(int timer_id,unsigned int count,void *func,int argc,char **argv)
{
	unsigned int tick_offset = 0x0;
	unsigned int tmp = 0x0;

	if(timer_id >= MAX_TIMER_COUNT){
		return TIMER_RET_FAIL;
	}

	if(func == NULL){
		return TIMER_RET_FAIL;
	}

	if(timer_node[timer_id].timer_stats == TIMER_IDLE){
		return TIMER_RET_FAIL;
	}

	tick_offset = count/200;
	pthread_mutex_lock(&tmr_mutex_tick);
	tmp = cur_tick;
	pthread_mutex_unlock(&tmr_mutex_tick);

	pthread_mutex_lock(&tmr_mutex_val);
	timer_node[timer_id].timer_stats = TIMER_START;
	timer_node[timer_id].tick_offset = tick_offset;
	timer_node[timer_id].tick_limit = tmp + tick_offset;
	timer_node[timer_id].callbackfunction = func;
	timer_node[timer_id].argc = argc;
	timer_node[timer_id].argv = argv;
	pthread_mutex_unlock(&tmr_mutex_val);
	
	return TIMER_RET_OK;
}
