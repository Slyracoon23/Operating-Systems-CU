#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>

#define DEVICE_NAME "/dev/simple_character_device"
#define BUFF_SIZE 16

char menu_select(void);

int main(){
	/* Variable Declarations: */
	char option;
	int length;
	int whence;
	int currPos;
	char buffer[BUFF_SIZE];
	int fd = open(DEVICE_NAME, O_RDWR); // open file with read write privileges
	bool running = true;

	while(running){
		memset(buffer, 0, BUFF_SIZE); // reset buffer
		char option = menu_select();

		switch(option){
			case 'r':
				/* ask user how many bytes */
				printf("Enter the number of bytes you want to read: "); 

				 /* user inputs length of how many bytes */
				scanf("%d", &length);

				/* reads from the file, puts it to the buffer for x-length */
				int checkRead = read(fd, buffer, length); 
				if(checkRead != 0){
					printf("Not enough buffer space to read\n");
				}
				else{
				/* prints the buffer */
				printf("Reading: %s\n", buffer); 
				}
				int c;
				while(c = getchar() != '\n'  && c != EOF);  // flush stdin
				break;

			case 'w':
				/* User writes to the file*/
				printf("Writing:"); 
				
				scanf("%s", buffer); // unsafe to go out of bounds
				buffer[BUFF_SIZE -1] = '\0'; // Null terminated just in case
				/* writes the buffer to file */
				int checkWrite = write(fd, buffer, strlen(buffer)); 
				if(checkWrite == 0){
					printf("Not enough buffer space to write\n");
				}
				while(getchar() != '\n');  // flush local buffer
				break;

			case 's':
				/* Print out the seek menu: */
				printf("SEEK OPTIONS:\n");
				printf("'0' seek set\n");
				printf("'1' seek cur\n");
				printf("'2' seek end\n");
				printf("Enter whence: ");

				/* User inputs an option */
				scanf("%d", &whence);

				/*User Inputs an offset value */
				printf("\nEnter an offset value: ");
				scanf("%d", &currPos);

				/*llseek operation to reposition read/write file */
				llseek(fd, currPos, whence);
				while(getchar() != '\n'); 
				break;

			case 'e':
				/* Exit the program */
				printf("Exiting\n");
				running = false;
				break;

			/* Handles invalid inputs*/
			default:
				printf("\nPick something else.\n");
				break;
		}
	}
	close(fd);
	return 0;
}

char menu_select(void){
	char option = 0;
	/* Print menu */
	printf("****Please Enter the Option******\n");
	printf("Option 'r' to read from device\n");
	printf("Option 'w' to write to device\n");
	printf("Option 's' to seek from device\n");
	printf("Option 'e' to exit from device\n");
	printf("Input: ");
	/* Retrieve the user input */
	scanf("%c", &option);
	return option;
}
