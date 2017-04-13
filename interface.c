/*

Author:		John Walter
Project:	4220 Final Project

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define COMMAND_SIZE 20
#define PORT_NUMBER 2222

int main(void){
		int choice = 0;
		char command[COMMAND_SIZE];
		while(1){
		memset(&command[0], 0, sizeof(command));
		printf("Select:\n\t(1) View Log\n\t(2) Send Command\n\t(3) Exit\n> ");
		scanf("%d",&choice);
		while(choice <1 | choice > 3){
			printf("Invalid choice: > ");
			scanf("%d",&choice);
		}
		switch(choice){
			case 1:

				break;
			case 2:
				printf("Enter Command: ");
				scanf("%s",command);
				break;
			case 3:
				exit(0);

		}
	}
}