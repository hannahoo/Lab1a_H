// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#ifdef __APPLE__
#include <err.h>
#define error(args...) errc(args)
#else
#include <error.h>
#endif


/* FIXME: You may need to add #include directives, macro definitions,
 static function definitions, etc.  */
#include<unistd.h>
#include<fcntl.h>
#include<stdlib.h>
#include<sys/wait.h>
#include <sys/types.h>
#include<stdio.h>
#include<string.h>

/*
 int close(pid ) 0:close successfully 1: failed
 waitpid(pid,int *status,option) return pid and set status as child process exit_status
 WEXITSTATUS() return exit_no of child process
 */

void execute_simple(command_t c);
void execute_sequence(command_t c);
void execute_and(command_t c);
void execute_or(command_t c);
void execute_subshell(command_t c);
void execute_pipe(command_t c);
int
command_status (command_t c)
{
    return c->status;
}

void
execute_command (command_t c, bool time_travel)
{
    /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
    if (c==NULL)
        error (1, 0, "command execution not yet implemented");
    switch(c->type)
    {
        case SIMPLE_COMMAND:
            execute_simple(c);
            break;
        case SEQUENCE_COMMAND:
            execute_sequence(c);
            break;
        case AND_COMMAND:
            execute_and(c);
            break;
        case OR_COMMAND:
            execute_or(c);
            break;
        case SUBSHELL_COMMAND:
            execute_subshell(c);
            break;
        case PIPE_COMMAND:
            execute_pipe(c);
            break;
        default:
            error (1, 0, "command type illegal");
            
    }
}

void execute_simple(command_t c)// c->word
{
    
    int fd[2]={-1,-1};
    if (c->input != NULL)
    {
        fd[0] = open(c->input, O_RDONLY,0777);//set for permission? like 644
        if (fd[0] < 0)// 0 1 2
            error(1, 0, "can't open input file simple");
    }
    if (c->output != NULL)
    {
        fd[1] = open(c->output, O_WRONLY | O_CREAT | O_TRUNC,0644);
        if (fd[1] < 0)
            error(1, 0, "can't open output file simple");
    }
    int p=fork();
    if(p<0)
        error(1,0,"fork failed: simple");
    if(p==0)// child process
    {
        if(fd[0]>=0)
            if(dup2(fd[0],0)<0)
                error(1,0,"can't read input file");
        if(fd[1]>=0)
            if(dup2(fd[1],1) <0)
                error(1,0,"can't write output file");
        //need to deal with special case when first word is "exec". in this case, execvp calls u.word[1], and &u.word[1]
        if (strcmp(c->u.word[0],"exec")==0)
        {
            execvp(c->u.word[1],&(c->u.word[1]));
        }
        else
        {
            execvp(c->u.word[0],c->u.word);// 0!!
        }
        _exit(127);// when fails, stop child process
        
    }
    else if(p>0){
        int status;
        if(waitpid(p,&status,0)<0)// 0: blocking wait father process call wait
            error(1,0 ,"waited process error simple");
        c->status=WEXITSTATUS(status);
    }
    
    if(fd[0]>=0)
        close(fd[0]);
    if(fd[1]>=0)
        close(fd[1]);
    // c->status=exit_status;
    
    
}
// which order?
void execute_and(command_t c)// c->command[2]
{
    int p=fork();
    if(p==0){
        execute_command(c->u.command[0],1);
        _exit(c->u.command[0]->status);
    }
    else if(p>0){
        int status;
        
        if(waitpid(p,&status,0)<0)//wait for child process exit to get signal
            error(1,0,"waited process error and 1");
        //waitpid return the pid of child process
        int exit_status=WEXITSTATUS(status);// in lib # include<sys/wait.h> 0-127
        if(exit_status==0)// successfully run
        {
            int q=fork();
            if(q<0)
                error(1,0,"fork failed: and 2");
            if(q==0)
            {
                execute_command(c->u.command[1],1);//execute status of right side
                _exit(c->u.command[1]->status);
            }
            
            else {
                if(waitpid(q,&status,0)<0)//wait for child process exit to get signal
                    error(1,0,"waited process error and 2");
                exit_status=WEXITSTATUS(status);
                c->status=exit_status;
            }
            
        }
        else if (exit_status>0)//first command run unsuccessfully
        {
            c->status=exit_status;//set exit status to child process
        }
        
    }
    else
        error(1,0,"fork failed: and 1");
}
void execute_or(command_t c)// c->command[2]
{
    int p=fork();
    if(p<0)
        error(1,0,"fork failed: or 1");
    
    if(p==0){
        execute_command(c->u.command[0],1);
        _exit(c->u.command[0]->status);
    }
    else { // p>0 parent process
        int status;
        if(waitpid(p,&status,0)<0)
            error(1,0,"waited process error or 1");
        
        int exit_status=WEXITSTATUS(status);// in lib # include<sys/wait.h> 0-127
        if(exit_status!=0)// unsuccessfully run, then run second process
        {
            int q=fork();
            if(q<0)
                error(1,0,"fork failed: or 2");
            if(q==0)
            {
                execute_command(c->u.command[1],1);
                _exit(c->u.command[1]->status);
            }
            else
            {
                if(waitpid(q,&status,0)<0)
                    error(1,0,"waited process error or 2");
                c->status=WEXITSTATUS(status);
            }
        }
        else
            c->status=exit_status;// only depend on the first process
        
    }
    
    
}

void execute_sequence(command_t c) // wait for child to exit
{
    int p=fork();
    if(p<0)
        error(1,0,"fork failed: sequence 1");
    
    if(p==0){
        execute_command(c->u.command[0],1);
        _exit(c->u.command[0]->status);
    }
    else {
        int status;
        if(waitpid(p,&status,0)<0)
            error(1,0,"waited process error sequence 1");
        //	c->status=WEXITSTATUS(status);// wait until the second process exit
        int q=fork();
        if(q<0)
            error(1,0,"fork failed: sequence 2");
        if(q==0)
        {
            execute_command(c->u.command[1],1);
            _exit(c->u.command[1]->status);
        }
        else { // p>0 q>0 parent process
            
            if(waitpid(q,&status,0)<0)// 0: blocking wait
                error(1,0 ,"waited process error sequence 2");
            c->status=WEXITSTATUS(status);// another??!!!
        }
    }
    
}
void execute_subshell(command_t c)// c->subshell_command->(word,command[2])
{
    
    int fd[2]={-1,-1};
    if(c->input!=NULL)
    {
        fd[0]=open(c->input,O_RDONLY,644);//READ FILE
        if(fd[0]<0)
            error(1,0,"can not open input file!");
    }
    if(c->output !=NULL)
    {
        fd[1]=open(c->output, O_CREAT|O_TRUNC|O_WRONLY,0644);//OWNER|GROUP|OTHER
        if(fd[1] <0)
            error(1,0,"can not open output file!");
    }
    int p=fork();
    if(p<0)
        error(1,0,"fork failed subshell");
    
    if(p==0){ //fork successfully
        if(fd[0]>=0)
            if(dup2(fd[0],0)<0) error(1,0,"dup2 error input file");
        if(fd[1]>=0)
            if(dup2(fd[1],1)<0) error(1,0,"dup2 error in output file");
        
        execute_command(c->u.subshell_command,1);
        _exit(c->u.subshell_command->status); //
        
    }
    else {
        int status;
        if(waitpid(p,&status,0)<0)
            error(1,0,"waited process error: subshell");
        c->status = WEXITSTATUS(status);
        
    }
    
    if(fd[0]>=0) close(fd[0]);
    if(fd[1]>=0) close(fd[1]);
    
}
void execute_pipe(command_t c)// c->command[2]
{
    int fd[2];
    //pipe(fd): creat a pipe and fill fd[0]=reading fd[1]: writing
    //______________
    //______________
    //fd[1]	       fd[0]
    //secondpid      firstpid
    // no need to call waitpid since the firstpid will wait in the pipe
    if (pipe(fd)<0)//to replace the pipe(fd) below
        error(1,0,"pipe fd fail");
    pid_t firstpid=fork();
    
    if (firstpid<0)
        error(1,0,"fork failed: pipe 1");
    
    if (firstpid==0)//execute right side of the pipe
    {
        if(fd[1]>=0)
            close(fd[1]);
        if (dup2(fd[0],0)<0)
            error(1,0,"dup2 fail in pipe");//check validity
        execute_command(c->u.command[1],1);//TA code doesn't have time travel 1, no need this for 1B
        _exit(c->u.command[1]->status);//propogate exit status to the parent
    }
    else //parent execute this code
    {
        pid_t secondpid=fork();
        if (secondpid<0)
            error(1,0,"fork fail: pipe 2");
        if (secondpid==0)//child 2 is using fd[1], so close fd[0]
        {
            if(fd[0]>=0)
                close (fd[0]);
            if (dup2(fd[1],1)<0)
                error(1,0,"dup2 fail in pipe");
            execute_command(c->u.command[0],1);
            _exit(c->u.command[0]->status);
        }
        else//parent
        {
            if(fd[0]>=0)
                close(fd[0]);
            if(fd[1]>=0)
                close(fd[1]);
            int status;
            pid_t returnpid=waitpid(-1,&status,0);//
            if (secondpid==returnpid)
                waitpid(firstpid,&status,0);//two processes, need to wait another to finish
            if (firstpid==returnpid)
                waitpid(secondpid,&status,0);
            c->status=WEXITSTATUS(status);//extract exit status, least sig 8 bits
        }
    }
    
    
}


/*************time travel*****************/

//void execute_no_dependency(queue* no_dependency);
//void execute_dependency(queue* dependency);

//int execute_graph(dependency_graph){}

void execute_no_dependency(queue_t no_dependency);
int  execute_dependency(queue_t dependency);

int execute_graph(dependency_t dependency_graph)
{
    execute_no_dependency(dependency_graph->no_dependency);
    
    int final_status=execute_dependency(dependency_graph->dependency);
    return final_status;///
}

void execute_no_dependency(queue_t no_dependency)
{
    queue_node_t queue_node_cursor=no_dependency->head;
    while (queue_node_cursor != NULL)
    {
        pid_t pid=fork();
        if (pid==0)
        {
            execute_command(queue_node_cursor->g->command,true);
            exit(0);
        }
        else
        {
            queue_node_cursor->g->pid=pid;
        }
        
        queue_node_cursor=queue_node_cursor->next;
    }
    
    /*
     graph_node_t graph_node_cursor=no_dependency->head->g;
     while (graph_node_cursor->next!=NULL)
     {
     
     graph_node_cursor=graph_node_cursor->next;
     }
     //graph node
     
     pseudo-code:
     for each GraphNode i in no_dependencies
     Pid_t pid=fork()
     if(pid==0)
     {
     execute_command(i->command tree)
     exit 0;
     }
     else
     {
     i->pid=pid;
     }
     
     */
}

int execute_dependency(queue_t dependency)
{
    int status;
    queue_node_t queue_node_cursor=dependency->head;
    // for each node in queue
    while (queue_node_cursor != NULL)
    {
        
        //for each graph node j in i->before
        //   struct graph_node** before;
        //loop through pointer of pointer
        //know the size of the before list
        for (int i=0; i<queue_node_cursor->g->size_before_list;i++)
        {
            waitpid(queue_node_cursor->g->before[i]->pid,&status,0);
        }// for loop ends here
        // no dependency now!
        pid_t pid=fork();
        if(pid==0)
        {
            execute_command(queue_node_cursor->g->command,true);
            exit(0);
        }
        else
        {
            queue_node_cursor->g->pid=pid;
        }
        // update cursor here
        queue_node_cursor=queue_node_cursor->next;
    }
    return status;
}
//graph node
/*
 pseudo code:
 
 for each GraphNode i in dependencies
 int status
 for each GraphNode j in i->before
 waitpid(j->pid, &status,0);
 pid_t pid=fork();
 if(pid==0)
 execute_command(i->command)
 exit(0)
 else
 i->pid=pid;
 
 */

//other edge cases? after list?
