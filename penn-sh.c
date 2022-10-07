#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "tokenizer.h"
#include <fcntl.h>
#include <ctype.h>

#define INPUT_SIZE 1024

pid_t childPid = 0;

void executeShell(int timeout);

void writeToStdout(char *text);

void alarmHandler(int sig);

void sigintHandler(int sig);

char *getCommandFromInput();

void registerSignalHandlers();

void killChildProcess();

void redirectionsSTDOUTtoFile(char* tokenAfter);

void redirectionsSTDINtoFile(char* tokenAfter) ;



int main(int argc, char **argv) {
    registerSignalHandlers();

    int timeout = 0;
    if (argc == 2) {
        timeout = atoi(argv[1]);
    }

    if (timeout < 0) {
        writeToStdout("Invalid input detected. Ignoring timeout value.\n");
        timeout = 0;
    }

    while (1) {
        executeShell(timeout);
    }

    return 0;
}

/* Sends SIGKILL signal to a child process.
 * Error checks for kill system call failure and exits program if
 * there is an error */
void killChildProcess() {
    if (kill(childPid, SIGKILL) == -1) {
        perror("Error in kill");
        exit(EXIT_FAILURE);
    }
}

/* Signal handler for SIGALRM. Catches SIGALRM signal and
 * kills the child process if it exists and is still executing.
 * It then prints out penn-shredder's catchphrase to standard output */
void alarmHandler(int sig) {
}

/* Signal handler for SIGINT. Catches SIGINT signal (e.g. Ctrl + C) and
 * kills the child process if it exists and is executing. Does not
 * do anything to the parent process and its execution */
void sigintHandler(int sig) {
    if (childPid != 0) {
        killChildProcess();
    }
}


/* Registers SIGALRM and SIGINT handlers with corresponding functions.
 * Error checks for signal system call failure and exits program if
 * there is an error */
void registerSignalHandlers() {
    if (signal(SIGINT, sigintHandler) == SIG_ERR) {
        perror("Error in signal");
        exit(EXIT_FAILURE);
    }
}

/* Prints the shell prompt and waits for input from user.
 * Takes timeout as an argument and starts an alarm of that timeout period
 * if there is a valid command. It then creates a child process which
 * executes the command with its arguments.
 *
 * The parent process waits for the child. On unsuccessful completion,
 * it exits the shell. */
void executeShell(int timeout) {

    char minishell[] = "penn-sh# ";
    writeToStdout(minishell);
    char *arg_array = (char*) malloc (1024 * sizeof(char));
    char *command;
    command = getCommandFromInput();

    int status;

    if (command[0] != '\0') {
        childPid = fork();

        if (childPid < 0) {
            perror("Error in creating child process");
            free(command);
            exit(EXIT_FAILURE);
        }

        if (childPid == 0) {
            int j = 0;
            while (command[j] != '\0') {
                
                if ((command[j] != '>') && (command[j] != '<')) {
                    arg_array[j] = command[j] ; 
                }
                else {
                    break ; 
                }
                j++;
            }
            arg_array[j+1] = '\0';


            int i = 0;
            while (command[i] != '\0') {
                int cntOut = 0 ; 
                int cntIn = 0 ; 
                if (command[i] == '>') {
                    int ind = i+1 ; 

                    if (cntOut == 0) {
                        redirectionsSTDOUTtoFile(&command[ind]) ; 
                    }
                    else {
                        perror("Invalid: Multiple standard output redirects") ; 
                    }
                    cntOut++;
                    
                }
                else if (command[i] == '<') {
                    int in = i+1 ; 

                    if (cntIn == 0) {
                        redirectionsSTDINtoFile(&command[in]) ;
                    }
                    else {
                        perror("Multiple standard input redirects or redirect in invalid location") ; 
                    }
                    cntIn++;
                }
                i++ ; 
            }
            if (execvp(&arg_array[0], arg_array) == -1) {
                perror("Error in execvp");
                free(command);
                exit(EXIT_FAILURE);
            }
            else {
                free(command);
            }
            free(arg_array);
            
        }
        else {
            do {
                if (wait(&status) == -1) {
                    perror("Error in child process termination");
                    exit(EXIT_FAILURE);
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            childPid = 0;
            free(command);
        }
    }
    
}

/* Writes particular text to standard output */
void writeToStdout(char *text) {
    if (write(STDOUT_FILENO, text, strlen(text)) == -1) {
        perror("Error in write");
        exit(EXIT_FAILURE);
    }
}

/* Reads input from standard input till it reaches a new line character.
 * Checks if EOF (Ctrl + D) is being read and exits penn-shredder if that is the case
 * Otherwise, it checks for a valid input and adds the characters to an input buffer.
 *
 * From this input buffer, the first 1023 characters (if more than 1023) or the whole
 * buffer are assigned to command and returned. An \0 is appended to the command so
 * that it is null terminated */
char *getCommandFromInput() {
    
    char string[256] = "";
    string[255] = '\0';	 
    int br ; 
    char *tok;
    char *command = (char*) malloc (1024 * sizeof(char));
    
    TOKENIZER *tokenizer;
    br = read( STDIN_FILENO, string, 255 );
    if (br == 0) {
        exit(0) ; 
    }

    else if (br > 0) {
        string[br-1] = '\0';   /* remove trailing \n */
        
        tokenizer = init_tokenizer( string );
        
        int i = 0 ; 
        while( (tok = get_next_token( tokenizer )) != NULL ) {
            command[i] = *tok ;        
            free(tok);
            i++;
        }
        command[i] = '\0';
        free_tokenizer( tokenizer ); /* free memory */
        
    }
    return command; 

}



void redirectionsSTDOUTtoFile(char* tokenAfter) {

    int new_stdout;
    new_stdout = open(tokenAfter, O_WRONLY | O_TRUNC | O_CREAT, 0644); 
    dup2(new_stdout,STDOUT_FILENO);

}


void redirectionsSTDINtoFile(char* tokenAfter) {

    int new_stdin ; 
    new_stdin = open(tokenAfter, O_RDONLY, 0644); 

    if (new_stdin < 0) {
        perror("Invalid standard input redirect: No such file or directory");  
    }

    dup2(new_stdin, STDIN_FILENO); 
    
}





