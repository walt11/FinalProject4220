/*

Author:		John Walter
Project:	4220 Final Project

*/

#ifndef MODULE
#define MODULE
#endif

#ifndef __KERNEL__
#define __KERNEL__
#endif

#include <linux/module.h>
#include <linux/kernel.h>

#include <asm/io.h>
#include <rtai.h>
#include <rtai_lxrt.h>
#include <rtai_sched.h>
#include <rtai_sem.h>
#include <linux/time.h>
#include <rtai_fifos.h>

MODULE_LICENSE("GPL");
unsigned long *ptr, *PBDR, *PBDDR, *PFDR, *PFDDR, *GPIOBIntType1, *GPIOBIntType2, *GPIOBEOI, *GPIOBIntEn, *IntStsB, *RawIntStsB;
SEM sem;
static RT_TASK mytask, LED_Task;
RTIME period;
int data[6][2];
int b1=0, b2=0, b3=0;
static void MainTask(int t){
	//int shift=0;
	//int bit=0;
	int value=0;
	unsigned long *ptr, *PBDR, *PBDDR;
	ptr = (unsigned long *)__ioremap(0x80840000,4096,0);
	PBDR = ptr + 1;
	PBDDR = ptr + 5;
	*PBDDR &= 0xFFFFFFFE;
	while(1){
		//int data[6][2];
		/*for(i=0;i<6;i++){
			for(j=0;j<2;j++){
				data[i][j] = value;
			}
		}*/
		value++;
		rt_sem_wait(&sem);
			data[5][0] = value;
			rtf_put(0,&data,sizeof(data));
		rt_sem_signal(&sem);
		rt_sleep(nano2count(1000000000));
	}

	//while(1){
		//struct timeval t;
			// continuously check the bit corresponding to the button to see if it changes
			//bit = ((*PBDR >> shift) & 0x01);
			// if the bit goes from 1 to 0 (the button was pressed)
			//if(bit == 0){
				//do_gettimeofday(&t);
				//int sec = t.tv_sec;
				//int usec = t.tv_usec;
				//rtf_put(0, &sec ,sizeof(sec));
				//rtf_put(0,&usec,sizeof(usec));

		//}
}


static void my_handler(unsigned irq_num, void *cookie){
	rt_disable_irq(59);
	struct timeval t;
	do_gettimeofday(&t);
	int sec = t.tv_sec;

	int i, bit, button;
	i=0;
	bit=0;
	button=0;
	for(i=0;i<3;i++){
		bit = ((*RawIntStsB >> i) & 0x01);
		if(bit != 0){
			button = i;
		}
	}
	//printk("Button %d was pressed\n",button);
	rt_sem_wait(&sem);
	if((button == 0)){
			if(data[0][0] == 1){ // if "switch" is on
				data[0][0] = 0;	 // button press indicates switch flips to off
			}else{
				data[0][0] = 1;
			}
			data[0][1] = sec;

		*GPIOBEOI |= 0x01;

	}else if((button == 1)){
			if(data[1][0] == 1){ // if "switch" is on
				data[1][0] = 0;	 // button press indicates switch flips to off
			}else{
				data[1][0] = 1;
			}
			data[1][1] = sec;

		*GPIOBEOI |= 0x02;

	}else if((button == 2)){
			if(data[2][0] == 1){ // if "switch" is on
				data[2][0] = 0;	 // button press indicates switch flips to off
			}else{
				data[2][0] = 1;
			}
			data[2][1] = sec;

		*GPIOBEOI |= 0x04;
	}else{}
	rt_sem_signal(&sem);
	rt_enable_irq(59);
}

static void LEDTask(int x){
	struct timeval t;
	int sec =0;
	int bit=0;
	while(1){
		// get LED 1 current status
		//bit = ((*PBDR >> 5) & 0x01);


		bit = ((*PBDR >> 5)&0x01);
		if(bit != data[3][0]){ // if the bit has changed
			do_gettimeofday(&t);
			sec = t.tv_sec;
			rt_sem_wait(&sem);
			data[3][0] = bit;
			data[3][1] = sec;
			rt_sem_signal(&sem);
		}


		// get LED 2 current status
		bit = ((*PBDR >> 6) & 0x01);
		if(bit != data[4][0]){ // if the bit has changed
			do_gettimeofday(&t);
			sec = t.tv_sec;
			rt_sem_wait(&sem);
			data[4][0] = bit;
			data[4][1] = sec;
			rt_sem_signal(&sem);
		}

		rt_sleep(nano2count(50000000));
	}
}

int init_module(void){
	ptr = (unsigned long *)__ioremap(0x80840000,4096,0);
	PBDR = ptr+1;
	PBDDR = ptr+5;
	GPIOBIntType1	= (unsigned long *)__ioremap(0x808400AC, 4096,0);
	GPIOBIntType2 	= (unsigned long *)__ioremap(0x808400B0, 4096,0);
	GPIOBEOI 		= (unsigned long *)__ioremap(0x808400B4, 4096,0);
	GPIOBIntEn 		= (unsigned long *)__ioremap(0x808400B8, 4096,0);
	IntStsB 		= (unsigned long *)__ioremap(0x808400BC, 4096,0);
	RawIntStsB	 	= (unsigned long *)__ioremap(0x808400C0, 4096,0);

	*PBDDR &= 0xF8;			//1111 1000
	*PBDDR &= 0x60; // set leds as output
	*GPIOBIntEn |= 0x07;	//0000 0111
	*GPIOBIntType1 |= 0x07;	//0000 0111
	*GPIOBIntType2 &= 0xF8; //1111 1000

	int dummy[6][2];
	int buf=0;
	rt_set_periodic_mode();
	period = start_rt_timer(nano2count(1000000));
	rt_sem_init(&sem,1);
	rtf_create(0,sizeof(dummy));
	rtf_create(1,sizeof(int));
	
	// this waits for the userspace program to become ready so that the data being
	// collected does not build up in fifo 0, when a 1 is read from the fifo, then
	// the tasks are initialzied and started
	while(buf!=1){
		rtf_get(1,&buf,sizeof(int));
	}

	rt_request_irq(59, my_handler, 0, 1);
	rt_enable_irq(59);


	// initialize a semaphore

	// initialize real time task and make it periodic
	rt_task_init(&mytask, MainTask,0,256,0,0,0);
	rt_task_init(&LED_Task, LEDTask, 0, 256,0,0,0);
	rt_task_resume(&mytask);
	rt_task_resume(&LED_Task);
	return 0;
}

void cleanup_module(void){
	rt_task_delete(&mytask);
	rt_task_delete(&LED_Task);
	stop_rt_timer();
	rt_disable_irq(59);
	rt_sem_delete(&sem);
}
