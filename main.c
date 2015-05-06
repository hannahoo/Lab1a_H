// UCLA CS 111 Lab 1 main program

#include <errno.h>

#ifdef __APPLE__
#include <err.h>
#define error(args...) errc(args)
#else
#include <error.h>
#endif

#include <getopt.h>
#include <stdio.h>

#include "command.h"

static char const *program_name;
static char const *script_name;

dependency_t create_graph(command_stream_t s);
int execute_graph(dependency_t graph);

static void
usage (void)
{
    error (1, 0, "usage: %s [-pt] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
    return getc (stream);
}

int
main (int argc, char **argv)
{
    
    
    
    int command_number = 1;
    bool print_tree = false;
    bool time_travel = false;
    program_name = argv[0];
    
    for (;;)
        switch (getopt (argc, argv, "pt"))
    {
        case 'p': print_tree = true; break;
        case 't': time_travel = true; break;
        default: usage (); break;
        case -1: goto options_exhausted;
    }
options_exhausted:;
    
    // There must be exactly one file argument.
    if (optind != argc - 1)
        usage ();
    
    script_name = argv[optind];
    FILE *script_stream = fopen (script_name, "r");
    if (! script_stream)
        error (1, errno, "%s: cannot open", script_name);
    command_stream_t command_stream =
    make_command_stream (get_next_byte, script_stream);
    
    command_t last_command = NULL;
    command_t command;
    
    // dependency_t create_graph(command_stream_t s);
    //int execute_graph(dependency_t graph);
    
    /***********add the if else statement *********/
    //add the time_travel case, can parallelize with the -t flag
    if (time_travel)
    {
        
        dependency_t graph=create_graph(command_stream); //inside read_command.c
        int final_status=0;
        final_status=execute_graph(graph); //inside execute_command.c
        return final_status;
        
        
        
    }
    else
    {
        
        while ((command = read_command_stream (command_stream)))
        {
            if (print_tree)
            {
                printf ("# %d\n", command_number++);
                print_command (command);
            }
            else
            {
                last_command = command;
                execute_command (command);
            }
        }
        
        return print_tree || !last_command ? 0 : command_status (last_command);
        
    }
    
}
