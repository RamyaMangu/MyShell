//Ramya Sai Swathi Mangu: 50021174, UCINETID: rmangu, rmangu@uci.edu
//Jessica Bhalerao: 76535936, UCINETID: jbhalera, jbhalera@uci.edu

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>

typedef enum{FOREGROUND, BACKGROUND, SUSPENDED, STOPPED, NONE} STATE;
typedef struct JOBS{
    int jobid;
    pid_t pid;
    STATE status;
    char *cmndline;
}JOBS;
struct JOBS PIDs[1024];
pid_t pid;
int numjobs =-1;
int main_pid;
int input_redirect = 0;
int output_redirect = 0;
int output_redirect_append = 0;
int con_length =0;
int MaxLine =80;
int MaxArgc =80;
int MaxJob =5;

void delete_job(pid_t pid)
{
    //Deleting the job based on the passed in pid
    for(int i = 0; i<=numjobs; i++) {
        if(PIDs[i].pid== pid){
            pid = PIDs[i].pid;
            if(PIDs[i].status = STOPPED)
            {
                kill(pid, SIGCONT);
            }
            kill(pid, SIGINT);
            int temp;
            waitpid(pid, &temp, 0);
            PIDs[i].status = NONE;
            PIDs[i].pid = 0;
            PIDs[i].jobid =0;
            return;
        }
    }
}
void sigchld_handler(int sig)
{
    pid_t child_pid;
    int status;
    //Detect any teminated or stopped jobs, but don't wait on others.
    //printf("Sig child handler\n");
    while((child_pid = waitpid(-1, &status, WNOHANG | WUNTRACED))>0)
    {
        //JOb stopped the reciept of a signal
        if(WIFSTOPPED(status))
        {
            //to do
        }
        //Job teminated (uncaught signal or normally)
        else if (WIFSIGNALED(status) || WIFEXITED(status))
        {
            delete_job(child_pid);
        }
        else
        {
            printf("Waitpid error\n");
        }
    }
    return;
}
void sigtstp_handler(int sig)
{
    //printf("PID: %d\n", pid);
    for(int i = 0; i<=numjobs; i++) {
        if(PIDs[i].pid== pid){
            PIDs[i].status = STOPPED;
        }
    }
    kill(pid, SIGTSTP);
    tcsetpgrp(STDIN_FILENO, getpgid(main_pid));
    tcsetpgrp(STDOUT_FILENO, getpgid(main_pid));
    return;
}
void sigint_handler(int sig)
{
    pid_t pid;
    int index =0;
    for(int i = 0; i<=numjobs; i++) {
        if(PIDs[i].status== FOREGROUND){
            pid = PIDs[i].pid;
            index =i;
            PIDs[i].status = NONE;
            PIDs[i].pid = 0;
            PIDs[i].jobid =0;
        }
    }
    if(pid != 0)
    {
        kill(pid, SIGINT);
    }
    return;
}
char *read_input() //------------- Change if there is some size specified for the input-------------
{
    char *input = NULL;
    ssize_t max_size =0;

    if(getline(&input, &max_size, stdin) != -1){
        //if we get no errors then we just return the input line
        return input;
    }
    //else we check for errors and exit
    if (feof(stdin)) 
    {
        exit(EXIT_SUCCESS);  // We recieved an EOF
    } 
    else  
    {
        perror("readline");
        exit(EXIT_FAILURE);
    }
}

#define MAX_LINE 80
#define DELIMS " \t\n\r\a"
char **get_content(char *input)
{
    //Declaring the required variables
    int curr_size = MAX_LINE; //max size for the string
    int index =0; //to track the position of char in the string
    char **terms = malloc(curr_size * sizeof(char*)); // Allocating a dynamic array to store 
    // the command and arguments
    char *cur_term; // This is the current argument
    cur_term = strtok(input, DELIMS); //Arguments seperated by specified delimeters (tokenizing)
    while(cur_term != NULL)
    {
        terms[index] = cur_term;
        index++;
        //Reallocating memory if command is longer than expected.
        if(index>=curr_size)
        {
            //Increasing the capacity of our array
            curr_size+=MAX_LINE;
            //extending the memory space
            terms = realloc(terms, curr_size* sizeof(char*));
        }
        //getting the next arg while we haven't reached the end of the command.
        cur_term = strtok(NULL, DELIMS);
        con_length+=1;
    }
    //making sure the dynamically allocated array is null terminated.
    terms[index] = '\0';
    return terms;

}

void test_print(char **args)
{
    char *cur = args[0];
    int pos =0;
    while(cur!=NULL)
    {
        printf("%s\n", cur);
        pos++;
        cur = args[pos];
    }
}

void pwd_function(){
    char cwd[256];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        perror("pwd error");
    }
    else
    {
        printf("%s\n", cwd);   
    }
}

void cd_function(char **content){
    if (chdir(content[1]) != 0) {
      printf("cd Error: No such directory\n");
    }
}

void add_job(pid_t pid, STATE status, char *cmndline)
{
    //printf("Adding job\n");
    extern int numjobs;
    numjobs++;
    JOBS bj = {numjobs+1, pid, status, cmndline};
    PIDs[numjobs] = bj;
    //printf("JobID: %d, PID: %d, Status: %d, Input: %s\n", PIDs[numjobs].jobid, PIDs[numjobs].pid, PIDs[numjobs].status, PIDs[numjobs].cmndline);
}
void IO_redirect(char **cont)
{
    char*out;
    //printf("IO Redirection function\n");
    if(input_redirect ==1 && output_redirect ==1)
    {
        out = cont[2];
    }
    else
    {
        if(con_length == 4)
        {
            //printf("More arguments\n");
            out = cont[2];
            //printf("Ouput file: %s\n", out);
        }
        else
        {
            out = cont[1];
            //printf("Ouput file: %s\n", out);
        }
    }
    if(input_redirect ==1)
    {
        char c;
        if(output_redirect==1)
        {
            out = cont[1];
        }
        int fdin = open(out, O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
        //printf("Reading the file\n");
        if(fdin == -1){
            perror("Failed to open the file");
            return;
        }
        if(dup2(fdin, STDIN_FILENO) == -1){
            perror("Failed to redirect");
            return;
        }
        //read(fdin, &c, 1);
        out = cont[2];
    }
    if(output_redirect ==1)
    {
        int fdout = open(out, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if(fdout == -1){
            perror("Failed to create file");
            return;
        }
        if(dup2(fdout, STDOUT_FILENO) == -1){
            perror("Failed to redirect output");
            return;
        }
        if(close(fdout) == -1){
            perror("Failed to close the output file");
            return;
        }
    }
    if(output_redirect_append ==1)
    {
        //printf("Ouput file: %s\n", out);
        int fdout = open(out, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IRWXU | S_IRWXG | S_IRWXO);
        if(fdout == -1){
            perror("Failed to append to file");
            return;
        }
        if(dup2(fdout, STDOUT_FILENO) == -1){
            perror("Failed to redirect output");
            return;
        }
        if(close(fdout) == -1){
            perror("Failed to close the output file");
            return;
        }
    }
}

void print_jobs()
{
    for(int i = 0; i<=numjobs; i++) {
        if(PIDs[i].status== BACKGROUND){
            printf("[%d] (%d) Running %s &\n", PIDs[i].jobid, PIDs[i].pid, PIDs[i].cmndline);
        }
        else if(PIDs[i].status== STOPPED){
            printf("[%d] (%d) Stopped %s\n", PIDs[i].jobid, PIDs[i].pid, PIDs[i].cmndline);
        }
    }
}
void general_cmd_function(char **content, char *input){
    extern pid_t pid;
    size_t n = sizeof(content)/sizeof(content[0]);
    //printf("n value: %ld\n", n);
    char *letter = content[n];
    //printf("letter: %s\n", letter);
    int bg;
    if(letter!= NULL)
    {
        bg = !strcmp(letter, "&");
    }
    if(bg)
    {
        content[n] = NULL;
        //test_print(content);
    }
    //printf("Background: %s %d\n", letter, bg);
    if ((pid = fork())== 0){
        setpgid(0, 0);
        if(input_redirect == 1 || output_redirect ==1 || output_redirect_append ==1)
        {
            //printf("I/O Redirection in general command");
            IO_redirect(content);
        }
        if (execv(content[0], content) < 0){
            if (execvp(content[0], content) < 0){
                printf("%s: Command not found.\n", content[0]);
                exit(0);
            }
        }
        
    }
    if (!bg) {   /* parent waits for fg job to terminate */
        int child_status;
        setpgid(pid, pid);
        //printf("fg process\n");
        add_job(pid, FOREGROUND, input);
        if (waitpid(pid, &child_status, WUNTRACED) < 0)
        {
            printf("waitfg: waitpid error");
        }
    }
    else
    {
        add_job(pid, BACKGROUND, input);
        return;
    }
}


void to_fg(char *input, char *job)
{
    int child_status;
    pid_t c_pd;
    //printf("To FG process\n");
    //printf("Job: %s\n", job);
    if(strcmp(job, "job") == 0){
        int jid = input[1] - '0';
        //printf("JID: %d\n", jid);
        for(int i = 0; i<=numjobs; i++) {
            if(PIDs[i].jobid == jid){
                //printf("Found jobid\n");
                PIDs[i].status = FOREGROUND;
                kill(PIDs[i].pid, SIGSTOP);
                tcsetpgrp(STDOUT_FILENO, getpgid(PIDs[i].pid));
                kill(PIDs[i].pid, SIGCONT);
                c_pd =PIDs[i].pid;
                pid =c_pd;
                break;
            }
        }
    }
    else{
        int pd = atoi(input);
        for(int i = 0; i<=numjobs; i++) {
            if(PIDs[i].pid == pd){
                //printf("Found pid\n");
                PIDs[i].status = FOREGROUND;
                kill(PIDs[i].pid, SIGSTOP);
                tcsetpgrp(STDOUT_FILENO, getpgid(PIDs[i].pid));
                kill(PIDs[i].pid, SIGCONT);
                c_pd =PIDs[i].pid;
                pid =c_pd;
                break;
            }
        }
    }
    waitpid(c_pd, NULL, WUNTRACED);
    tcsetpgrp(STDIN_FILENO, main_pid);
    return;
}
void to_bg(char *input, char *job)
{
    int child_status;
    pid_t c_pd;
    //printf("To BG process\n");
    //printf("Job: %s\n", job);
    if(strcmp(job, "job") == 0){
        int jid = input[1] - '0';
        //printf("JID: %d\n", jid);
        for(int i = 0; i<=numjobs; i++) {
            if(PIDs[i].jobid == jid){
                //printf("Found jobid\n");
                PIDs[i].status = BACKGROUND;
                //kill(PIDs[i].pid, SIGSTOP);
                //tcsetpgrp(STDOUT_FILENO, getpgid(PIDs[i].pid));
                kill(PIDs[i].pid, SIGCONT);
                c_pd =PIDs[i].pid;
                pid =c_pd;
                break;
            }
        }
    }
    else{
        int pd = atoi(input);
        for(int i = 0; i<=numjobs; i++) {
            if(PIDs[i].pid == pd){
                //printf("Found pid\n");
                PIDs[i].status = BACKGROUND;
                //kill(PIDs[i].pid, SIGSTOP);
                //tcsetpgrp(STDOUT_FILENO, getpgid(PIDs[i].pid));
                kill(PIDs[i].pid, SIGCONT);
                c_pd =PIDs[i].pid;
                pid =c_pd;
                break;
            }
        }
    }
    //waitpid(c_pd, NULL, WUNTRACED);
    tcsetpgrp(STDIN_FILENO, main_pid);
    return;
}
void kill_proc(char *input, char *job)
{
    //printf("Kill process\n");
    //printf("Job: %s\n", job);
    if(strcmp(job, "job") == 0){
        int jid = input[1] - '0';
        //printf("JID: %d\n", jid);
        for(int i = 0; i<=numjobs; i++) {
            if(PIDs[i].jobid == jid){
                pid_t pd = PIDs[i].pid;
                //int temp;
                if(PIDs[i].status = STOPPED)
                {
                    kill(pd, SIGCONT);
                }
                int temp;
                kill(pd, SIGINT);
                waitpid(pid, &temp, 0);
                PIDs[i].status = NONE;
                PIDs[i].pid = 0;
                PIDs[i].jobid =0;
                return;
            }
        }
    }
    else{
        int pd = atoi(input);
        for(int i = 0; i<=numjobs; i++) {
            if(PIDs[i].pid== pd){
                if(PIDs[i].status = STOPPED)
                {
                    kill(pd, SIGCONT);
                }
                int temp;
                kill(pd, SIGINT);
                waitpid(pid, &temp, 0);
                //waitpid(pd, &temp, 0);
                PIDs[i].status = NONE;
                PIDs[i].pid = 0;
                PIDs[i].jobid =0;
                return;
            }
        }
    }
}
char **split(char **content)
{
    int curr_size = MAX_LINE; //max size for the string
        extern int input_redirect;
        extern int output_redirect;
        extern int output_redirect_append;
        int index =0; //to track the position of char in the string
        char **cont = malloc(curr_size * sizeof(char*)); // Allocating a dynamic array to store 
        char *cur = content[0];
        int pos =0;
        int in =0;
        while(cur!=NULL)
        {
            if(strcmp(cur, "<") == 0)
            {
                input_redirect = 1;
            }
            else if(strcmp(cur, ">") == 0)
            {
                output_redirect = 1;
                //printf("Output Redirect\n");
            }
            else if(strcmp(cur, ">>") == 0)
            {
                output_redirect_append = 1;
            }
            else{
                cont[in] = cur;
                in++;
                //printf("Cont pos: %d, %s\n", pos, cont[pos]);
            }
            pos++;
            cur = content[pos];
        }
        cont[in] = NULL;
        return cont;
}

int exe_command(char **content, char *input)
{
    //printf("%d\n", con_length);
    //printf("Execute Command\n");
    char *builtins[] = {"cd", "pwd", "quit", "jobs", "bg", "fg", "kill"};
    char *command = content[0];
    //printf("%s\n", command);
    if(strcmp(command, "quit") == 0)
    {
        return 0;
    }
    int flag =0;
    int length = sizeof(builtins)/sizeof(builtins[0]);
    //int con_len = sizeof(content)/sizeof(content[0]);
    for(int i =0; i<length; i++)
    {
       if(strcmp(command, builtins[i]) == 0)
       {
        flag = 1;
       }
    }
    if(flag == 1)
    {
        if (strcmp(command, "pwd") == 0){
            pwd_function();
        }
        else if (strcmp(command, "cd") == 0){
            cd_function(content);
        }
        else if(strcmp(command, "jobs") == 0){
            //printf("Printing jobs\n");
            print_jobs();
        }
        else if(strcmp(command, "fg") == 0){
            //printf("Converting to fg\n");
            char *sarg = content[1];
            char c= sarg[0];
            //printf("Argument: %s \n", sarg);
            if(c == '%'){
                //printf("given job id\n");
                char *job = "job";
                to_fg(content[1], job);
            }
            else{
                //printf("given pid\n");
                char *job = "pid";
                to_fg(content[1], job);
            }
        }
        else if(strcmp(command, "bg") == 0)
        {
            //printf("Converting to bg\n");
            char *sarg = content[1];
            char c= sarg[0];
            //printf("Argument: %s \n", sarg);
            if(c == '%'){
                //printf("given job id\n");
                char *job = "job";
                to_bg(content[1], job);
            }
            else{
                //printf("given pid\n");
                char *job = "pid";
                to_bg(content[1], job);
            }
        }
        else if(strcmp(command, "kill") == 0)
        {
            //printf("Killing the process\n");
            char *sarg = content[1];
            char c= sarg[0];
            //printf("Argument: %s \n", sarg);
            if(c == '%'){
                //printf("given job id\n");
                char *job = "job";
                kill_proc(content[1], job);
            }
            else{
                //printf("given pid\n");
                char *job = "pid";
                kill_proc(content[1], job);
            }
        }
    } 
    else if(con_length > 1 && strcmp(content[1], "&")!=0 && content[2] != NULL)
    {
        if(strcmp(content[1], "<") == 0 || strcmp(content[1], ">") == 0 || strcmp(content[2], ">") == 0 || strcmp(content[1], ">>") == 0  || strcmp(content[2], ">>")== 0)
        {
            //printf("I/O Redirect Command\n");
            //Declaring the required variables
            char **cont = split(content);
            //printf("Printing Cont\n");
            //test_print(cont);
            general_cmd_function(cont, input);
        }
    }
    else {
        //printf("General command in execute\n");
        general_cmd_function(content, input);
    }
    //printf("came here\n");
    return 1;
}

//This function will prompt the user to enter the command and uses functions to read the lines 
//and quit when needed
int command_loop()
{
    // Declaring the variables
    char *input; // To store the entire input assuming that input can be any size
    char **content; //This will be the array of chars where each index is having the 
    //sub strings of the input
    int flag; //to know when to exit the program
    do{
        signal(SIGINT, sigint_handler);
        signal(SIGCHLD, sigchld_handler);
        signal(SIGTSTP, sigtstp_handler);
        signal(SIGTTOU, SIG_IGN);
        input_redirect =0;
        output_redirect =0;
        output_redirect_append =0;
        con_length= 0;
        printf("prompt > ");
        input = read_input();
        //printf("Completed read input: ");
        //printf("Input: %s", input);
        content = get_content(input);
        //printf("Completed get content function\n");
        //test_print(content);
        flag = exe_command(content, input);
    }while(flag);
    //Killing all the rest of the processes.
    for(int i =0; i<=numjobs; i++)
    {
        pid_t pid = PIDs[i].pid;
        //printf("Killing while quitting");
        if(PIDs[i].status = STOPPED)
        {
            kill(pid, SIGCONT);
        }
        kill(pid, SIGINT);
        int temp;
        waitpid(pid, &temp, 0);
        PIDs[i].status = NONE;
        PIDs[i].pid = 0;
        PIDs[i].jobid =0;
    }
    return 0;
}


int main()
{
    // First, we run the command loop where we continuously prompt the user to 
    // enter the command
    main_pid = getpid();
    command_loop();
}
