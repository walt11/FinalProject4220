/* 	Name       : 	server_tcp.c
	Author     : 	Luis A. Rivera
	Description: 	ECE 4220/7220 lab, Fall 2012
					Lab 5. Simple server (TCP) for lecture purposes			*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>

#include <fcntl.h>
#include <sys/mman.h>

#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT_NUMBER 2221
#define SEND_PORT_NUM 2223

pthread_t th1, th2, th3, th4;
int sockfd;
struct sockaddr_in serv_addr, cli_addr;
struct in_addr *addr_list;
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
int j;
char *iplist[10];
char *ip = "10.3.52.19";
void connection(int); 			// function prototype
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void Read_From_Socket(void *x){
	//puts("read from socket");

    int newsockfd, pid;
    socklen_t clilen;

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // Creates socket. Connection based.
    if (sockfd < 0)
   	 error("ERROR opening socket");

    // fill in fields
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;		 // symbol constant for Internet domain
    serv_addr.sin_addr.s_addr = INADDR_ANY; // IP address of the machine on which
											 // the server is running
    serv_addr.sin_port = htons(PORT_NUMBER);	 // port number

    // binds the socket to the address of the host and the port number
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
   	 error("ERROR on binding");

    listen(sockfd, 5);			// listen for connections
    clilen = sizeof(cli_addr);	// size of structure

	 // To allow the server to handle multiple simultaneous connections: infinite
    // loop and fork.
    while (1)
    {
   	 // blocks until a client connects to the server
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        j++;		// counter for the connections that are established.
        pid = fork();
        if (pid < 0)
            error("ERROR on fork");

        if (pid == 0)	// child process
        {
       	 //printf("Connection #%d created\n",j);
        	//addr_list = (struct in_addr *) &cli_addr.sin_addr.s_addr;
        	//strcpy(iplist[j-1],inet_ntoa(*addr_list));
        	//strcpy(iplist[j-1],"10.3.52.19");

        	//printf("%s\n", iplist[j-1]);
            close(sockfd);			// close socket
            connection(newsockfd);	// call function that handles communication

            exit(0);
        }
        else			// parent
        {
       	 close(newsockfd);
       	 signal(SIGCHLD,SIG_IGN);	// to avoid zombie problem
        }
    } 	// end of while
}

/********************************* DOSTUFF() **********************************
 There is a separate instance of this function for each connection.  It handles
 all communication once a connection has been established.
 *****************************************************************************/
void connection (int sock)
{
	//puts("Connection");
	char filename[10];
	sprintf(filename, "RTU_%d.txt",j);
	FILE *out = fopen(filename,"w");
	fclose(out);
	while(1){

			out = fopen(filename,"a");
			read(sock,&d,sizeof(d));	// recvfrom() could be used
			if(d.b1 != prev.b1){
				fprintf(out,"Button 1\t%d\t%d\t%d\n", d.b1, d.b1_time[0], d.b1_time[1]);
			}
			if(d.b2 != prev.b2){
				fprintf(out,"Button 2\t%d\t%d\t%d\n", d.b2, d.b2_time[0], d.b2_time[1]);
			}
			if(d.b3 != prev.b3){
				fprintf(out,"Button 3\t%d\t%d\t%d\n", d.b3, d.b3_time[0], d.b3_time[1]);
			}
			if(d.led1 != prev.led1){
				fprintf(out,"LED 1\t\t%d\t%d\t%d\n", d.led1, d.led1_time[0], d.led1_time[1]);
			}
			if(d.led2 != prev.led2){
				fprintf(out,"LED 2\t\t%d\t%d\t%d\n", d.led2, d.led2_time[0], d.led2_time[1]);
			}
			fclose(out);
			prev = d;

	}
}
int main(int argc, char *argv[])
{
	int choice=0;
	int i, rtu=0, c=0;
	pthread_create(&th1, NULL, (void*)Read_From_Socket, NULL);

	   int sock, n;
	   unsigned int length;
	   struct sockaddr_in server, from;
	   struct hostent *hp;

	   sock = socket(AF_INET, SOCK_DGRAM, 0); // Creates socket. Connectionless.
	   if (sock < 0)
		   error("socket");

	   server.sin_family = AF_INET;		// symbol constant for Internet domain

	   server.sin_port = htons(SEND_PORT_NUM);	// port field
	   length = sizeof(struct sockaddr_in);		// size of structure



	while(1){
		choice = 0;
		while((choice < 1) | (choice > 4)){
			printf("Select:\n\t(1) Send a Command\n\t(2) View Log\n\t(3) Retrieve Unsent Data\n\t(4) Quit\n> ");
			scanf("%d",&choice);
		}
		switch(choice){
		case 1:

			printf("Select an RTU: ");
			//for(i=0; i<j; i++){
			i = 0;
				printf("(%d) %s\n", i+1, ip);
			//}
			while((rtu<1) | (rtu>j)){
				printf("> ");
				scanf("%d",&rtu);
			}
			hp = gethostbyname(ip);		// converts hostname input (e.g. 10.3.52.15)
			bcopy((char *)hp->h_addr, (char *)&server.sin_addr, hp->h_length);
			printf("(1) Toggle LED 1\n(2) Toggle LED 2\n(3) Flip Switch 1\n(4) Flip Switch 2\n(5) Flip Switch 3\n> ");
			while((c<1) | (c>j)){
				scanf("%d",&c);
			}
//			switch(c){
//			case 1:
			c = c-1;
			n = sendto(sock, &c, sizeof(c), 0, (const struct sockaddr *)&server,length);
//				break;
//			case 2:
//				break;
//			case 3:
//				break;
//			case 4:
//				break;



			break;
		case 2:
			system("cat RTU_1.txt");
			break;
		case 3:

			break;
		case 4:
			close(sockfd);
			exit(0);
			break;
		}
	}
	return 0;
}

