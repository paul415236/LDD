#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<error.h>
#include<fcntl.h>
#include<sys/types.h>
#include <string.h>

#define SCULL_DEV	"/dev/scull0"

int main()
{
	int fd,len;
	char inbuf[20];
	char outbuf[20] ="scull dev test!";
	// write
	fd = open(SCULL_DEV, O_WRONLY);
	if(fd < 0)
	{
		printf("Error openning the device of sculldev for wrITing!\n");
		exit(1);
	}

	printf ("write len = %d \n", strlen(outbuf));
	len = write(fd,outbuf,strlen(outbuf));
	if(len < 0)
	{
		printf("Error writing to the device!\n");
		close(fd);
		exit(1); 
	}

	printf("writing %d bytes to the device!\n",len);
	close(fd);

	// read
	fd = open(SCULL_DEV, O_RDONLY);
	if(fd < 0)
	{ 
		printf("Error openning the device of sculldev for reading!\n");
		exit(1);
	}
	len = read(fd,inbuf,len);
	if(len < 0)
	{
		printf("Error reading from the device!\n ");
		close(fd);
		exit(1);
	}
	printf("reading %d bytes from the device!\n",len);
	if(len)
		printf("%s\n",inbuf);
}
