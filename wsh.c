#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

int batchMode;
char ***localVar;
int localVarCnt;
int maxVarCnt;
int historyCnt;
int maxHisCnt;
int hisCnt; // cur history count
struct history *head;

struct history {
	struct history *prev;
	struct history *next;
	char **cmds;
	int cmdCnt; // doesn't include NULL terminator
};

int search_name(char *name) {
	for(int i = 0; i < localVarCnt; i++) {
		if(strcmp(localVar[i][0], name) == 0) return i;
	}
	// not found
	return -1;
}

void print_localVar() {
	for(int i = 0; i < localVarCnt; i++) {
		if(strcmp(localVar[i][0], "") != 0) { // only printed active local vars
			printf("%s=%s\n", localVar[i][0], localVar[i][1]);
		}
	}
}

void update_local(char *toParse) {
	char *name = strsep(&toParse, "=");
        char *value = strsep(&toParse, "=");
        if(strcmp(value, "") == 0) { // delete some variable
        	int pos = search_name(name);
                if(pos != -1) { // empty name marks unused variables
                	strcpy(localVar[pos][0], "");
                        strcpy(localVar[pos][1], "");
                }
        } else {
        	if(maxVarCnt == localVarCnt + 5) {
                	maxVarCnt += 100;
                        localVar = realloc(localVar, maxVarCnt * sizeof(char**));
			if(localVar == NULL) exit(-1);
                }

                int pos = search_name(name);
                if(pos != -1) {
                	strcpy(localVar[pos][0], name);
                        strcpy(localVar[pos][1], value);
                } else { // new local variable
                        localVar[localVarCnt] = malloc(sizeof(char[2]));
                        localVar[localVarCnt][0] = malloc(30 * sizeof(char));
                        localVar[localVarCnt][1] = malloc(30 * sizeof(char));
			if(localVar[localVarCnt][0] == NULL || localVar[localVarCnt][1] == NULL) exit(-1);
                        strcpy(localVar[localVarCnt][0], name);
                        strcpy(localVar[localVarCnt][1], value);
                        localVarCnt++;
                }
        }
}

void resize_history(int cnt) {
	if(head == NULL && hisCnt == 0) {
		head = malloc(sizeof(struct history));
		if(head == NULL) exit(-1);
		head->next = NULL;
	} else {
		if(cnt < maxHisCnt && hisCnt > cnt) { // need to free some entries
			// probably should free here but would be complicated
			struct history *itr = head;
			for(int i = 0; i < cnt - 1; i++) {
				itr = itr->next;
			}
			itr->next = NULL;
			hisCnt = cnt;
			if(cnt == 0) head = NULL;
		}
	}
	maxHisCnt = cnt;
}

void insert_history(char **command, int num) {
	if(maxHisCnt == 0) return; // never insert on history set 0
	if(hisCnt == 0) { // no history yet
		struct history *newHistory = malloc(sizeof(struct history));
		if(newHistory == NULL) exit(-1);
		newHistory->cmds = malloc(256 * sizeof(char *));
		if(newHistory->cmds == NULL) exit(-1);
		for(int i = 0; i < num; i++) {
			newHistory->cmds[i] = malloc(20 * sizeof(char));
			if(newHistory->cmds[i] == NULL) exit(-1);
			strcpy(newHistory->cmds[i], command[i]);
		}
		newHistory->cmds[num] = NULL;
		newHistory->cmdCnt = num;

		head = newHistory;
		newHistory->next = NULL;
		hisCnt++;
	} else if(hisCnt < maxHisCnt) { // not full
		struct history *newHistory = malloc(sizeof(struct history));
		if(newHistory == NULL) exit(-1);
		newHistory->cmds = malloc(256 * sizeof(char *));
		if(newHistory->cmds == NULL) exit(-1);
                for(int i = 0; i < num; i++) {
                        newHistory->cmds[i] = malloc(20 * sizeof(char));
			if(newHistory->cmds[i] == NULL) exit(-1);
                        strcpy(newHistory->cmds[i], command[i]);
                }
		newHistory->cmds[num] = NULL;
		newHistory->cmdCnt = num;

		head->prev = newHistory;
		newHistory->next = head;
                head = newHistory;
                
                hisCnt++;
	} else { // full
		struct history *newHistory = malloc(sizeof(struct history));
        	if(newHistory == NULL) exit(-1);
		newHistory->cmds = malloc(256 * sizeof(char *));
		if(newHistory->cmds == NULL) exit(-1);
                for(int i = 0; i < num; i++) {
                        newHistory->cmds[i] = malloc(20 * sizeof(char));
			if(newHistory->cmds[i] == NULL) exit(-1);
			strcpy(newHistory->cmds[i], command[i]);
                }
		newHistory->cmds[num] = NULL;
		newHistory->cmdCnt = num;

		head->prev = newHistory;
                newHistory->next = head;
                head = newHistory;

		struct history *itr = head;
		while(itr->next != NULL) {
			itr = itr->next;
		}
		free(itr->cmds);
		free(itr);
	}
}

void print_history() {
	struct history *itr = head;
	for(int i = 1; i <= hisCnt; i++, itr = itr->next) {
		printf("%d) ", i);
		for(int j = 0; j < itr->cmdCnt; j++) {
			printf("%s", itr->cmds[j]);
			if(j != itr->cmdCnt - 1) printf(" ");
		}
		printf("\n");
	}
}

void run_pipe(char **input_parse, int i) {
	char ***arg = malloc(20 * sizeof(20 * sizeof(20 * sizeof(char))));
	if(arg == NULL) exit(-1);
        for(int j = 0; j < 20; j++) {
        	arg[j] = malloc(20 * sizeof(20 * sizeof(char)));
		if(arg[j] == NULL) exit(-1);
        }
        int j = 0, index = 0;
        for(int k = 0; k < i; k++) {
        	if(strcmp(input_parse[k], "|") == 0) {
                	arg[j][++index] = NULL;
                        j++;
                        index = 0;
                        continue;
                }
                arg[j][index++] = input_parse[k];
                if(k == i - 1) arg[j][++index] = NULL;
         }

         j++; // j denotes the number of cmds
         int fd[2];
         int oldfd[2];
         for(int k = 0; k < j; k++) {

         	if(k != j - 1) {
                	pipe(fd);
                }

                pid_t pid;
                pid = fork();
		if(pid == -1) {
			exit(-1);
		}

                if(pid == 0) {  // child process
                	if(k > 0) {
                        	dup2(oldfd[0], STDIN_FILENO);
                                close(oldfd[0]);
                                close(oldfd[1]);
                        }
                        if(k < j - 1) {
                                close(fd[0]);
                                dup2(fd[1], STDOUT_FILENO); // write to STDOUT
                                close(fd[1]);
                        }

                        if(execvp(arg[k][0], arg[k]) == -1) {
                        	printf("execvp: No such file or directory\n");
                                exit(-1);
                        }
                        exit(0);
               	} else { // parent process
                	if(k > 0) {
                            	close(oldfd[0]);
                               	close(oldfd[1]);
                     	}

                   	if(k < j - 1) {
                         	oldfd[0] = fd[0];
                             	oldfd[1] = fd[1];
                    	}
                	waitpid(pid, NULL, 0);
           	}
	}

     	if(j > 1) {
        	close(oldfd[0]);
               	close(oldfd[1]);
       	}

       	for(int j = 0; j < 20; j++)
          	free(arg[j]);
      	free(arg);
}

void history(int index) {
        struct history *itr = head;
        for(int i = 0; i < index - 1; i++) {
                itr = itr->next;
        }
        for(int i = 0; i < itr->cmdCnt; i++) {
                if(itr->cmds[i] != NULL && strcmp(itr->cmds[i], "|") == 0) {
                        run_pipe(itr->cmds, itr->cmdCnt);
                        return;
                }
        }

        pid_t pid = fork();
        int wstatus;
	if(pid == -1) {
		exit(-1);
	}

        if(pid == 0) { // child process
                if(execvp(itr->cmds[0], itr->cmds) == -1) {
                        printf("execvp: No such file or directory\n");
                        exit(-1);
                }
                exit(0);
        } else { // parent process
                wait(&wstatus);
        }
}

void init() {
	maxVarCnt = 10;
	localVarCnt = 0;
	maxHisCnt = 5;
	hisCnt = 0;
	head = NULL;

	localVar = malloc(maxVarCnt * sizeof(char**));
	if(localVar == NULL) exit(-1);
	resize_history(5);
}

void dump() {
	for(int i = 0; i < localVarCnt; i++) {
		free(localVar[i][0]);
		free(localVar[i][1]);
	}
	free(localVar);

	for(int i = 0; i < hisCnt; i++) {
		if(i == hisCnt - 1) break;
		struct history *itr = head;
		for(int j = i; j < hisCnt - 1; j++) itr = itr->next;
		free(itr->cmds);
		free(itr);
	}
}

int main(int argc, char* argv[]) {
	init();
	
	if(argc == 1) {
		batchMode = 0;
	} else if(argc == 2) {
		batchMode = 1;
	} else {
		exit(-1);
	}
	
	if(batchMode == 0) {
		size_t n = 256;
		char *input = malloc(n * sizeof(char));
		char **input_parse = malloc(n * sizeof(20 * sizeof(char)));
		printf("wsh> ");
		while(getline(&input, &n, stdin) >= 0) {
			int i = 0;
			int pipeCnt = 0; // count the number of pipes
			char *tmp = strsep(&input, "\n");
			do {
				input_parse[i] = strsep(&tmp, " ");
				if(input_parse[i] != NULL && input_parse[i][0] == '$') { // value sub
					char *envVal = getenv(&input_parse[i][1]);
                                        if(envVal != NULL) { // found environment variable
						strcpy(input_parse[i], envVal);
                                        } else { // try finding local variable
						int varIndex = search_name(&input_parse[i][1]);
						if(varIndex != -1) { // found local variable
							strcpy(input_parse[i], localVar[varIndex][1]);
						} else {
							i--; // variable don't exist, i decrement by one
						}
					}
				}
				if(input_parse[i] != NULL && strcmp(input_parse[i], "|") == 0) pipeCnt++;
			} while(input_parse[i++] != NULL);
			i--;

			if(strcmp(input_parse[0], "exit") == 0) {
				exit(0);	
			} else if(strcmp(input_parse[0], "cd") == 0) {
				if(input_parse[2] != NULL) exit(-1);
				if(chdir(input_parse[1]) == -1) exit(-1); //exit on failure to cd
			} else if(strcmp(input_parse[0], "export") == 0) {
				char *name = strsep(&input_parse[1], "=");
				char *value = strsep(&input_parse[1], "=");
				setenv(name, value, 1); // 1 denotes always overwrite
			} else if(strcmp(input_parse[0], "local") == 0) {
				update_local(input_parse[1]);
			} else if(strcmp(input_parse[0], "vars") == 0) {
				print_localVar();
			} else if(strcmp(input_parse[0], "history") == 0) {
				if(input_parse[1] == NULL) print_history();
				else if(i == 3 && strcmp(input_parse[1], "set") == 0) resize_history(atoi(input_parse[2]));
				else { // history number command
					history(atoi(input_parse[1]));
				}
			} else if(pipeCnt > 0) { // need for pipe
				run_pipe(input_parse, i);
				if(hisCnt != 0 && head->cmdCnt == i) { // check if its the most recent entry
                                        for(int j = 0; j < i; j++) {
                                                if(strcmp(head->cmds[j], input_parse[j]) != 0) { // different commands
                                                        insert_history(input_parse, i);
                                                        break;
                                                }
                                        }
                                } else {
                                        insert_history(input_parse, i);
                                }
			} else {
				pid_t pid = fork();
                        	int wstatus;
				if(pid == -1) exit(-1);

                        	if(pid == 0) { // child process
                                	if(execvp(input_parse[0], input_parse) == -1) {
						printf("execvp: No such file or directory\n");
						exit(-1);
					}
                                	exit(0);
                        	} else { // parent process
                                	wait(&wstatus);
                        	}

				if(hisCnt != 0 && head->cmdCnt == i) { // check if its the most recent entry
					for(int j = 0; j < i; j++) {
						if(strcmp(head->cmds[j], input_parse[j]) != 0) { // different commands
							insert_history(input_parse, i);
							break;
						}
					}
				} else {
					insert_history(input_parse, i);
				}
			}
			printf("wsh> ");
		}
			
	} else {
		FILE *fp = fopen(argv[1], "r");
		if(fp == NULL) exit(-1);
		size_t n = 256;
                char *input = malloc(n * sizeof(char));
                char **input_parse = malloc(n * sizeof(20 * sizeof(char)));
                while(getline(&input, &n, fp) >= 0) {
                        int i = 0;
                        int pipeCnt = 0; // count the number of pipes
                        char *tmp = strsep(&input, "\n");
                        do {
                                input_parse[i] = strsep(&tmp, " ");
                                if(input_parse[i] != NULL && input_parse[i][0] == '$') { // value sub
                                        char *envVal = getenv(&input_parse[i][1]);
                                        if(envVal != NULL) { // found environment variable
                                                strcpy(input_parse[i], envVal);
                                        } else { // try finding local variable
                                                int varIndex = search_name(&input_parse[i][1]);
                                                if(varIndex != -1) { // found local variable
                                                        strcpy(input_parse[i], localVar[varIndex][1]);
                                                } else {
                                                        i--; // variable don't exist, i decrement by one
                                                }
                                        }
                                }
                                if(input_parse[i] != NULL && strcmp(input_parse[i], "|") == 0) pipeCnt++;
                        } while(input_parse[i++] != NULL);
                        i--;

                        if(strcmp(input_parse[0], "exit") == 0) {
                                exit(0);
                        } else if(strcmp(input_parse[0], "cd") == 0) {
                                if(input_parse[2] != NULL) exit(-1);
                                if(chdir(input_parse[1]) == -1) exit(-1); //exit on failure to cd
                        } else if(strcmp(input_parse[0], "export") == 0) {
                                char *name = strsep(&input_parse[1], "=");
                                char *value = strsep(&input_parse[1], "=");
                                setenv(name, value, 1); // 1 denotes always overwrite
                        } else if(strcmp(input_parse[0], "local") == 0) {
                                update_local(input_parse[1]);
                        } else if(strcmp(input_parse[0], "vars") == 0) {
                                print_localVar();
                        } else if(strcmp(input_parse[0], "history") == 0) {
                                if(input_parse[1] == NULL) print_history();
                                else if(i == 3 && strcmp(input_parse[1], "set") == 0) resize_history(atoi(input_parse[2]));
                                else { // history number command
                                        history(atoi(input_parse[1]));
                                }
                        } else if(pipeCnt > 0) { // need for pipe
                                run_pipe(input_parse, i);
                                if(hisCnt != 0 && head->cmdCnt == i) { // check if its the most recent entry
                                        for(int j = 0; j < i; j++) {
                                                if(strcmp(head->cmds[j], input_parse[j]) != 0) { // different commands
                                                        insert_history(input_parse, i);
                                                        break;
                                                }
                                        }
                                } else {
                                        insert_history(input_parse, i);
                                }
                        } else {
                                pid_t pid = fork();
                                int wstatus;
				if(pid == -1) exit(-1);

                                if(pid == 0) { // child process
                                        if(execvp(input_parse[0], input_parse) == -1) {
                                                printf("execvp: No such file or directory\n");
                                                exit(-1);
                                        }
                                        exit(0);
                                } else { // parent process
                                        wait(&wstatus);
                                }

                                if(hisCnt != 0 && head->cmdCnt == i) { // check if its the most recent entry
                                        for(int j = 0; j < i; j++) {
                                                if(strcmp(head->cmds[j], input_parse[j]) != 0) { // different commands
                                                        insert_history(input_parse, i);
                                                        break;
                                                }
                                        }
                                } else {
                                        insert_history(input_parse, i);
                                }
                        }
		}
	}

	dump();
	
	return 0;
}
