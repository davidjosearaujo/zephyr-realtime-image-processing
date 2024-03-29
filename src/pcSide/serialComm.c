/* *******************************************************************
 * SOTR 22-23
 * Paulo Pedreiras, Nov. 2022
 * nRF boards register as /dev/ttyACMx (usually ACM0)
 * Default config is 115200,8,n,1
 * Can try it with any nRF example that outputs text to the terminal
 * and/or with the console example
 *
 * Adapted form:
 *  LINUX SERIAL PORTS USING C/C++
 *  Linux Serial Ports Using C/C++
 *  Article by:Geoffrey Hunter
 *  Date Published:	June 24, 2017
 *  Last Modified:	October 3, 2022
 *  https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
 *
 * Suggested reading:
 *  The TTY demystified, by Linus Åkesson
 *  Linus Åkesson homepage/blog, at (2022/11):
 *  https://www.linusakesson.net/programming/tty/index.php
 *
 ******************************************************************** */

// C library headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Linux headers
#include <fcntl.h>	 // Contains file controls like O_RDWR
#include <errno.h>	 // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>	 // write(), read(), close()

#include "imageGenerator.h"

int main()
{
	// Open the serial port. Change device path as needed (currently set to an standard FTDI USB-UART cable type device)
	int serial_port = open("/dev/ttyACM0", O_RDWR);

	// Create new termios struct, we call it 'tty' for convention
	struct termios tty;

	// Read in existing settings, and handle any error
	if (tcgetattr(serial_port, &tty) != 0)
	{
		printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
		return 1;
	}

	tty.c_cflag &= ~PARENB;		   // Clear parity bit, disabling parity (most common)
	tty.c_cflag &= ~CSTOPB;		   // Clear stop field, only one stop bit used in communication (most common)
	tty.c_cflag &= ~CSIZE;		   // Clear all bits that set the data size
	tty.c_cflag |= CS8;			   // 8 bits per byte (most common)
	tty.c_cflag &= ~CRTSCTS;	   // Disable RTS/CTS hardware flow control (most common)
	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO;														 // Disable echo
	tty.c_lflag &= ~ECHOE;														 // Disable erasure
	tty.c_lflag &= ~ECHONL;														 // Disable new-line echo
	tty.c_lflag &= ~ISIG;														 // Disable interpretation of INTR, QUIT and SUSP
	tty.c_iflag &= ~(IXON | IXOFF | IXANY);										 // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Disable any special handling of received bytes

	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	// tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
	// tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

	tty.c_cc[VTIME] = 10; // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
	tty.c_cc[VMIN] = 0;

	// Set in/out baud rate to be 115200
	cfsetispeed(&tty, B115200);
	cfsetospeed(&tty, B115200);

	// Save tty settings, also checking for error
	if (tcsetattr(serial_port, TCSANOW, &tty) != 0)
	{
		printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
		return 1;
	}

	//To create a new set of images, uncomment follwoing lines
	createImageFolder();
	// return 0;

	// Allocate memory for read buffer, set size according to your needs
	char read_buf[256];

	// Normally you wouldn't do this memset() call, but since we will just receive
	// ASCII data for this example, we'll set everything to 0 so we can
	// call printf() easily.
	memset(&read_buf, '\0', sizeof(read_buf));

	/*
	 * Loop forever and print output
	 * Stop with CTRL-C
	 */

	int fileNum = 1;
	while (fileNum <= DIRSIZE)
	{
		// Read bytes. The behaviour of read() (e.g. does it block?,
		// how long does it block for?) depends on the configuration
		// settings above, specifically VMIN and VTIME
		//
		// int num_bytes = read(serial_port, &read_buf, sizeof(read_buf));
		// n is the number of bytes read. n may be 0 if no bytes were received, and can also be -1 to signal an error.
		// if (num_bytes < 0) {
		//     printf("Error reading: %s", strerror(errno));
		//     return 1;
		// }
		//
		// Here we assume we received ASCII data, but you might be sending raw bytes (in that case, don't try and
		// print it to the screen like this!)
		// printf("Read %i bytes. Received message: %s\n", num_bytes, read_buf);
		// break;

		char fileName[20];
        sprintf(fileName, "images/img%d.raw", fileNum);
		FILE *file = fopen(fileName, "r");
		if (file == NULL) {
			perror("Error opening file");
			return 1;
		}

		unsigned char msg[IMGWIDTH * IMGWIDTH];
		char line[256];
		int index = 0;
		while (fgets(line, sizeof(line), file)) {
			// Tokenize the line
			char *token = strtok(line, " ");
			while (token != NULL) {
				// Convert the token to an unsigned char
				unsigned char hex_value;
				sscanf(token, "%hhx", &hex_value);

				// Convert the unsigned char to an ASCII character
				char ascii_char = (char)hex_value;
				if (hex_value == 0) {
					ascii_char = '0';
				}

				// Store the ASCII character in the array
				msg[index] = ascii_char;

				token = strtok(NULL, " ");
				index++;
			}
		}
		fclose(file);

		printf("Image: %d\n", fileNum);
		for (int i = 0; i < IMGWIDTH * IMGWIDTH; i++) {
			printf("%hhx ", msg[i]);
			if ((i + 1) % IMGWIDTH == 0) {
				printf("\n");
			}
		}
		printf("\n");
		write(serial_port, msg, sizeof(msg));

		unsigned char end[] = {'\n'};
		write(serial_port, end, sizeof(end));

		fileNum += 1;
		sleep(1);
	}

	close(serial_port);
	return 0; // success
};