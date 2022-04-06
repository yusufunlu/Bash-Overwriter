#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#define DEBUG false
#define debug_print(fmt, ...) do { if (DEBUG) fprintf(stdout, fmt, __VA_ARGS__); } while (0)
#define MAX_LINE 80 /* The maximum length command */
#define MAX_HISTORY 10
#define FAKE_WAIT_TIME 1
#define EXIT_KEYWORD "exit"
#define WAIT_KEYWORD "&"
#define HISTORY_KEYWORD "history"
#define HISTORY_SYMBOL "!!"

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

char rawInput[MAX_LINE];
void parsingArguments();
char *arguments[MAX_LINE];
void waitChild();
int executeCommand();
void fancyCommandWait(void);
void showHistory(int order);
void handleSigint(int sig);
void saveHistory();
pid_t endID;
pid_t childID;
int status;
time_t when;
int commandCount = 0;
char history[10][MAX_LINE];
bool toWait = false;
clock_t tStart;

int main(void) {
    int should_run = 1; /* flag to determine when to exit program */

    while (should_run) {
        // memset(rawInput, 0, sizeof rawInput);
        printf("YU>");
        fflush(stdout);
        toWait = false;
        memset( rawInput, '\n', MAX_LINE );

        fgets(rawInput, MAX_LINE, stdin);
        if(rawInput[0] == '\n') {
            printf("Command cannot be empty\n");
            continue;
        }
        tStart = clock();

        rawInput[strcspn(rawInput, "\n")] = 0;

        debug_print("%s\n", rawInput);

        saveHistory();
        parsingArguments();
        signal(SIGINT, handleSigint);

        if(strcmp(arguments[0], EXIT_KEYWORD) == 0) {
            printf("exit been pressed\n");
            break;
        }

        childID = fork();

        if (childID < 0) { /* error occurred */
            fprintf(stderr, "Fork Failed\n");
            exit(-1);
        } else if (childID == 0) { /* child process */
            if (strcmp(arguments[0], HISTORY_KEYWORD) == 0 || strcmp(arguments[0], HISTORY_SYMBOL) == 0) {
                showHistory(-1);
            }else {
                char* historyStart = strstr(arguments[0], "!");
                if (historyStart!= NULL) {
                    debug_print("Found %s \n",historyStart);
                    int historyOrder = arguments[0][1] -'0';
                    debug_print("historyOrder: %d\n",historyOrder);
                    showHistory(historyOrder);
                } else {
                    executeCommand();
                }
            }
            exit(0);
        } else { /* parent process */
            waitChild();
        }

        /**
        * After reading user input, the steps are:
        * (1) fork a child process using fork()
        * (2) the child process will invoke execvp()
        * (3) if command included &, parent will invoke wait()
        */
    }

    return 0;
}

int executeCommand() {
    debug_print("In child: child pid %d\n", getpid());
    //For demonstration to show how parent cursor will wait the child
    if (toWait) {
        sleep(2);
        //fancyCommandWait();
    }
    int execResult = execvp(arguments[0],arguments);
    debug_print("execResult: %d\n",execResult);
    //execvp() does not return for success situation or it returns an error.
    exit(-1);
}


void waitChild() {
    clock_t tUpdate;
    //For double check kill the child process after a while
    if (!toWait){
        sleep(FAKE_WAIT_TIME);
        int killedChildResult = kill(childID, SIGKILL);
        debug_print("It was enough time for you child: %d result: %d\n",childID,killedChildResult);
        return;
    }

    debug_print("In parent parent id: %d child id: %d\n",getpid(),childID);
    //wait() unsuccessful case return -1

    while (endID !=childID) {
        tUpdate = clock();
        //WNOHANG Do not block the main process so UI render can still work
        endID = waitpid(childID, &status, WNOHANG);
        printf("\r %s%ldms>%s", KGRN,((tUpdate - tStart) / 1000),KNRM);

        fflush(stdout);
        usleep(10000);

        debug_print("child exited endId: %d status: %d\n", endID, status);

        if (endID == -1) {            /* error calling waitpid       */
            perror("Wait error");
            //exit(EXIT_FAILURE);
        } else if (endID == 0) {        /* child still running         */
            time(&when);
        } else if (endID == childID) {  /* child ended                 */
            if (WIFEXITED(status))
                debug_print("Child ended normally%s\n", " ");
            else if (WIFSIGNALED(status))
                debug_print("Child ended because of an uncaught signal%s\n", "");
            else if (WIFSTOPPED(status))
                debug_print("Child process has stopped%s\n", " ");
        }
    }
    printf("\n");
}
void parsingArguments() {

    int i =0;
    arguments[i] = strtok(rawInput, " ");

    while( arguments[i] != NULL) {
        i++;
        arguments[i] = strtok(NULL, " ");
        if(arguments[i] != NULL && strcmp(arguments[i], WAIT_KEYWORD) == 0) {
            toWait = true;
            i--;
        }
    }
    arguments[i] = NULL;
}

void fancyCommandWait() {
    //fancy wait simulation
    time(&when);
    debug_print("Parent waiting for child at %s\n", ctime(&when));

    //(100+0)/2*100*1000
    for (int percent=0;percent<=100;percent++){
        printf("\r %%%d ",percent);
        fflush(stdout);
        usleep(FAKE_WAIT_TIME * 10000);
    }
    time(&when);
    debug_print("Parent waiting for child at %s\n", ctime(&when));

}

void handleSigint(int sig) {
    showHistory(-1);
    sleep(FAKE_WAIT_TIME);
    kill(childID, SIGKILL);
    debug_print("It was enough time for you child: %d\n",childID);
    return;
}
void showHistory(int order) {

    if(order < 0) {
        //printf("****HISTORY***** \n");
        if(commandCount> 0) {
            for (int index=commandCount;index>0; index--) {
                printf("%d. %s \n",index, history[commandCount-index]);
            }
        }
    } else {
        if(order>commandCount) {
            printf("%d. doesn't exist \n",order);
        } else {
            printf("%d. %s \n", order,history[commandCount-order]);
        }
    }
    return;

}

void saveHistory() {
    //history feature, move 0 to 1, 1 to 2... 8 to 9
    //0(zero) is reserved for last command
    //when commandCount is 1, move 0 to 1 and save current command to 0
    //when commandCount is 2, move 1 to 2 and 0 to 1 and save current command to 0
    //when commandCount is 3, move 2 to 3 and 1 to 2 and 0 to 1 and save current command to 0
    //when commandCount is 9, move 8 to 9, 7 to 8
    //when commandCount is 10,don't move 9 to 10, move 8 to 9

    if(strcmp(rawInput, HISTORY_KEYWORD) == 0) {
        return;
    }
    if(strcmp(rawInput, HISTORY_SYMBOL) == 0) {
        return;
    }
    if(strstr(rawInput, "!") != NULL ) {
        return;
    }

    if(commandCount> 0) {
        int index;
        if(commandCount==MAX_HISTORY) {
            index = commandCount-1;
        } else {
            index = commandCount;
        }
        for (;index>0; index--) {
            if(index!=MAX_HISTORY) {
                strcpy(history[index], history[index-1]);
            } else {
                debug_print("%s is removed\n",history[index-1]);
            }
        }
    }
    commandCount++;
    strcpy(history[0], rawInput);
}