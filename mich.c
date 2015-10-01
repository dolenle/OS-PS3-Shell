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

struct arg {
	char *argStr;
	struct arg *next;
};

enum redirMode {R_APPEND, R_TRUNCATE};

FILE *input;

char *cmdBuf;
size_t bufSize = 256;
int readSize;
struct arg *argStart;

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

void outputRedirect(char* redir, char* operation, int trunc) {
	int src;
	int dest;
	if(redir == operation) {
		src = 1; //stdout
	} else if(redir == operation+1 && operation[0] == '2') {
		src = 2; //stderr
	} else {
		fprintf(stderr, "Error: Invalid redirection %s\n", operation);
		exit(-1);
	}
	if(trunc) {
		dest = open(operation+src, O_WRONLY|O_CREAT|O_TRUNC, 0666);
	} else {
		dest = open(operation+src+1, O_WRONLY|O_CREAT|O_APPEND, 0666);
	}
	printf("dest=%d\n", dest);
	if(dest == -1) {
		perror("Could not open output file");
		exit(-1);
	}
	if(dup2(dest,src) == -1) {
		perror("Could not duplicate file descriptor");
		exit(-1);
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

	if(!(cmdBuf = malloc(bufSize+1))) {
		fprintf(stderr, "Could not allocate buffer.\n");
	}

	while((readSize = getline(&cmdBuf, &bufSize, input)) != -1) {
		if(cmdBuf[0] == '#') { //comment
			continue;
		}
		cmdBuf[readSize-1] = 0;
		struct arg *lastArg = argStart = NULL;
		int numArgs = 0;
		char* argTok = strtok(cmdBuf, " \t");
		while(argTok) { //create linked list for argument parsing
			if(!argStart) {
				argStart = lastArg = newArg(argTok, NULL);
			} else {
				lastArg = newArg(argTok, lastArg);
			}
			numArgs++;
			argTok = strtok(NULL, " \t");
		}

		int pid = fork();

		if(pid == 0) { //inside child
			char **cmdArgs = NULL;
			if(numArgs) {
				cmdArgs = malloc(sizeof(char*)*numArgs+1);
				int i;
				for(i=0; i<numArgs; i++) {
					char* argument = argStart->argStr;
					char* redir = NULL;
					if((redir = strstr(argument, "<")) == argument) {
						int src = open(argument+1, O_RDONLY);
						if(!src) {
							perror("Could not open input file");
							exit(-1);
						}
						if(dup2(src,0) == -1) { //stdin
							perror("Could not duplicate file descriptor");
							exit(-1);
						}
					} else if(redir = strstr(argument, ">")) {
						outputRedirect(redir, argument, R_TRUNCATE);
					} else if(redir = strstr(argument, ">>")) {
						outputRedirect(redir, argument, R_APPEND);
					}
					if(redir) { //redirection operation, skip it
						numArgs--;
						i--;
					} else {
						cmdArgs[i] = argument; //add argument to array
					}
					argStart = argStart->next;
				}
				cmdArgs[i] = 0;
			}
			if(execvp(cmdBuf, cmdArgs) == -1) {
				perror("error");
				exit(1);
			}
		}
	}
	

	return 0;
}