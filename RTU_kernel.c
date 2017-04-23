/* 
#############################################################
	Authors:	John Walter
			Evan Schulte
	Project:	ECE 4220 Final Project: RTU System
#############################################################
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
unsigned long *ptr, *PBDR, *PBDDR, *PFDR, *PFDDR, *GPIOBIntType1, *GPIOBIntType2, *GPIOBEOI, *GPIOBIntEn, *IntStsB, *RawIntStsB, *Debounce;
SEM sem;
static RT_TASK LED_Task, Analog_Task;
RTIME period;

static void my_handler(unsigned irq_num, void *cookie){

	rt_disable_irq(59);
	struct timeval t;
	do_gettimeofday(&t);
	int sec = t.tv_sec;
	int usec = t.tv_usec;
	int i, bit, button;
	i=0;
	bit=0;
	button=0;
	for(i=0;i<3;i++){
		if((*RawIntStsB & 0x01) == 0x01){
			button = 1;
		}else if((*RawIntStsB & 0x02) == 0x02){
			button = 2;
		}else{
			button = 3;
		}
		printk("Button pushed: %d\n",button);
	}
	rtf_put(0,&button,sizeof(int));
	rtf_put(0,&sec,sizeof(int));
	rtf_put(0,&usec,sizeof(int));
	//printk("Button %d was pressed\n",button);
	if((button == 0)){
		*GPIOBEOI |= 0x01;
	}else if((button == 1)){
		*GPIOBEOI |= 0x02;
	}else{
		*GPIOBEOI |= 0x04;
	}
	rt_enable_irq(59);
}

static void LEDTask(int x){
	//******* It is not reading from FIFO 1 or writing to fifo 2
	printk("LEDTask started");
	int buf = 0;
	struct timeval t;
	int sec = 0;
	int usec = 0;

	while(1){
		if(rtf_get(1, &buf, sizeof(buf)) > 0){
			printk("FROM FIFO 1: %d\n",buf);
			if(buf == 0){
				*PBDR |= 0x80;
			}else if(buf == 1){
				*PBDR &= ~0x80;
			}else if(buf == 2){
				*PBDR |= 0x40;
			}else if(buf == 3){
				*PBDR &= ~0x40;
			}
			do_gettimeofday(&t);
			printk("sec: %d \tusec: %d\n", t.tv_sec, t.tv_usec);
			rtf_put(2, &t, sizeof(t));

		}
		rt_sleep(nano2count(50000000));
	}
}
/*
static void AnalogTask(int t){

}
*/
int init_module(void){
	printk("Module started\n");
	ptr = (unsigned long *)__ioremap(0x80840000,4096,0);
	PBDR = ptr+1;
	PBDDR = ptr+5;
	GPIOBIntType1	= (unsigned long *)__ioremap(0x808400AC, 4096,0);
	GPIOBIntType2 	= (unsigned long *)__ioremap(0x808400B0, 4096,0);
	GPIOBEOI 		= (unsigned long *)__ioremap(0x808400B4, 4096,0);
	GPIOBIntEn 		= (unsigned long *)__ioremap(0x808400B8, 4096,0);
	IntStsB 		= (unsigned long *)__ioremap(0x808400BC, 4096,0);
	RawIntStsB	 	= (unsigned long *)__ioremap(0x808400C0, 4096,0);
	Debounce		= (unsigned long *)__ioremap(0x808400C4, 4096,0);

	*PBDDR &= 0xF8;			//1111 1000
	*PBDDR &= 0x60; 		//set leds as output
	*GPIOBIntEn |= 0x07;	//0000 0111
	*GPIOBIntType1 |= 0x07;	//0000 0111
	*GPIOBIntType2 &= 0xF8; //1111 1000
	*Debounce |= 0x07;		//0000 0111

	struct timeval t;
	rt_set_periodic_mode();
	period = start_rt_timer(nano2count(1000000));
	rt_sem_init(&sem,1);
	rtf_create(0,sizeof(int));
	printk("Created fifo 0\n");
	rtf_create(1,sizeof(int));
	printk("Created fifo 1\n");
	rtf_create(2,sizeof(t));
	printk("Created fifo 2\n");

	rt_request_irq(59, my_handler, 0, 1);
	rt_enable_irq(59);


	// initialize a semaphore

	// initialize real time task and make it periodic
	rt_task_init(&LED_Task, LEDTask, 0, 256,0,0,0);
	//rt_task_init(&Analog_Task, AnalogTask, 0, 256, 0, 0, 0);
	//rt_task_resume(&Analog_Task);
	rt_task_resume(&LED_Task);
	return 0;
}

void cleanup_module(void){
	rt_task_delete(&LED_Task);
	rt_task_delete(&Analog_Task);

	stop_rt_timer();
	rtf_destroy(0);
	rtf_destroy(1);
	rtf_destroy(2);
	rt_disable_irq(59);
	rt_sem_delete(&sem);
}
