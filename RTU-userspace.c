/*

Author:		John Walter
Project:	4220 Final Project

*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <fcntl.h>
#include <semaphore.h>

sem_t sem;

void read_fifo(){
	int i,j;
	int fifo_buffer[6][2];
	int fd_fifo_in;
	fd_fifo_in = open("/dev/rtf/0",O_RDWR);
	while(1){
		read(fd_fifo_in, &fifo_buffer, sizeof(fifo_buffer));
		for(i=0;i<6;i++){
			for(j=0;j<2;j++){
				printf("%d ",fifo_buffer[i][j]);
			}
			printf("\n");
		}
		printf("\n\n");
	}
}

int main(void)
{
	pthread_t readFifo;
	//created thread to test reading from the fifo, kernel module must be installed on board first
	pthread_create(&readFifo, NULL, (void*)&read_fifo,NULL);
	while(1){}
}
