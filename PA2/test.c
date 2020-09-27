#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>

#define DEVICE_NAME "/dev/simple_character_device"
#define BUFF_SIZE 16

char menu_select(void);

int main(){
	/* Variable Declarations: */
	char input;
	int length;
	int whence;
	int currPos;
	char buffer[BUFF_SIZE];
	int file = open(DEVICE_NAME, O_RDWR);
	bool running = true;

	while(running){
		char input = menu_select();

		switch(input){
			case 'r':
				/* ask user how many bytes */
				printf("How many bytes to read: "); 

				 /* user inputs length of how many bytes */
				scanf("%d", &length);

				/* reads from the file, puts it to the buffer for x-length */
				int checkRead = read(file, buffer, length); 
				if(checkRead != 0){
					printf("Not enough buffer space to read\n");
				}
				else{
				/* prints the buffer */
				printf("Reading: %s\n", buffer); 
				}	
				while(getchar() != '\n'); 
				break;

			case 'w':
				/* User writes to the file*/
				printf("Writing:"); 
				
				scanf("%s", buffer);

				/* writes the buffer to file */
				int checkWrite = write(file, buffer, strlen(buffer));
				if(checkWrite == 0){
					printf("Not enough buffer space to write\n");
				}
				while(getchar() != '\n'); 
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

				/*llseek operation */
				llseek(file, currPos, whence);
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
	close(file);
	return 0;
}

char menu_select(void){
	char input = 0;
	/* Print menu */
	printf("Menu:\n");
	printf("'r' to read from device\n");
	printf("'w' to write to device\n");
	printf("'s' to seek from device\n");
	printf("'e' to exit from device\n");
	printf("Input: ");
	/* Retrieve the user input */
	scanf("%c", &input);
	return input;
}
