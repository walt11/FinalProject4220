/*

Author:		John Walter
Project:	4220 Final Project

*/


#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include<rtai.h>
#include<rtai_lxrt.h>



int main(void){
	int fd, choice;
	unsigned long *PBDR, *PBDDR, *ptr, *PFDR, *PFDDR;

	// open the file /dev/mem
	fd = open("/dev/mem", O_RDWR|O_SYNC); // O_RDWR|O_SYNC

	// if the file could not be opened - error out
	if(fd == -1){
		puts("Error opening file");
		return -1;
	}

	// point to the memory location of the board
	ptr = (unsigned long *)mmap(NULL,4096/*getpagesize()*/,PROT_READ|PROT_WRITE,MAP_SHARED, fd, 0x80840000); // 4096
	// if the memory map could not be completed
	// Access B port
	PBDR = ptr + 1; 	// 0x80840004
	PBDDR = ptr + 5; 	// 0x80840014
	*PBDDR |= 0x60; // set leds as output
	int bit, i;
	while(1){
		printf("Choose LED\n\t(1) RED On\n\t(2) RED Off\n\t(3) YELLOW ON\n\t(4) YELLOW Off\n> ");
		scanf("%d",&choice);
		while((choice>4) | (choice<1)){
			printf("Invalid. Choose again: ");
			scanf("%d",choice);
		}
		switch(choice){
		case 1:
			*PBDR |= 0x20;
			break;

		case 2:
			*PBDR &= ~0x20;
			break;
		case 3:
			*PBDR |= 0x40;
			break;
		case 4:
			*PBDR &= ~0x40;
			break;
		}

		for(i=0;i<8;i++){
			bit = ((*PBDR >> i) & 0x01);
			printf("%d ",bit);
		}
		printf("\n");

	}



	return 0;
}
