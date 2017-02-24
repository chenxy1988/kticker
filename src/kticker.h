#ifndef __KTICKER_H__
#define __KTICKER_H__

/**Timer configurations **/
#define MAX_TIMER_COUNT		32 

#define TIMER_RET_OK	0
#define TIMER_RET_FAIL	-1

typedef enum{
	TIMER_ONE = 1, 
	TIMER_LOOP,
}e_timer_type;

typedef enum{
	TIMER_IDLE = 0,
	TIMER_START,
	TIMER_STOP,
}e_timer_stats;

typedef struct tmr_node{
	unsigned int timer_id;
	unsigned int module_id;
	e_timer_type	timer_type;
	e_timer_stats	timer_stats;
	int tick_offset;
	unsigned short int tick_limit;
	void (*callbackfunction)(int argc,char **argv);
	int argc;
	char **argv;
	struct tmr_node *p_pre;
	struct tmr_node *p_next;

}tmr_node_t;


typedef struct{
	unsigned int idle_count;
	unsigned int use_count;
	tmr_node_t *tmr_idle;
	tmr_node_t *tmr_use;
}tmr_list_t;


int timer_init();
int create_timer(e_timer_type type,unsigned int *t_id);
int delete_timer(unsigned int timer_id);
int start_timer(int timer_id,unsigned int count,void *func,int argc,char **argv);



#endif
