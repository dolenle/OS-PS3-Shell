/*
	ECE357 Operating Systems
	Dolen Le
	PS 3 Simple Shell
	Prof. Hakner
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct arg {
	char *argStr;
	struct arg *next;
};

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
	//temp->argStr = strdup(arg); //remember to free this!
	temp->next = NULL;
	if(prev) {
		prev->next = temp;
	}
	return temp;
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
		//printf("%s",cmdBuf);
		cmdBuf[readSize-1] = 0;
		struct arg *lastArg = argStart = NULL;
		char* argTok = strtok(cmdBuf, " \t");
		int numArgs = 0, argLen = 0;
		while(argTok) {
			if(!argStart) {
				argStart = lastArg = newArg(argTok, NULL);
			} else {
				lastArg = newArg(argTok, lastArg);
			}
			numArgs++;
			if(strlen(argTok) > argLen) {
				argLen = strlen(argTok);
			}
			argTok = strtok(NULL, " \t");
		}
		argLen++;
		char **cmdArgs = NULL;
		if(numArgs) {
			cmdArgs = malloc(sizeof(char*)*numArgs+1);
			int i;
			for(i=0; i<numArgs; i++) {
				cmdArgs[i] = argStart->argStr;
				//cmdArgs[i] = malloc(sizeof(char)*argLen);
				//strncpy(cmdArgs[i], argStart->argStr, argLen);
				argStart = argStart->next;
				//printf("ARG%d=%s", i, cmdArgs[i]);
			}
			cmdArgs[numArgs] = 0;
		}

		if(execvp(cmdBuf, cmdArgs) == -1) {
			perror("error");
			exit(1);
		}
	}
	

	return 0;
}