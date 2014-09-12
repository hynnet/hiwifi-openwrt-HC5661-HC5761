/* packaging program for making a file like u-boot loadable by stage1 loader */
/* J J Larkworthy */
/* 10 Oct 2006 */
/* build this program using:                 */
/* gcc packager.c crc32.c -o packager              */
/*          */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <crc32.h>

int main(int argc, char **argv)
{
	char *file_in_name = argv[1];
	char *file_out_name = argv[2];
	struct stat file_stat;
	unsigned int file_size;
	unsigned long file_crc;
	int status;
	char *buffer;
	
	int file_in;
	int file_out;
	
/* ensure we have valid parameters to start */
	if (argc != 3) {
	printf("Need command file_in file_out only\n");
	return 1;
	}
	printf("Input file - %s\n", file_in_name);
	printf("Output file - %s\n", file_out_name);


	file_in = open(file_in_name, O_RDONLY);
	status = fstat(file_in, &file_stat);
	file_size = file_stat.st_size;

	printf("Input File Size - %d\n", file_size);

	buffer=malloc(file_size);
	if(file_size != read(file_in, buffer, file_size)) {
		printf("failed to read all data in\n");
		return 1;
	}
	file_crc= crc32(0,(char *) buffer, file_size);

	file_out=creat(file_out_name, S_IRWXU+S_IRGRP+S_IROTH);
	write(file_out, &file_size, 4);
	write(file_out, &file_crc, 4);
	if (file_size != write(file_out, buffer, file_size)){
		printf("Failed to output all data\n");
	}
	close(file_in);
	close(file_out);
	
}
