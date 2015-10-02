/*
	ECE357 Operating Systems
	Dolen Le
	PS 3 Simple Shell
	Prof. Hakner
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

struct arg {
	char *argStr;
	struct arg *next;
};

enum redirMode {R_APPEND, R_TRUNCATE, R_READ};

FILE *input;

char *cmdBuf;
size_t bufSize = 256;
int readSize;
struct arg *argStart;
struct rusage *usage;
struct timeval *before, *after;
int errors = 0;

struct arg *newArg(char* arg, struct arg *prev) {
	struct arg *temp = malloc(sizeof(struct arg));
	if(!temp) {
		fprintf(stderr, "Could not allocate memory.\n");
		exit(-1);
	}
	temp->argStr = arg;
	temp->next = NULL;
	if(prev) {
		prev->next = temp;
	}
	return temp;
}

void doRedirect(char* redir, char* operation, int trunc) {
	int src;
	int dest;
	if(redir == operation) {
		if(*redir == '<') {
			src = 0; //stdin
		} else {
			src = 1; //stdout
		}
	} else if(redir == operation+1 && operation[0] == '2') {
		src = 2; //stderr
	} else {
		fprintf(stderr, "Error: Invalid redirection %s\n", operation);
		exit(-1);
	}
	if(trunc == R_TRUNCATE) {
		dest = open(operation+src, O_WRONLY|O_CREAT|O_TRUNC, 0666);
	} else if(trunc == R_APPEND) {
		dest = open(operation+src+1, O_WRONLY|O_CREAT|O_APPEND, 0666);
	} else {
		dest = open(operation+src+1, O_RDONLY);
	}
	if(dest == -1) {
		perror("Could not open output file");
		exit(-1);
	}
	if(dup2(dest,src) == -1) {
		perror("Could not duplicate file descriptor");
		exit(-1);
	}
	if(close(dest) == -1) {
		perror("Could not close destination file");
	}
}

int main(int argc, char *argv[]) {
	if(argc == 2) {
		if(!(input = fopen(argv[1], "r"))) {
			perror("Could not open script");
			exit(-1);
		}
	} else if(argc == 1) {
		input = stdin;
	} else {
		fprintf(stderr, "Usage: %s [script_path]\n", argv[0]);
		exit(-1);
	}

	if(!(cmdBuf = malloc(bufSize)) || !(usage = malloc(sizeof(struct rusage))) ||
	!(before = malloc(sizeof(struct timeval))) || !(after = malloc(sizeof(struct timeval)))) {
		fprintf(stderr, "Could not allocate buffer.\n");
	}

	while((readSize = getline(&cmdBuf, &bufSize, input)) != -1) { //get each line of input
		if(cmdBuf[0] == '#') { //comment
			continue;
		}
		cmdBuf[readSize-1] = 0; //delete newline
		struct arg *lastArg = argStart = NULL;
		int numArgs = 0;
		char* argTok;
		if(argTok = strtok(cmdBuf, " \t")) {
			argStart = lastArg = newArg(argTok, NULL);
			numArgs = 1;
			argTok = strtok(NULL, " \t");
		}
		while(argTok) { //create linked list for argument parsing
			lastArg = newArg(argTok, lastArg);
			numArgs++;
			argTok = strtok(NULL, " \t");
		}
		
		gettimeofday(before, NULL);
		int pid = fork();

		if(pid == 0) { //inside child
			char **cmdArgs = NULL;
			if(numArgs) { //build the argument array from the linked list
				cmdArgs = malloc(sizeof(char*)*numArgs+1);
				int i;
				for(i=0; i<numArgs; i++) {
					char* argument = argStart->argStr;
					char* redir = NULL;
					if((redir = strstr(argument, "<")) == argument) {
						doRedirect(redir, argument, R_READ);
					} else if(redir = strstr(argument, ">>")) {
						doRedirect(redir, argument, R_APPEND);
					} else if(redir = strstr(argument, ">")) {
						doRedirect(redir, argument, R_TRUNCATE);
					}
					if(redir) { //redirection operation, skip it
						numArgs--;
						i--;
					} else {
						cmdArgs[i] = argument; //add argument to array
					}
					argStart = argStart->next;
				}
				cmdArgs[i] = 0; //terminating null
			}
			printf("Running %s...\n", cmdBuf);
			if(execvp(cmdBuf, cmdArgs) == -1) {
				perror("Exec error");
				exit(-1);
			}
		} else if(pid > 0) { //parent waits for child
			int status;
			if(wait3(&status, 0, usage) == -1) {
				perror("Error waiting for child process");
				exit(-1);
			}
			gettimeofday(after, NULL);
			if(status) {
				errors++;
			}
			float realTime = (after->tv_sec + after->tv_usec/1000000.0) - (before->tv_sec + before->tv_usec/1000000.0);
			float sysTime = (usage->ru_stime.tv_sec + usage->ru_stime.tv_usec/1000000.0);
			float userTime = (usage->ru_utime.tv_sec + usage->ru_utime.tv_usec/1000000.0);
			printf("Exit status %d\n", status);
			printf("Real: %fs System: %fs User: %fs\n", realTime, sysTime, userTime);
			while(argStart) { //clean up linked list
				struct arg *temp = argStart;
				argStart = argStart->next;
				free(temp);
			}
		} else {
			perror("Could not fork process");
			exit(-1);
		}
	}
	
	printf("EOF Reached - Doneski.\n");
	return errors;
}