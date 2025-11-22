#define _XOPEN_SOURCE 700 

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>

// display directory for every new line
void newLine(){
    char cwd[1024], host[1024];
    char *user = getlogin();
    char *home;

    if (user == NULL){
        perror("Error fetching user");
        sleep(3);
        exit(1);
    }

    //fetching host name
    if(gethostname(host, sizeof(host)) != 0){
        perror("Error fetching host");
        sleep(3);
        exit(1);
    }

    //fetching home directory
    home = getenv("HOME");
    if(home == NULL){
        perror("Error fetching environemnt variable");
        sleep(3);
        exit(1);
    }

    //fetching pathname of current working directory
    if (getcwd(cwd, sizeof(cwd)) != NULL){
        if (!strncmp(home, cwd, strlen(home))){
            printf("%s@%s:~%s$ ", user, host, cwd+strlen(home));
        }else{
            printf("%s@%s:~%s$ ", user, host, cwd);
        }
        
    }else{
        perror("Error fetching current working directory");
        sleep(3);
        exit(1);
    }
    return;
}

//return to home directory
void hDir(){
    char *home;
    home = getenv("HOME");  //getting home directory

    if(home == NULL){
        perror("Error fetching environemnt variable");
        sleep(3);
        exit(1);
    }
    
    //changing directory to home
    if(chdir(home) == -1){
        perror("Error going to home directory");
        sleep(3);
        exit(1);
    }
    return;
}

//initalizing array to sort and store user inputted commands
char **setArgs(char *input){
    int size = strlen(input), count = 0;
    char** words = NULL;
    char *copy = strdup(input);         //duplicating user input for local manipulation
    char *token = strtok(copy, " \n");  //tokenizing user inputs

    //assigning tokens to array indexes
    while (token != NULL){
        words = realloc(words, (count+1) * sizeof(char*));
        words[count] = strdup(token);
        count++;
        token = strtok(NULL, " \n");
    }
    words = realloc(words, (count+1) * sizeof(char *));
    words[count] = NULL;

    free(copy);
    return words;
}

//freeing memory of args array
void freeArgs(char **args) {
    if (args) {
        for (int i = 0; args[i] != NULL; i++) {
            free(args[i]);
        }
        free(args);
    }
    return;
}

//trim a string
int trim(char *str) {
    char *end;

    // Trim leading whitespace
    while(isspace((unsigned char)*str)){
        str++;
    }
        
    // All spaces
    if(*str == 0){
        return 1;
    } 
        
    // Trim trailing whitespace
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)){
        end--;
    }

    // Write new null terminator character
    end[1] = '\0';
    return 0;
}

//check if we exit the terminal
int terminate(char *input){
    char *copy = strdup(input);
    char *token = strtok(copy, " ");
    if(!strcmp(token, "exit")){
        free(copy);
        return 1;
    }
    free(copy);
    return 0;
}

//implementation of cd command
void changeDir(char *buf){
    char *dir = buf + 3;
    if(trim(dir)){
        hDir();
        return;
    }else{
        chdir(dir);
    }
}

//signal handler
void handle(){
    return;
}

//main function
int main(void){
    char buf[1024];
    int status, bytes;
    pid_t pid;
    
    hDir();

    while(1){
        signal(SIGINT, handle);     //allow read() to be interrupted but does nothing
        newLine();         
        fflush(stdout);

        //reading user input
        if((bytes = (read(STDIN_FILENO, buf, sizeof(buf)))) == -1){
            if(errno == EINTR){                 //ctrl + c
                write(STDOUT_FILENO, "\n", 1);
                continue;
            }
            perror("Error reading from terminal");
            sleep(2);
            exit(1);
        }else if (bytes == 0){              //ctrl + d
            write(STDOUT_FILENO, "\n", 1);
            exit(0);
        }
        buf[strcspn(buf, "\n")] = '\0'; //removing newline from user input

        //checking if no command was given and clearing whitespace
        if(trim(buf)){
            continue;
        }
        
        //check termination status and terminate if required
        if(terminate(buf)){
            exit(0);
        }

        //run cd command
        if (!strncmp(buf, "cd", 2)&& (buf[2] == '\0' || isspace((unsigned char)buf[2]) || buf[2] == '/')){
            changeDir(buf);
            continue;
        }
        
        char **args = setArgs(buf);                 //assigning arguemnts from user input to array using setArgs()    
        pid = fork();                               //create child process
        if(pid == -1){
            perror("Error starting new process");
            exit(1);
        }else if(pid == 0){
            if(execvp(args[0], args) == -1){        //executing command in child process
                perror("Execution failed");
                exit(1);
            }
        }else{
            wait(&status);  //wait for child process to end
        }
        freeArgs(args);     //free memory held by args[]
    }
}
