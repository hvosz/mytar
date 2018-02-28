#include <dirent.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>
#include <time.h>

#define MAXPATH 256
#define BLOCK 512
#define ASCII2NUM(x) ((x) - 48)

/*execute type*/
#define ERROR -1
#define CREATE 1
#define EXTRACT 2
#define DISPLAY 3

struct THeader 
{
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char typeflag;
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
};

 
int isError(int argc, char *argv[])
{
	if (argc < 3){
		fprintf(stderr, 
			"usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
		return 1;
	}
	return 0;
}

/**************************Helper******************************/
void addBlock(int fd)
{
	char buffer[BLOCK];
	int i;
	for (i = 0; i < BLOCK; i++) 
		buffer[i] = '\0';

	write(fd, buffer, BLOCK);
}

bool isEmptyBlock(char buffer[BLOCK])
{
	int i;
	bool res = true;
	for (i = 0; i < BLOCK; i++) {
		if (buffer[i] !='\0') {
			res = false;
			i = BLOCK;
		} 
	}
	return res;
}

uint64_t octToDec(char *oct) {
	int i;
	int len = strlen(oct);
	uint64_t res = 0;
	uint64_t octMultiplier = 1;
	for (i = len - 1; i >= 0; i--) {
		res += ASCII2NUM(oct[i]) * octMultiplier;
		octMultiplier *= 8;
	}
	return res;
}

/*get path of file or directory*/
char *getPath(int argc, char *argv[], char *name, char path[])
{
	int i;
	int pathLen;
	int nameLen;

	/*if no destination was given, don't add path to name*/
	if(argc <= 3) {
		strcpy(path, name);
		return name;
	}

	/*else add path to beginning of name*/
	pathLen = strlen(argv[3]);
	nameLen = strlen(name);

	for (i = 0; i < pathLen; i++) {
		path[i] = argv[3][i];
	}

	/*make sure the path ends in '/'*/
	if (path[i - 1] != '/')
		path[i] = '/';

	for (i = 0; i < nameLen; i++) { 
		path[i + pathLen] = name[i];
	}
	path[i + pathLen + 1] = '\0';

	return path;
}
/**************************end Helper************************/

/****************************Header**************************/

/*testing function*/
void displayHeader(struct THeader *header)
{
	int i;
	printf("name:		");
	for (i = 0; i < 100; i++)
		printf("%c", header->name[i]);
	printf("\n");
	printf("mode:		%s\n", header->mode);
	printf("uid:		%s\n", header->uid);
	printf("gid:		%s\n", header->gid);
	printf("size:		%s\n", header->size);
	printf("mtime:		%s\n", header->mtime);
	printf("chksum:		%s\n", header->chksum);
	printf("typeflag:	%c\n", header->typeflag);
	printf("linkname: 	%s\n", header->linkname);
	printf("magic: 		%s\n", header->magic);
	printf("version: 	%s\n", header->version);
	printf("uname: 		%s\n", header->uname);
	printf("gname: 		%s\n", header->gname);
	/*printf("devmajor: 	%s\n", header->devmajor);
	printf("devminor: 	%s\n", header->devminor);*/
	printf("prefix: 	%s\n", header->prefix);
}

/*Takes curent block and structures it into a header*/
struct THeader *getHeader(int fd, int curBlock) {
	struct THeader *header = malloc(sizeof(struct THeader));
	char buffer[BLOCK];
	int i;

	lseek(fd, BLOCK * curBlock, SEEK_SET);

	read(fd, &buffer, 100);
	for (i = 0; i < 100; i++) {
		header->name[i] = buffer[i];
	}

	read(fd, &buffer, 8);
	for (i = 0; i < 8; i++) {
		header->mode[i] = buffer[i];
	}

	read(fd, &buffer, 8);
	for (i = 0; i < 8; i++) {
		header->uid[i] = buffer[i];
	}

	read(fd, &buffer, 8);
	for (i = 0; i < 8; i++) {
		header->gid[i] = buffer[i];
	}

	read(fd, &buffer, 12);
	for (i = 0; i < 12; i++) {
		header->size[i] = buffer[i];
	}

	read(fd, &buffer, 12);
	for (i = 0; i < 12; i++) {
		header->mtime[i] = buffer[i];
	}

	read(fd, &buffer, 8);
	for (i = 0; i < 8; i++) {
		header->chksum[i] = buffer[i];
	}

	read(fd, &buffer, 1);
	header->typeflag = buffer[0];

	read(fd, &buffer, 100);
	for (i = 0; i < 100; i++) {
		header->linkname[i] = buffer[i];
	}

	read(fd, &buffer, 6);
	for (i = 0; i < 6; i++) {
		header->magic[i] = buffer[i];
	}

	read(fd, &buffer, 2);
	for (i = 0; i < 2; i++) {
		header->version[i] = buffer[i];
	}

	read(fd, &buffer, 32);
	for (i = 0; i < 32; i++) {
		header->uname[i] = buffer[i];
	}

	read(fd, &buffer, 32);
	for (i = 0; i < 32; i++) {
		header->gname[i] = buffer[i];
	}

	read(fd, &buffer, 8);
	for (i = 0; i < 8; i++) {
		header->devmajor[i] = buffer[i];
	}
	/*fprintf(stdout, "test: devminor is %d\n", (int) octToDec(buffer));*/

	read(fd, &buffer, 8);
	for (i = 0; i < 8; i++) {
		header->devminor[i] = buffer[i];
	}
	/*fprintf(stdout, "test: devmajor is %s\n", header->devmajor);*/

	read(fd, &buffer, 155);
	for (i = 0; i < 155; i++) {
		header->prefix[i] = buffer[i];
	}

	return header;

}
/*******************end Header***********************/

/*****************execute and display******************/

int extractFile(int fd, int curBlock, 
	struct THeader *header, char path[])
{
	int outfd;
	char buffer[BLOCK];
	int loop;
	int remainder;
	int i;
	int fSize;

	/*initialize buffer string*/
	for (i = 0; i < BLOCK; i++) {
		buffer[i] = ' ';
	}

	/*create new file, exit if failure*/
	outfd = open(path, O_WRONLY | O_CREAT, (int)octToDec(header->mode));
	if (outfd == -1)
		exit(EXIT_FAILURE);

	/*copy file into outfd (keeping track of blocks traversed)*/
	fSize = (int) octToDec(header->size);
	loop = (int) fSize/ BLOCK;
	remainder = (int) fSize % BLOCK;

	for (i = 0; i < loop; i++) {
		read(fd, &buffer, BLOCK);
		write(outfd, &buffer, BLOCK);
		curBlock += 1;
	}

	/*write the non null characters in last block*/
	read(fd, &buffer, remainder);  
	write(outfd, &buffer, remainder);
	curBlock += 1;

	/*free space*/
	close(outfd);

	return curBlock;
}

int skipFile(int fd, int curBlock, struct THeader *header, char path[])
{
	/*MASOOD. This
	is literally a copy of the extract file function
	so my code doesn't break. 
	Change it how you will, but it behaves exactly like extract
	But you can use this function here to display
	the function. NOTE that you should only have one print statement 
	here since this is called in a while loop that goes to
	 each header in the
	program so long as you RETURN WHAT YOUR CURENT BLOCK IS*/
	int loop;
	int fSize;

	/*copy file into outfd (keeping track of blocks traversed)*/
	fSize = (int) octToDec(header->size);
	loop = (int) fSize/ BLOCK;
	curBlock += loop + 1;
	lseek(fd, BLOCK * curBlock, SEEK_SET);

	return curBlock;
}

void displayFile(char *name, struct THeader *header)
{
	struct stat sb;
	int i;
	int uLen = strlen(header->uname);
	int gLen = strlen(header->gname);
	char owner[50];
	time_t mtime = (int)octToDec(header->mtime);
	char mtimeStr[20];
	struct tm *timeInfo;

	/*format owner name*/
	for (i = 0; i < uLen; i++)
		owner[i] = header->uname[i];
	owner[i] = '/';
	for (i = 0; i < gLen; i++)
		owner[i + uLen + 1] = header->gname[i];
	owner[i + uLen + 1] = '\0';

	/*formating mtime*/
	timeInfo = localtime(&mtime);
	strftime(mtimeStr, sizeof(mtimeStr), "%G-%m-%d %H:%M", timeInfo);

	stat(name, &sb);


	/*print permissions*/
    printf((S_ISDIR((int)octToDec(header->mode))) ? "d" : "-");
    printf(((int)octToDec(header->mode) & S_IRUSR) ? "r" : "-") ;
    printf(((int)octToDec(header->mode) & S_IWUSR) ? "w" : "-");
    printf(((int)octToDec(header->mode) & S_IXUSR) ? "x" : "-");
    printf(((int)octToDec(header->mode) & S_IRGRP) ? "r" : "-");
    printf(((int)octToDec(header->mode) & S_IWGRP) ? "w" : "-");
    printf(((int)octToDec(header->mode) & S_IXGRP) ? "x" : "-");
    printf(((int)octToDec(header->mode) & S_IROTH) ? "r" : "-");
    printf(((int)octToDec(header->mode) & S_IWOTH) ? "w" : "-");
    printf(((int)octToDec(header->mode) & S_IXOTH) ? "x" : "-");

    /*printf("uid: %s, gid: %s, uname: %s, gname: %s\n",
    	header->uid, header->gid, header->uname, header->gname);*/
    
    printf(" %17s %8d", owner, (int)octToDec(header->size));
    printf(" %16s %.100s\n", 
    	mtimeStr, name);


}

/*Function used for extract and display.
Loops through the headers and files of the tar
and either extracts or displays them*/
void travTar (int argc, char *argv[], 
	bool verbosity, bool strict, bool isDis)
{
	int fd = open(argv[2], O_RDONLY);
	struct THeader *header;
	char buffer[BLOCK];
	int i;
	char path[103];
	int curBlock = 0;
	char type;
	bool done = false;
	/*char badChars[] = "!@%^*~|";
	bool validName = true;*/

	/*init string buffers*/
	for (i = 0; i < 103; i++) {
		path[i] = '\0';
	}
	for (i = 0; i < BLOCK; i++) {
		buffer[i] = ' ';
	}

	/*traverse tar tree*/
	while(!done) {
		header = getHeader(fd, curBlock);
		curBlock += 1;
		/*ensure we are past header*/
		lseek(fd, BLOCK * curBlock, SEEK_SET);
		/*figure out type*/
		type = header->typeflag;
		getPath(argc, argv, header->name, path);
		/*check if valid filename*/
		/*for(i = 0; i < strlen(badChars); i++) {
			if(strchr(path, badChars[i]) != NULL) {
				validName = false;
			}
		}
		if (!validName) {
			fprintf(stderr, "Bad Tar file\n");
			exit(EXIT_FAILURE);
		}*/
		/*display files if "xv" or "t"*/
		if((verbosity && !isDis) || (!verbosity && isDis)) {
			fprintf(stdout, "%.100s\n", path);
		}
		/*display files if "tv"*/
		if (verbosity && isDis)
			displayFile(path, header);
		/*handle each type different*/
		/*MASOOD!!!!!! You have a header for a directory or a file*/
		if (isDis) {
			switch(type) {
				case '0':  /*If it is a file*/
					curBlock = skipFile(fd, curBlock, 
						header, path);
					break;
				case '5': /*If it is a directory*/
					mkdir(path,
					 (int)octToDec(header->mode));
				case '\0': /*if it is a sym link*/
				default:  /*if it is anything else*/
					break;
			}
		}
		/*END MASOOD*/
		if(!isDis) {
			switch(type) {
				case '0':
					curBlock = extractFile(fd, curBlock,
					 header, path);
					break;
				case '5':
					/*do nothing*/
				default:
					break;
			}
		}
		/*check if next block is empty and therefore end of tarfile*/
		lseek(fd, BLOCK * curBlock, SEEK_SET);
		read(fd, &buffer, BLOCK);
		if(isEmptyBlock(buffer))
			done = true;
		lseek(fd, BLOCK * curBlock, SEEK_SET);

		/*free header*/
		free(header);
	}
}

/*********************end execute and display********************/

/**********************create***************************/

int putFile(int tarfd, int curBlock, struct stat *sb, char *name)
{
	int fd;
	char buffer[BLOCK];
	int loop;
	int remainder;
	int i;
	int fSize;

	/*initialize buffer string*/
	for (i = 0; i < BLOCK; i++) {
		buffer[i] = ' ';
	}

	/*write blank header*/
	lseek(tarfd, BLOCK * curBlock, SEEK_SET);
	curBlock += 1;

	/*open file TODO: figure out how to handle a -1 error*/
	fd = open(name, O_RDONLY);
	if (fd == -1)
		exit(EXIT_FAILURE); /*TODO*/

	/*copy file into tarfd (keeping track of blocks traversed)*/
	fSize = sb->st_size;
	loop = (int) fSize/ BLOCK;
	remainder = (int) fSize % BLOCK;

	for (i = 0; i < loop; i++) {
		read(fd, &buffer, BLOCK);
		write(tarfd, &buffer, BLOCK);
		curBlock += 1;
	}

	/*write the non null characters in last block*/
	read(fd, &buffer, remainder);  
	write(tarfd, &buffer, remainder);
	curBlock += 1;

	/*free space*/
	close(fd);

	return curBlock;
}

int putDir(int tarfd, int curBlock, char *name)
{
	/*char buffer[BLOCK];*/
	DIR *dir;
	struct dirent *dp;
	struct stat sb;
	char path[103];

	/*initialize buffer string*/
	/* for (i = 0; i < BLOCK; i++) {
		buffer[i] = ' ';
	}*/

	/*open file TODO: figure out how to handle a -1 error*/
	if (!(dir = opendir(name))) {
		return curBlock; /*TODO*/
	}

	/*insert header for directory*/
	lseek(tarfd, BLOCK * curBlock, SEEK_SET);
	curBlock += 1;

	while((dp = readdir(dir)) != NULL) {
		/*ignore current and parent directories*/
		if (strcmp(dp->d_name, ".") == 0 || 
			strcmp(dp->d_name, "..") == 0)
			continue;

		/*get path and stat*/
		snprintf(path, sizeof(path), "%s/%s", name, dp->d_name);
		if(stat(path, &sb) == -1)
			continue;

		/*make blank header*/
		lseek(tarfd, BLOCK * curBlock, SEEK_SET);
		curBlock += 1;

		/*if dir, recurse*/
		if (S_ISDIR(sb.st_mode)) {
			putDir(tarfd, curBlock, path);
		}
		else if (S_ISREG(sb.st_mode)) {
			curBlock = putFile(tarfd, curBlock, &sb, path);
		}
	}
	closedir(dir);
	return curBlock;
}

/*FOR MASOOD*/
/*struct THeader *makeHeader(char *name)
{*/
	/*open stat of name*/
	/*get a stat and get information*/
	/*check if it is a directory or symlink or file*/
	/*gets all the information from file*/
	/*make a DIR and a dirent and a stat*/

	/*return header;
}*/


/*NOTE: this is an incomplete create. It currently will just write the
files to the tar and make blank space for the header. TODO, make a 
createHeader function that takes in a file and constructs a buffer
to write to the file. TODO, rename current getHeader function so it
is more descernable between getHeader and createHeader because it
is currently very confusing*/

/*TODO: make sure to include verbosity and strict. They currently 
aren't in affect right now*/
void makeTar(int argc, char *argv[], bool verbosity, bool strict)
{

	int tarfd = open(argv[2], 
		O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	/*char buffer[BLOCK];*/
	int i;
	struct stat sb;
	int curBlock = 0;
	
	/*loop through each given file*/
	for (i = 3; i < argc; i++) {

		/*get status of given file/directory*/
		if(stat(argv[i], &sb) == -1)
			break;

		/*if file, print file to tar*/
		if (S_ISREG(sb.st_mode)){
			curBlock = putFile(tarfd, curBlock, &sb, argv[i]);
		}

		/*if directory, print write directory to tar file*/
		else if (S_ISDIR(sb.st_mode)) {
			curBlock = putDir(tarfd, curBlock, argv[i]);
		}


	}

	/*Add two empty blocks for end of file*/
	/*NOTE:: I'm not sure this is how a file is terminated*/
	/*Pleas double check, it is currently late and I will
	address this tomorrow*/
	lseek(tarfd, BLOCK * curBlock, SEEK_SET);
	addBlock(tarfd);
	curBlock += 1;
	lseek(tarfd, BLOCK * curBlock, SEEK_SET);
	addBlock(tarfd);

	/*free header*/
	/*free(header);*/

}

/******************************end create**************************/

int main(int argc, char *argv[])
{
	int options;
	int execute = 0;
	bool verbosity = false;
	bool strict = false;
	int i;
	int len;
	/*determine if input is valid...ish*/
	if (isError(argc, argv)){
		fprintf(stdout, "Not enough input\n");
		return 0;
	}

	/*get command line arguments while still checking for errors*/
	len = strlen (argv[1]);
	for (i = 0; i < len; i++) {
		options = argv[1][i];
		switch(options) {
			case 'f':
				break;
			case 'c':
				execute = (execute == 0) ? CREATE : ERROR;
				break;
			case 't':
				execute = (execute == 0) ? DISPLAY : ERROR;
				break;
			case 'x':
				execute = (execute == 0) ? EXTRACT : ERROR;
				break;
			case 'v':
				verbosity = true;
				break;
			case 'S':
				strict = true;
				break;	
			default:
				execute = ERROR;
		}
		if (execute == ERROR) {
			printf("a\n");
			fprintf(stderr, 
			"usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
			exit(EXIT_FAILURE);
		}
	}

	/*make sure that an execute was given*/
	if (execute == 0) {
		printf("b\n");
		fprintf(stderr,
		 "usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
		exit(EXIT_FAILURE);
	}

	/*execute code*/
	switch(execute) {
		case CREATE:
			makeTar(argc, argv, verbosity, strict);
			break;
		case DISPLAY:
			travTar(argc, argv, verbosity, strict, true);
			break;
		case EXTRACT:
			travTar(argc, argv, verbosity, strict, false);
			break;
	}


	return 0;
}