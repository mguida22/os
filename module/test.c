#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]){
	char cmd;
	char data[1024];

	while(1){
		printf("Enter a command. Options:\n");
		printf("  Press r to read from device\n");
		printf("  Press w to write to the device\n");
		printf("  Press e to exit from the device\n");
		printf("  Press anything else to keep reading or writing from the device");
		printf("Enter a command:");

		scanf(" %c", &cmd);

		switch(cmd){
			case 'r':
				printf("Data read from device:\n\n");
				system("cat /dev/simple_character_device");
				printf("\n");
				break;
			case 'w':
				printf("Enter data to write to device:\n\n");
				scanf(" %[^\n]", data);
				char line[1024];
				sprintf(line, "echo \"%s\" > /dev/simple_character_device", data);
				system(line);
				break;
			case 'e':
				printf("Exiting.\n");
				exit(0);
				break;
			default:
				break;
		}
	}
}
