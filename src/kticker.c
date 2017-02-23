#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <timer.h>

#include "kticker.h"


#define TMR_ALREADY_INIT	1
#define TMR_NOT_INIT	0
static pthread_t timer_task_id = 0;

struct itimerval timerval;
static const long int div = 1000;
static long int tick_count;
static pthread_mutex_t	tmr_mutex_val = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t	tmr_mutex_tick = PTHREAD_MUTEX_INITIALIZER;


static unsigned int cur_tick;
static unsigned int old_tick;
statis unsigned char isTmrinit = TMR_NOT_INIT;

t_list_t	timer_list_table[MAX_TIMER_COUNT];
tmr_node_t	timer_node;


/*
* Timer init function
*
*/

int timer_init()
{
	int ret,i=0;

	if(isTmrinit != TMR_NOT_INIT){ //Seems someone called it already, exit
		return TIMER_RET_FAIL;
	}

	old_tick = 0x0;
	
	pthread_mutex_lock(&tmr_mutex_val);
	memset(timer_node,0x0,sizeof(timer_node));
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
	timer_list_table.tmr_idle = timer_list_table;
	timer_list_table.tmr_use = NULL;
	pthread_mutex_unlock(&tmr_mutex_val);

	return TIMER_RET_OK;
}


/*
*	Create a timer
*/
int timer_create(e_timer_type type,unsigned int *t_id)
{
	tmr_node_t *node=NULL;

	if(!tid)
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
	node->timer_state = TIMER_STOP;
	node->tick_offset = 0x0;
	node->callbackfunction = NULL;
	*t_id = node->timer_id;

	return TIMER_RET_OK;
}

int timer_delete(unsigned int timer_id)
{
	if(timer_id > MAX_TIMER_COUNT){
		return TIMER_RET_FAIL;//timer id invalid
	}
	if(timer_list_table[timer_id].timer_state == TIMER_IDLE){
		return TIMER_RET_FAIL; //timer stopped already
	}

	pthread_mutex_lock(&tmr_mutex_val);
	timer_list_table[timer_id].timer_state == TIMER_STOP;
	pthread_mutex_lock(&tmr_mutex_val);

	return TIMER_RET_OK;
}

static void sig_handler(int timer_sig)
{
	
}

void *task_timer()
{

}


