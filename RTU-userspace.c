/* 
###################################################
	Authors:	John Walter
				Evan Schulte
	Project:	ECE 4220 Final Project: RTU System
###################################################
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <rtai.h>
#include <rtai_lxrt.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define SERVER_ADDRESS "10.3.52.254"
#define PORT_NUMBER 2221
#define PORT_REC 2223

RTIME BaseP;

char buffer[100];
int one;
int two;
pthread_t th1, th2, th3, th4, th5;
int sensor=0;
sem_t sem1;
struct Data {
	// status
	int b1;
	int b2;
	int b3;
	int led1;
	int led2;

	// times
	int b1_time[2];
	int b2_time[2];
	int b3_time[2];
	int led1_time[2];
	int led2_time[2];

};
struct Data d, prev, com;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void Send_to_socket(void *x){
	RT_TASK *rttask1 = rt_task_init(nam2num("th1"),0,512,256);
	BaseP = start_rt_timer(nano2count(1000000000));
	rt_task_make_periodic(rttask1,rt_get_time(),BaseP);

	int sockfd;
	int portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = PORT_NUMBER;
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // Creates socket. Connection based.
    if (sockfd < 0)
        puts("Error opening socket");

    server = gethostbyname(SERVER_ADDRESS);

    // fill in fields of serv_addr
    bzero((char *) &serv_addr, sizeof(serv_addr));	// sets all values to zero
    serv_addr.sin_family = AF_INET;		// symbol constant for Internet domain

    // copy to serv_addr.sin_addr.s_addr. Function memcpy could be used instead.
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port = htons(portno);		// fill sin_port field
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
	        puts("ERROR connecting to server. Writing to log instead");
	        // NEED TO HANDLE THIS
	}else{
		puts("connection established");
	}

    while(1){
    	printf("Thing\t\tStatus\tSeconds\t\tuSeconds\n");
    	printf("-----\t\t------\t-------\t\t--------\n");
    	printf("Button 1\t%d\t%d\t%d\n", d.b1, d.b1_time[0], d.b1_time[1]);
    	printf("Button 2\t%d\t%d\t%d\n", d.b2, d.b2_time[0], d.b2_time[1]);
    	printf("Button 3\t%d\t%d\t%d\n", d.b3, d.b3_time[0], d.b3_time[1]);
    	printf("LED 1\t\t%d\t%d\t%d\n", d.led1, d.led1_time[0], d.led1_time[1]);
    	printf("LED 2\t\t%d\t%d\t%d\n\n\n", d.led2, d.led2_time[0], d.led2_time[1]);

    	// send message
    	n = write(sockfd,&d,sizeof(d));	// sendto() could be used.
    	if (n < 0){
    		puts("Could not write to socket");
    		// THIS IS PROBABLY WHERE WE NEED TO STORE THE UNSENT DATA
    	}

    	rt_task_wait_period();

    }
}



void Button_FIFO(void *x){
	int button=0;
	int sec=0;
	int usec=0;
	int fd_fifo_in = open("/dev/rtf/0", O_RDWR);
	while(1){
		read(fd_fifo_in, &button, sizeof(int));
		read(fd_fifo_in, &sec, sizeof(int));
		read(fd_fifo_in, &usec, sizeof(int));

		sem_wait(&sem1);
		switch(button){
			case 1:
					if(d.b1 == 1){
						d.b1 = 0;
					}else{
						d.b1 = 1;
					}
					d.b1_time[0] = sec;
					d.b1_time[1] = usec;
				break;
			case 2:
					if(d.b2 == 1){
						d.b2 = 0;
					}else{
						d.b2 = 1;
					}
					d.b2_time[0] = sec;
					d.b2_time[1] = usec;
				break;
			case 3:
					if(d.b3 == 1){
						d.b3 = 0;
					}else{
						d.b3 = 1;
					}
					d.b3_time[0] = sec;
					d.b3_time[1] = usec;
				break;
		}
		sem_post(&sem1);
	}
}
void TIME_FIFO(void *x){
	struct timeval t;
	int fd_fifo_in = open("/dev/rtf/2", O_RDWR);
	while(1){
		if(read(fd_fifo_in, &t, sizeof(t)) == -1){
			puts("Read from fifo 2 error");
		}
		if(sensor == 0){
			// put time retrieved into time for LED 1
			d.led1_time[0] = t.tv_sec;
			d.led1_time[1] = t.tv_usec;
		}else if(sensor == 1){
			// put time retrieved into time for LED 2
			d.led2_time[0] = t.tv_sec;
			d.led2_time[1] = t.tv_usec;
		}else if(sensor == 2){
			// put time retrieved into time for LED 2
			d.b1_time[0] = t.tv_sec;
			d.led2_time[1] = t.tv_usec;
		}else if(sensor == 3){
			// put time retrieved into time for LED 2
			d.b2_time[0] = t.tv_sec;
			d.led2_time[1] = t.tv_usec;
		}else if(sensor == 4){
			// put time retrieved into time for LED 2
			d.b3_time[0] = t.tv_sec;
			d.led2_time[1] = t.tv_usec;
		}
	}
}

void LED_Test(void *x){
	unsigned long *PBDR, *PBDDR, *ptr;
	int fd_fifo_in = open("/dev/rtf/1", O_RDWR);
	int fd;
		// open the file /dev/mem
		fd = open("/dev/mem", O_RDWR|O_SYNC); // O_RDWR|O_SYNC

		// if the file could not be opened - error out


		// point to the memory location of the board
		ptr = (unsigned long *)mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED, fd, 0x80840000); // 4096
		// if the memory map could not be completed

		// Access B port
		PBDR = ptr + 1; 	// 0x80840004
		PBDDR = ptr + 5; 	// 0x80840014
		*PBDDR &= 0xFFFFFFF7;
		*PBDDR &= 0xFFFFFFEF;
		int bit, bit2;
		int write_v=0;
		int value1 = 0;
		int value2 = 0;
		// NOT writing to FIFO???
		while(1){
			bit = ((*PBDR >> 3) & 0x01);
			if(bit == 0){
				LED = 1;
				printf("Button press detected\n");
				if(value1 == 1){ // led1 is on
					value1 = 0;
					d.led1 = 0;
					write_v = 2;
				}else{
					d.led1 = 1;
					value1 = 1;
					write_v = 1;
				}
				write(fd_fifo_in,&write_v,sizeof(int));
			}

			bit2 = ((*PBDR >> 4) & 0x01);
			if(bit2 == 0){
				LED = 2;
				printf("Button press detected\n");
				if(value2 == 1){ // led1 is on
					value2 = 0;
					d.led2 = 0;
					write_v = 4;
				}else{
					d.led2 = 1;
					value2 = 1;
					write_v = 3;
				}
				write(fd_fifo_in,&write_v,sizeof(int));
			}

			usleep(100000);
		}

}

void Rec_Command(void *x){
	int fd_fifo_in = open("/dev/rtf/1", O_RDWR);
	int sock, length, n;
	int command=0;
	   int boolval = 1;		// for a socket option
	   socklen_t fromlen;
	   struct sockaddr_in server;
	   struct sockaddr_in from;


	   sock = socket(AF_INET, SOCK_DGRAM, 0); // Creates socket. Connectionless.
	   if (sock < 0)
		   error("Opening socket");

	   length = sizeof(server);			// length of structure
	   bzero(&server,length);			// sets all values to zero. memset() could be used
	   server.sin_family = AF_INET;		// symbol constant for Internet domain
	   server.sin_addr.s_addr = INADDR_ANY;		// IP address of the machine on which
												// the server is running
	   server.sin_port = htons(PORT_REC);	// port number

	   // binds the socket to the address of the host and the port number
	   if (bind(sock, (struct sockaddr *)&server, length) < 0)
	       error("binding");

	   // set broadcast option
	   if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &boolval, sizeof(boolval)) < 0)
	   	{
	   		printf("error setting socket options\n");
	   		exit(-1);
	   	}

	   fromlen = sizeof(struct sockaddr_in);	// size of structure

	   while (1){
		   // receive from client
	       n = recvfrom(sock, &command, sizeof(int), 0, (struct sockaddr *)&from, &fromlen);
	       if (n < 0)
	    	   error("recvfrom");
	       printf("Command: %d\n",command);
	       sensor = command;
	       sem_wait(&sem1);
	       if(command == 0){
	       		if(d.led1 == 1){
	       			d.led1 = 0;
	       		}else{
	       			d.led1 = 1;
	       		}
	       }else if(command == 1){
	       		if(d.led2 == 1){
	       			d.led2 = 0;
	       		}
	       }else if(command == 2){
	       		if(d.b1 == 1){
	       			d.b1 = 0;
	       		}else{
	       			d.b1 = 1;
	       		}
	       }else if(command == 3){
	       		if(d.b2 == 1){
	       			d.b2 = 0;
	       		}else{
	       			d.b2 = 1;
	       		}
	       }else if(command == 4){
	       		if(d.b3 == 1){
	       			d.b3 = 0;
	       		}else{
	       			d.b3 = 1;
	       		}
	       }
	       sem_post(&sem1);
	       write(fd_fifo_in,&command,sizeof(int));
	   }
}


int main(void){
	BaseP = start_rt_timer(nano2count(1000000));
	pthread_create(&th1, NULL, (void*)Send_to_socket, NULL);
	pthread_create(&th2, NULL, (void*)Button_FIFO, NULL);
	pthread_create(&th3, NULL, (void*)TIME_FIFO, NULL);
	pthread_create(&th4, NULL, (void*)LED_Test, NULL);
	pthread_create(&th5, NULL, (void*)Rec_Command, NULL);

	pthread_join(th1,0);
	pthread_join(th2,0);
	pthread_join(th3,0);
	pthread_join(th4,0);
	pthread_join(th5,0);

	sem_init(&sem1,0,1);

	//int choice=0;
	//int fd_fifo_in = open("/dev/rtf/1", O_RDWR);
	while(1){
		/*
		while((choice<1) | (choice > 4)){
			printf("Pick:\n\t(1) LED 1 ON\n\t(2) LED 1 OFF\n\t(3) LED 2 ON\n\t(4) LED 2 OFF\n> ");
			scanf("%d",&choice);
		}
		write(fd_fifo_in,&choice,sizeof(int));
		switch(choice){
			case 1:
				d.led1 = 1;
				LED = 1;
				break;
			case 2:
				d.led1 = 0;
				LED = 1;
				break;
			case 3:
				d.led2 = 1;
				LED = 2;
				break;
			case 4:
				d.led2 = 0;
				LED = 2;
				break;
		}

		// will have to write to FIFO 1 for LEDs
		// 1 = turn on led 1	global LED = 1
		// 2 = turn off led 1	global LED = 1
		// 3 = turn on led 2	global LED = 2
		// 4 = turn off led 2	global LED = 2
	*/
	}

	return 0;
}
