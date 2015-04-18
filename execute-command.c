// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
#include<unistd.h>
#include<fcntl.h>
#include<stdlib.h>
//#include<sys/wait.h>
#include<stdio.h>

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
	int p=fork();
	int fd[2];
	if (c->input != NULL)
  {
    fd[0] = open(c->input, O_RDONLY);
    if (fd[0] < 0)// 0 1 2 
      error(1, 0, "can't open input file");
  }
  if (c->output != NULL)
  {
    fd[1] = open(c->output, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd[1] < 0)
      error(1, 0, "can't open output file");
  }
  if(p==0)// child process
  {
  	if(fd[0]>=0)
  	if(dup2(fd[0],0)<0)
  	error(1,0,"can't read input file");
  	if(fd[1]>=0)
  	if(dup2(fd[1],1) <0)
  	error(1,0,"can't write output file");
  	execvp(c->u.word[0],c->u.word);// 0!!
  	exit(127);// when fails
  	
  }
  else{
  	int status;
  	if(wait_pid(p,&status,0)<0)// 0: blocking wait
  		error(1,0 ,"waited process error");
  	int exit_status=WEXITSTATUS(status);
  	if(fd[0]>=0)
  		close(fd[0]);
  	if(fd[1]>=0)
  		close(fd[1]);
  	c->status=exit_status;
  } 
}
// which order?
void execute_and(command_t c)// c->command[2]
{
	int p=fork();
	if(p==0){
		execute_command(c->u.command[0],1);
	}
	else{
		int status;
		wait_pid(p,&status,0);
		int exit_status=WEXITSTATUS(status);// in lib # include<sys/wait.h> 0-127
		if(exit_status==0)// successfully run
		{
			int q=fork();
			if(q==0)
			execute_command(c->u.command[1],1);	
		}
		
	}
}
void execute_or(command_t c)// c->command[2]
{
	int p=fork();
	if(p==0){
		execute_command(c->u.command[0],1);
	}
	else{
		int status;
		wait_pid(p,&status,0);
		int exit_status=WEXITSTATUS(status);// in lib # include<sys/wait.h> 0-127
		if(exit_status!=0)// unsuccessfully run
		{
			int q=fork();
			if(q==0)
			execute_command(c->u.command[1],1);	
		}
		
	}
}
void execute_sequence(command_t c) // wait for child to exit
{
	int p=fork();
	if(p==0){
		execute_command(c->u.command[1],1);	
	}
	else{
		int q=fork();
		if(q==0)
			execute_command(c->u.command[0],1);
		else{
			int status;
  			if(wait_pid(p,&status,0)<0)// 0: blocking wait
  				error(1,0 ,"waited process error");
  			int exit_status=WEXITSTATUS(status);
  			c->status=exit_status;// another??!!!
	}
	}

}
void execute_subshell(command_t c)// c->subshell_command->(word,command[2])
{
	int p=fork();
	int fd[2]={-1,-1};
	if(c->input!=NULL)
	fd[0]=open(c->input,O_RDONLY);//READ FILE
	if(c->output !=NULL)
	fd[1]=open(c->output, O_CREAT|O_TRUNC|O_WRONLY,0644);//OWNER|GROUP|OTHER
	
	if(p==0){
		if(fd[0]>=0)
		if(dup2(fd[0],0)<0) error(1,0,"in open input file");
		if(fd[1] >=0)
		{
			if(dup2(fd[1],1)<0) error(1,0,"in open output file");
		}
	
		
		execute_command(c->u.subshell_command,1);	
	}
	if(fd[0]>=0) close(fd[0]);
	if(fd[1]>=0) close(fd[1]);
	
}
void execute_pipe(command_t c)// c->command[2]
{
	int fd[2];
	pipe(fd);//system call
//______________
//______________
//fd[1]	       fd[0]
//secondpid      firstpid
// no need to call waitpid since the firstpid will wait in the pipe
// synchronization is done by pipe 	
int firstpid=fork();
if(firstpid==0)
{
	close(fd[1]);// close unused write command
	dup2(fd[0],0);//read end (direction)
	execute_command(c->u.command[0],1);//?
}
else{
	int secondpid=fork();
	if(secondpid==0){
			close(fd[0]);//
			dup2(fd[1],1);//intead of write to stdout, write to pipe 
			execute_command(c->u.command[1],1);
	}	
	else{
			close(fd[0]);
			close(fd[1]);
			// parent process is not part of the pipe: establish pipe between child1 and child2
			// parent need to close the pipe
			int status;
			waitpid(-1,&status,0);//
			waitpid(-1,&status,0);// 2 child process will finish
	}
		
}
}
