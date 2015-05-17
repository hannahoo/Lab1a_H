// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include "alloc.h"


#ifdef __APPLE__
#include <err.h>
#define error(args...) errc(args)
#else
#include <error.h>
#endif

/* FIXME: You may need to add #include directives, macro definitions,
 static function definitions, etc.  */

//additional includes
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

//end of additional includes
const int  OPERATOR_LEN =8;
const int WORD_LEN =16;
const int INT_STACK_LEN=16;
const int MAX_BEFORE=20;

/* FIXME: Define the type 'struct command_stream' here.  This should
 complete the incomplete type declaration in command.h.  */

//auxiliary functions declarations
bool isWord (char c);
bool isSpecial (char c);
bool isNotValid (char c);
enum command_type cmd_type(char *cc);
command_t init_command(enum command_type type);
command_stream_t init_stream();
command_t store_simple_command (char *c, int *i, size_t size);
int precedence (char *operator);
char* get_next_word (char *c, int *i, size_t size);
void init_op_stack();
void init_cmd_stack();


//global variable
int count_left_bracket=0;
int line_number =1;
bool wait_input =false; //sign for <
bool wait_output =false; // >
bool both_in_out = false; //<>
bool has_option_c=false;
int in_mode;
int out_mode;

//auxiliary functions
bool isWord (char c)
{
    return (isalpha(c) || isdigit(c) || (c == '!') || (c == '%') || (c == '+') || (c == ',') ||
            (c == '-') || (c == '.') || (c == '/') || (c == ':') || (c == '@') || (c == '^') || (c == '_') );
    
}

bool isSpecial (char c)
{
    return ( (c == ';') || (c == '|') || (c == '(') || (c == ')') || (c == '<') || (c == '>') || (c == '&'));
}

bool isNotValid (char c)
{
    return ( (c!=' ') && (c!='#') && (c!='\n') && (c!= '\t') &&(!isWord(c)) && (!isSpecial(c)) );
}
//end of auxiliary functions

//initialize command
enum command_type cmd_type(char *cc)
{
    
    if(!strcmp(cc,"&&")) return AND_COMMAND;
    if(!strcmp(cc,"||")) return OR_COMMAND;
    if(!strcmp(cc,"|")) return PIPE_COMMAND;
    if(!strcmp(cc,";")) return SEQUENCE_COMMAND;
    if(!strcmp(cc,"(")) return SUBSHELL_COMMAND;
    else {
        err(1, "invalid type");
        return SIMPLE_COMMAND;
    }
    
}
command_t init_command(enum command_type type)
{
    command_t command=(command_t)checked_malloc(sizeof(struct command));
    command->type=type;
    command->status=-1;
    command->input_mode=-1;// initial
    command->output_mode=-1;
    command->input=NULL;
    command->output=NULL;
    command->u.word=NULL;
    command->u.command[0]=NULL;
    command->u.command[1]=NULL;
    command->u.subshell_command=NULL;
    
    /* put this inside build command tree
     if(type == SUBSHELL_COMMAND)
     {
     command->u.subshell_command= command_t checked_malloc(sizeof(struct command));
     }
     */
    return command;
}


//initialize command stream
command_stream_t init_stream()
{
    command_stream_t stream=(command_stream_t) checked_malloc(sizeof(struct command_stream));
    stream->head=NULL;
    stream->tail=NULL;
    stream->cursor=NULL;
    return stream;
}

char* get_next_word (char *c, int *i, size_t size  )
{
    int starting=*i;
    int size_word=0;
    while (isWord(c[*i]) && (*i)<size)
    {
        size_word++;
        (*i)++;
    }
    char *word=(char *)checked_malloc(sizeof(char)*(size_word+1) );
    if (size_word == 0)
        return NULL;
    int j=0;
    while (j<size_word)
    {
        word[j]=c[starting+j];
        j++;
    }
    word[size_word]='\0';
    if (size_word==2 && strcmp(word,"-C")==0)
        has_option_c=true;
    return word;
}

//create simple command (consists of one or more words)
command_t store_simple_command (char *c, int *i, size_t size ) // to avoid similar name with build-in function
{
    command_t command= init_command(SIMPLE_COMMAND);
    int size_word=1;
    int count_w=0;
    command->u.word=(char**)malloc(sizeof(char*)*size_word);
    
    // store words instead of store pointers
    for(int j=0;j<size_word;j++) // j instead of i
        command->u.word[j]=(char*)malloc(sizeof(char)*size);
    
    
    while ((*i)<size)
    {
        char tmp_c=c[*i];
        //if meet operator, then return command
        if (tmp_c==' ' || tmp_c=='\t')
        {
            (*i)++;
        }
        else if (isWord(tmp_c))
        {
            char* w= get_next_word(c, i, size);
            if (count_w==size_word)
            {
                size_word=size_word*2;
                command->u.word=realloc(command->u.word,sizeof(char*)*size_word);
            }
            command->u.word[count_w]=w;//entire word
            count_w++;
            command->u.word[count_w]=NULL;
        }
        else if (isNotValid(tmp_c))
        {
            fprintf(stderr,"line %d: character, '%c', is not valid. \n", line_number, tmp_c );
            exit(1);
        }
        else//meet an operator of any sort or others ...i.e. \n, < > #
        {
            (*i)--;//can deal with it later in make_comamnd_tree function
            break;
        }
    }
    return command;
}


//precedance
int precedence (char *operator)
{
    /*
     lowest --> highest
     ; \n    1
     &&/||   2
     |       3
     */
    if (strcmp(operator,";")==0)
        return 1;
    else if (strcmp(operator,"\n")==0)
        return 1;
    else if (strcmp(operator,"&&")==0)
        return 2;
    else if (strcmp(operator,"||")==0)
        return 2;
    else if (strcmp(operator,"|")==0)
        return 3;
    else
        return -1;
}
//***********************************************************************************


//build command tree


//stack def
struct op_stack{
    char** operator;
    int len;
    int top;
    
}op_s; // global var

void init_op_stack()
{
    
    op_s.len=INT_STACK_LEN;
    op_s.top=0;
    op_s.operator=(char**)checked_malloc(op_s.len*sizeof(char*));
    for(int i=0;i<op_s.len;i++)
    {
        op_s.operator[i]=(char*)checked_malloc(OPERATOR_LEN*sizeof(char));
    }
    
}

struct cmd_stack{
    command_t* command;
    int len;
    int top;
}cmd_s;
void init_cmd_stack(){
    cmd_s.len=INT_STACK_LEN;
    cmd_s.top=0;
    cmd_s.command=(command_t*)checked_malloc(cmd_s.len * sizeof( command_t));
    for (int i=0 ; i<cmd_s.len; i++) {
        cmd_s.command[i]=(command_t)checked_malloc(sizeof(struct command));
    }
}


void push_op(char* cc)
{
    if (op_s.len==(op_s.top))  // get larger space
        
    {
        op_s.operator=(char**) realloc(op_s.operator, op_s.len*2*sizeof(char*));
        for (int i=op_s.len; i<2*op_s.len; i++)
            op_s.operator[i]=(char*) checked_malloc(OPERATOR_LEN * sizeof(char) ) ;
        op_s.len= 2 * op_s.len;
    }
    op_s.top++;
    strcpy(op_s.operator[op_s.top-1],cc);
    op_s.operator[op_s.top-1][strlen(cc)]='\0';
    //debug
    if (op_s.operator[op_s.top-1]==NULL) {
        error(1, 0, "%d: syntax error null op top push", line_number);
    }
}
char* pop_op()
{
    if (op_s.top<=0) {
        error(1, 0, "%d: syntax error 1 operater stack invalid pop", line_number);
    }
    
    else{
        //strcpy(cc,op_s.operator[op_s.top-1]);
        char* t=op_s.operator[op_s.top-1];
        //op_s.operator[op_s.top-1]=NULL;
        op_s.top--;
        return t;
    }
    
}

void push_cmd( command_t cc)
{
    if (cmd_s.len==cmd_s.top) { // get larger space
        cmd_s.command=(command_t*)realloc(cmd_s.command, 2*cmd_s.len*sizeof(command_t));// realloc store previous content
        for (int i=cmd_s.len; i<2*cmd_s.len; i++)
            cmd_s.command[i]=(command_t) checked_malloc(sizeof(struct command));
        cmd_s.len=2*cmd_s.len;
        
    }
    cmd_s.top++;
    cmd_s.command[cmd_s.top-1]=cc; // copy store
    //debug
    if (cmd_s.command[cmd_s.top-1]==NULL) {
        error(1, 0, "%d: syntax error null command top push", line_number);
    }
}

command_t pop_cmd()
{
    if (cmd_s.top<=0) {
        error(1, 0, "%d: syntax error 2 command stack invalid pop", line_number);    }
    else{
        command_t cc=cmd_s.command[cmd_s.top-1];
        //cmd_s.command[cmd_s.top-1]=NULL;
        cmd_s.top--;
        return cc;
    }
}

//********************************************************

command_t build_command_t(char* buff, int* index, size_t ssize)// size=buff--read_size
{
    // init operator stack and command stack
    
    init_op_stack();
    init_cmd_stack();
    // init global varible
    count_left_bracket=0;
    wait_output=false;
    wait_input=false;
    in_mode=-1;
    out_mode=-1;
    bool isReturn=false;
    while (*index<ssize &&(isReturn==false))
    {
        int next= *index+1;// used to detect || &&
        switch (buff[*index]){
            case '(' :
                count_left_bracket++;
                push_op("(");
                break;
            case ';' :
                // ;;
                if (op_s.top-count_left_bracket>=cmd_s.top ) {
                    error(1, 0, "%d: syntax error 3 dismatched operator and command", line_number);
                }
                // >;
                if(wait_input || wait_output)
                    error(1, 0, "%d: syntax error 3-2 < > happens at wrong place", line_number);
                // \n ;
                int i=(*index)-1;
                while ((i>=0)&&((buff[i]==' ')||(buff[i])=='\t')){ // skip all ' ' to find if the previous is \n : || b
                    i--;
                }
                if (buff[i]=='\n') {
                    error(1, 0, "%d: syntax error 4 \n appears in wrong place", line_number);
                }
                
                
                if (op_s.top ==0)
                    push_op(";");
                else{
                    
                    while ( (op_s.top>0)&&(precedence(op_s.operator[op_s.top-1])>=precedence(";")) &&(strcmp(op_s.operator[op_s.top-1],"(")!=0) )
                    {
                        char* tmp_op8= pop_op();
                        command_t tmp8=init_command(cmd_type(tmp_op8));
                        tmp8->u.command[1]=pop_cmd(  );
                        tmp8->u.command[0]=pop_cmd( );
                        push_cmd( tmp8);
                        
                    }
                    push_op( ";");
                    
                }
                // ; \n regard as consistent
                if ((*index+1<ssize)&&buff[*index+1]=='\n') { // regard \n after; as a ;
                    *index=(*index)+1;
                    line_number++;
                }
                break;
            case '|':
                if(wait_input || wait_output)
                    error(1, 0, "%d: syntax error 5-1 < > happens at wrong place", line_number);
                // check if /n operator
                int check=*index-1;
                while((check>=0) &&(( buff[check]==' ')||(buff[check]=='\t')||(buff[check]=='\n'))){
                    if(buff[check]=='\n')
                        error(1,0,"%d%c%c: syntax error 5-2 \n happends before operator |",line_number,buff[check],buff[check+1]);
                    check--;
                    //	printf("%d : %c\n",line_number,buff[check]);
                }
                if ((next<ssize)&&buff[next]=='|')  // case: ||
                {
                    *index=(*index)+1;
                    next=(*index)+1;
                    int i=(*index)-1;
                    while ((buff[i]!=' ') && (i>=0)) { // skip all ' ' to find if the previous is \n : || b
                        i--;
                    }
                    if (buff[i]=='\n') {
                        error(1, 0, "%d: syntax error 5-2 \n appears in wrong place", line_number);
                    }
                    if (op_s.top-count_left_bracket>=cmd_s.top) {
                        error(1, 0, "%d: syntax error 6 dismatched operator and command",line_number);
                    }
                    
                    if (op_s. top==0)
                        push_op("||");
                    else{
                        
                        while ( (op_s.top>0)&&(precedence(op_s.operator[op_s.top-1])>=precedence("||")) &&(strcmp(op_s.operator[op_s.top-1],"||")!=0))
                        {
                            //char* tmp_op=(char*) malloc(OPERATOR_LEN*sizeof(char));;
                            //pop_op(tmp_op);
                            char* tmp_op7= pop_op();
                            command_t tmp7=init_command(cmd_type(tmp_op7));
                            tmp7->u.command[1]=pop_cmd( );
                            tmp7->u.command[0]=pop_cmd( );
                            push_cmd( tmp7);
                        }
                        push_op( "||");
                    }
                }
                else{ // case |
                    
                    if (op_s.top ==0)
                        push_op( "|");
                    else{
                        while ( (precedence(op_s.operator[op_s.top-1])>=precedence("|")) &&(strcmp(op_s.operator[op_s.top-1],"|")!=0)&&(op_s.top>0))
                        {
                            char* tmp_op6=pop_op();
                            command_t tmp6=init_command(cmd_type(tmp_op6));
                            tmp6->u.command[1]=pop_cmd( );
                            tmp6->u.command[0]=pop_cmd();
                            push_cmd( tmp6);
                        }
                        push_op("|");
                    }
                }
                
                // skip all space unitl find a word
                while ((*index<ssize-1)&&((buff[*index+1]==' ')||(buff[*index+1]=='\t')||(buff[*index+1]=='\n'))) {
                    if (buff[*index+1]=='\n') {
                        line_number++;
                    }
                    
                    *index=*index+1;
                }
                
                break;
            case '&':
                if(wait_input || wait_output)
                    error(1, 0, "%d: syntax error 9 < > happens at wrong place", line_number);
                check=*index-1;
                while((check>=0) &&((buff[check]==' ')||( buff[check]=='\n')||(buff[check]=='\t')))
                {
                    if(buff[check]=='\n')
                        error(1,0, "syntax error 9-2 \n happens before &");
                    check--;
                }
                if((next<ssize)&&buff[next]=='&')
                {
                    *index=(*index)+1;
                    if (op_s.top-count_left_bracket>=cmd_s.top) {
                        error(1, 0, "%d: syntax error 9  dismatched operator and command",line_number);
                    }
                    //valid
                    if (op_s.top ==0)
                        push_op( "&&");
                    else{
                        while ((op_s.top>0)&& (precedence(op_s.operator[op_s.top-1])>=precedence("&&")) &&(strcmp(op_s.operator[op_s.top-1],"&&")!=0))
                        {
                            char* tmp_op5=pop_op();
                            command_t tmp5= (command_t)malloc(sizeof(struct command));
                            tmp5=init_command(cmd_type(tmp_op5));
                            tmp5->u.command[1]=pop_cmd( );
                            tmp5->u.command[0]=pop_cmd( );
                            push_cmd( tmp5);
                        }
                        push_op( "&&");
                    }
                }
                else
                    error(1, 0, "%d: syntax error 10 invalid & ", line_number);
                
                // skip all space
                while ((*index<ssize-1)&&((buff[*index+1]==' ')||(buff[*index+1]=='\t')||(buff[*index+1]=='\n'))) {
                    if (buff[*index+1]=='\n') {
                        line_number++;
                    }
                    *index=*index+1;
                }
                break;
                
            case ')':
                /*if (op_s.top>=cmd_s.top)
                 error(1, 0, "%d: syntax error 11  dismatched operator and command",line_number);*/
                if(wait_input || wait_output)
                    error(1, 0, "%d: syntax error 11-1 < or > happens at wrong place", line_number);
                
                
                while ((op_s.top>=1)&&(strcmp(op_s.operator[op_s.top-1],"(")!=0))
                {
                    char* tmp_op4=pop_op();
                    command_t tmp4=init_command(cmd_type(tmp_op4));
                    tmp4->u.command[1]=pop_cmd( );
                    tmp4->u.command[0]=pop_cmd( );
                    push_cmd( tmp4);
                }
                
                if ((strcmp(op_s.operator[0],"(")) &&(op_s.top==0)) {
                    error(1, 0, "%d: syntax error 11-2 \n appears in wrong place", line_number);
                    
                }
                pop_op();
                
                count_left_bracket--;
                // (  )
                command_t tmp3=init_command(SUBSHELL_COMMAND);
                tmp3->u.subshell_command= (command_t) checked_malloc(sizeof(struct command));
                tmp3->u.subshell_command=pop_cmd();
                push_cmd( tmp3);
                
                break;
            case '<':
                if (((op_s.top-count_left_bracket)>=cmd_s.top))
                    error(1, 0, "%d: syntax error 12 dismatched operator and command",line_number);
                
                if((next<ssize)&&buff[next]=='\n')
                    error(1, 0, "%d: syntax error 13-1 < followed by newline ", line_number);
                if(wait_input || wait_output||both_in_out)
                    error(1, 0, "%d: syntax error 13-2 < happens at wrong place", line_number);
                // input mode
                //initial -1, standard 0, <>:1 ?, <&: 2
                
                if((*index+1)<ssize){
                    switch(buff[*index+1]){
                            
                        case '>':
                            in_mode=1;
                            *index=*index+1;
                            out_mode=1;
                            //wait_output=true;
                            both_in_out=true;//special treatment 
                            break;
                        case '&':
                            in_mode=2;
                            *index=*index+1;
                            wait_input=true;
                            break;
                        case ' ':
                            in_mode=0;
                            wait_input=true;
                            break;
                        default: // standard : \0 or character //also need to check for invalid operator <
                            if (isWord(buff[*index+1]))
                                in_mode=0;
                            else
                                error(1, 0, "%d: invalid input operator ", line_number);
                            wait_input=true;
                            
                            //in_mode=0;
                    }
                    
                }
                //wait_input=true;
                break;
                
            case '>':
                if((op_s.top-count_left_bracket)>=cmd_s.top)
                    error(1, 0, "%d: syntax error 14 dismatched operator and command",line_number);
                
                if((next<ssize)&&buff[next]=='\n')
                    error(1, 0, "%d: syntax error 15-1 > followed by newline ", line_number);
                if(wait_input || wait_output||both_in_out)
                    error(1, 0, "%d: syntax error 15-2 > happens at wrong place", line_number);
                //command_t tmp=store_simple_command();// things needed
                //push_cmd(tmp);
                
                //initial -1, standard 0, <>:1, >& :2, >>: 3, >| 4
                if((*index+1)<ssize){
                    switch(buff[*index+1]){
                        case '&':
                            out_mode=2;
                            *index=*index+1;
                            break;
                        case '>':
                            out_mode=3;
                            *index=*index+1;
                            break;
                        case '|':
                            out_mode=4;
                            *index=*index+1;
                            if (has_option_c==false)
                                error(1, 0, "%d: option C isn't implemented before >|", line_number);
                            break;

                        case ' ':
                            out_mode=0;
                            break;
                        default: // standard : \0 or character //also need to check for invalid operator <
                            if (isWord(buff[*index+1]))
                                out_mode=0;
                            else
                                error(1, 0, "%d: invalid output operator ", line_number);
                            break;
                    }
                    
                }
                wait_output=true;
                break;
                
            case '#': // skip comments
                
                if ((*index-1>=0)&&(buff[*index-1]!=' '&&buff[*index-1]!='\t'&&buff[*index-1]!='\n'))
                {
                    error(1,0,"%d: syntax error: ordinary token right before #",line_number);
                }
                
                //
                int comments=0;
                while((*index<ssize)&&(buff[*index]=='#'))//skip all comments
                {
                    comments++;
                    *index=*index+1;
                    // reach the end of each comment line
                    while ((*index<ssize)&&(buff[*index] !='\n')) {
                        
                        *index=(*index)+1;
                    }
                    if (*index != ssize) {
                        // skip all \n following # and index points to the last \n
                        while (buff[*index+1]==' '||buff[*index+1]=='\t')
                            *index=*index+1;
                        
                        while(((*index+1)<ssize) &&((buff[*index+1]=='\n')||(buff[*index+1]==' ')||(buff[*index+1]=='\t')))
                        {
                            if((op_s.top>0|| cmd_s.top>0)&& ((buff[*index+1]=='\n'))){
                                //	printf("%d %d",op_s.top,cmd_s.top);
                                isReturn= true;
                                has_option_c=false;
                            }
                            if(buff[*index+1]=='\n')
                                line_number++;
                            
                            *index=*index+1;
                        }
                        if(((op_s.top>0)||(cmd_s.top>0))&&(comments>1))
                        {
                            isReturn = true;
                            has_option_c=false;
                        }
                        
                        if(*index<ssize-1 && ((buff[*index]=='\n')||(buff[*index]==' ')||(buff[*index]=='\t')))
                            *index=*index+1;
                        
                        
                        line_number++;
                    }
                }
                *index=*index-1;
                
                break;
            case ' ':
                break;
            case '\t':
                break;
            case '\n':
                line_number++;
                //
                /* if(((op_s.top-count_left_bracket)>=cmd_s.top) &&(cmd_s.top>0))
                 error(1, 0, "%d: syntax error 16 /n happens with dismatched operator and command", line_number);
                 */// ? \n
                if((next<ssize)&&(buff[next]!='(') && (buff[next]!=')')&&(isSpecial(buff[next])))
                    
                    error(1, 0, "%d: syntax error 17 /n is before specials other than ( )",line_number);
                
                // regard as ; a /n b
                if (cmd_s.top>0 && buff[*index-1]!='\n' && (buff[*index +1]!='\n') &&(*index-1>=0)&&(*index+1<ssize)) {
                    
                    
                    if (op_s.top ==0)
                    {
                        push_op(";");
                        if(cmd_s.top<=0)
                            error(1,0, "syntax error: 17-2 /n appear at wrong place");
                    }
                    else{
                        
                        while ((op_s.top>0)&& (precedence(op_s.operator[op_s.top-1])>=precedence(";")) &&(strcmp(op_s.operator[op_s.top-1],"(")!=0) )
                        {
                            char* tmp_op1=pop_op();
                            command_t tmp2=init_command(cmd_type(tmp_op1));
                            tmp2->u.command[1]=pop_cmd(  );
                            tmp2->u.command[0]=pop_cmd( );
                            push_cmd( tmp2);
                            
                        }
                        push_op( ";");
                        
                    }
                }
                // \n \n as a break return a tree
                while((*index+1<ssize)&&(buff[*index+1]==' '|| buff[*index+1]=='\t'))
                    *index=*index+1;
                
                if ((*index+1<ssize)&&(buff[*index+1]=='\n' || buff[*index+1]=='#')) {
                    // skip all the \n
                    while(((buff[(*index)+1]=='\n')||(buff[*index+1]=='#')||(buff[*index+1]=='\t')||(buff[*index+1]==' '))&& (*index<ssize -1))
                    {
                        if (buff[(*index)+1]=='\n')
                            line_number++;
                        else if(buff[*index+1]=='#')
                        {
                            while((*index+1<ssize)&&(buff[*index+1]!='\n'))
                            {
                                *index=*index+1;
                            }
                            line_number++;
                        }
                        *index=*index+1;
                    }
                    
                    // return a tree
                    if(op_s.top>0 || cmd_s.top>0)
                    {
                        isReturn=true;
                        has_option_c=false;
                    }
                }
                
                break;
            default: // simple command
                //check valid input
                if(isNotValid(buff[*index]))
                    error(1, 0, "%d: syntax error 19 invalid input", line_number);
                if (wait_input && wait_output)//happens when <>, both in and out are true
                    error (1, 0, "%d: syntax error 20 input output dismatch", line_number);
                if (wait_input && both_in_out)
                    error (1, 0, "%d: syntax error 20 input output dismatch", line_number);
                if (wait_output && both_in_out)
                    error (1, 0, "%d: syntax error 20 input output dismatch", line_number);
                if (both_in_out)//for <>, store both input and output
                {
                    char* text= get_next_word(buff, index, ssize);
                    if (text==NULL|| strlen(text)==0) {
                        error(1, 0, "%d: syntax error 21-1 invalid <", line_number);
                    }
                    int len = sizeof(text)/sizeof(char);
                    cmd_s.command[cmd_s.top-1]->input = (char*) checked_malloc(sizeof(char)*len);
                    cmd_s.command[cmd_s.top-1]->output = (char*) checked_malloc(sizeof(char)*len);
                    strcpy(cmd_s.command[cmd_s.top-1]->input, text); // undefined text //how do you save as both input and output but save only a copy in command stack?
                    strcpy(cmd_s.command[cmd_s.top-1]->output, text); // undefined text
                    *index=*index-1;//to get back one step since there's a ++ at the end
                    cmd_s.command[cmd_s.top-1]->input_mode=in_mode;
                    cmd_s.command[cmd_s.top-1]->output_mode=out_mode;
                    in_mode=-1;
                    out_mode=-1;
                    both_in_out=false;
                    break;
                }
                if(wait_input)
                {
                    //*index=(*index)+1;
                    char* text= get_next_word(buff, index, ssize);
                    if (text==NULL|| strlen(text)==0) {
                        error(1, 0, "%d: syntax error 21-1 invalid <", line_number);
                    }
                    int len = sizeof(text)/sizeof(char);
                    cmd_s.command[cmd_s.top-1]->input = (char*) checked_malloc(sizeof(char)*len);
                    strcpy(cmd_s.command[cmd_s.top-1]->input, text); // undefined text
                    *index=*index-1;//to get back one step since there's a ++ at the end
                    cmd_s.command[cmd_s.top-1]->input_mode=in_mode;
                    in_mode=-1;
                    wait_input=false;
                    break;
                }
                if(wait_output)
                {
                    //*index=(*index)+1;
                    char* text= get_next_word(buff, index, ssize);
                    if (text==NULL || strlen(text)==0) {
                        error(1, 0, "%d: syntax error 21-2 invalid >", line_number);
                    }
                    int len = sizeof(text)/sizeof(char);
                    cmd_s.command[cmd_s.top-1]->output = (char*) checked_malloc(sizeof(char)*len);
                    strcpy(cmd_s.command[cmd_s.top-1]->output, text); // undefined text
                    *index=*index-1;//
                    cmd_s.command[cmd_s.top-1]->output_mode=out_mode;
                    out_mode=-1;
                    wait_output =false;
                    
                    break;
                }
                // simple command
                //*index=(*index)+1;
                command_t tmp1=store_simple_command(buff, index, ssize);// things needed
                push_cmd(tmp1);
                break;
        }
        (*index)=(*index)+1;
    }
    // return process
    if (wait_input || wait_output||both_in_out) {
        error(1, 0, "%d: syntax error 21-3 invalid > <", line_number);
    }
    // pop all operators and commands
    /* if ((op_s.top>=cmd_s.top) &&(cmd_s.top>0))
     {
     
     error(1, 0, "%d: syntax error 21 dismatched operator and command", line_number);
     }*/
    
    while (op_s.top>0) {
        // only ; left
        if(op_s.top==1 && (strcmp(op_s.operator[0],";")==0)&&(cmd_s.top==1))
            return cmd_s.command[0];
        
        char* tmp_op=pop_op();
        command_t tmp= (command_t) malloc(sizeof(struct command));
        tmp=init_command(cmd_type(tmp_op));
        tmp->u.command[1]=pop_cmd( );
        tmp->u.command[0]=pop_cmd();
        push_cmd( tmp);
    }
    if (op_s.top==0 && (cmd_s.top==0)) {
        return NULL;
    }
    if ((op_s.top!=0)||(cmd_s.top!=1)) //
        error(1, 0, "%d: %d %d: syntax error 22 dismatched operator and command", line_number,op_s.top,cmd_s.top);
    
    return cmd_s.command[0];
    
}
command_stream_t
make_command_stream (int (*get_next_byte) (void *),
                     void *get_next_byte_argument)
{
    size_t buf_size=2048;
    size_t read_size=0;
    int c;
    char *buf=checked_malloc(buf_size);
    
    while ((c=get_next_byte(get_next_byte_argument))!=EOF)
    {
        buf[read_size++]=c;
        if(read_size==buf_size)
        {
            buf_size=buf_size*2;
            buf=(char*)checked_grow_alloc(buf,&buf_size );
        }
        
    }
    
    command_stream_t stream=init_stream();
    stream->head= (struct command_node*) checked_malloc(sizeof(struct command_node));
    command_t tmp=NULL;
    int index=0 ;
    stream->head->c=build_command_t(buf,&index,read_size );
    stream->head->next=NULL;
    
    
    stream->cursor= stream->head;
    stream->tail= stream->head;
    tmp=build_command_t(buf, &index, read_size);
    while(tmp!=NULL){
        
        stream->cursor->next =(struct command_node*) checked_malloc(sizeof(struct command_node));
        
        stream->cursor->next->c=tmp;
        stream->cursor->next->next=NULL;
        stream->cursor=stream->cursor->next;
        
        tmp=build_command_t(buf, &index, read_size);
    }
    stream->tail=stream->cursor;
    stream->cursor=stream->head;
    
    
    return stream;
    
    /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
    
    
    //error (1, 0, "command reading not yet implemented");
    //return 0;
}

command_t
read_command_stream (command_stream_t s)
{
    if (s->cursor==NULL)
    {
        return NULL;
    }
    command_t tmp=s->cursor->c;
    s->cursor=s->cursor->next;
    return tmp;
    //return NULL;
}





/****************time travel************/

//dependency_graph create_graph(command_stream_t c_stream){}
//void process_command(command_t c){}

void process_command (list_node_t node,command_t cmd, int *index_r, int * index_w);
bool haveDependency(list_node_t A,list_node_t B);
bool intersect(char ** a, char** b);

//need fix: each tree has separate read/write list
char ** read_list[10]={0};
char ** write_list[10]={0};
int read_list_index;
int write_list_index;

int command_t_no=1;// set the number of command tree
// read_list : a char ** for each tree
bool haveDependency(list_node_t A,list_node_t B)
{
    //if B is after A
    //intersect RAW
    //!( A->wl && B->rl ==empty)
    if (intersect(A->write_list, B->read_list)) {
        return true;
    }
    // WAW
    //!( A->wl && B->wl ==empty)
    if (intersect(A->write_list, B->write_list)) {
        return true;
    }
    // WAR
    //!( A->rl && B->wl ==empty)
    if (intersect(A->read_list, B->write_list)) {
        return true;
    }
    return false;
    
}
bool intersect(char ** a, char** b)
{
    if (a==NULL || b== NULL) {
        return false;
    }
    else{
        for (int i=0; a[i]!=NULL; i++) {
            for (int j=0; b[j]!=NULL; j++) {
                if (strcmp(a[i], b[j])==0) {
                    return true;
                }
            }
        }
        return false;
    }
}
//inital list stream

list_stream_t init_list_stream()
{
    list_stream_t stream=(list_stream_t) checked_malloc(sizeof(struct list_stream));
    stream->head=NULL;
    stream->tail=NULL;
    stream->cursor=NULL;
    return stream;
}
// initial graphnode
graph_node_t init_graph_node(command_t cmd){
    graph_node_t g=( graph_node_t) checked_malloc(sizeof( graph_node_t));
    
    g->command=cmd;
    g->pid=-1;
    g->size_before_list=0;
    g->before =(graph_node_t*)checked_malloc(sizeof(graph_node_t)* MAX_BEFORE);
    return g;
}
// initial list node
list_node_t init_list_node(graph_node_t node)
{
    list_node_t l= ( list_node_t) checked_malloc(sizeof(struct list_node));
    l->graph_node=node;
    l->next=NULL;
    l->read_list=(char**)checked_malloc(sizeof(char*)*100);//
    l->write_list=(char**)checked_malloc(sizeof(char*)*100);//
    return l;
    
}
queue_t init_queue()
{
    queue_t Q=(queue_t )checked_malloc(sizeof(queue_t));
    Q->head=NULL;
    Q->cursor=NULL;
    Q->tail=NULL;
    return Q;
}
queue_node_t init_queue_node()
{
    queue_node_t q=(queue_node_t) checked_malloc(sizeof(queue_node_t));
    q->g=NULL;
    q->next=NULL;
    return q;
}
dependency_t init_dependency_graph()
{
    dependency_t d = (dependency_t) checked_malloc(sizeof(struct dependency_graph));
    d->no_dependency=init_queue();
    d->dependency=init_queue();
    return d;
}


// build the dependency graph
/*for each command tree k in command stream
 do{
 //determine if there is dependency between command trees before it and itself.
 
 processCommand(k->command);//given root node, generate the read&write list
 for i=1:k-1
 if(haveDependency(i,k))
 update k's before list;
 //check the before list
 if before==NULL
 add k to no_dependencies;
 else
 add k to dependencies;
 }*/
//

dependency_t create_graph(command_stream_t s)
{
    command_t command;
    list_stream_t stream= init_list_stream();
    
    // initial dependency graph
    dependency_t d = init_dependency_graph();
    
    while ((command = read_command_stream (s)))//for each command tree
    {
        int index_r=0;
        int index_w=0;
        
        //
        //create a new GraphNode to store command tree k;
        graph_node_t node=init_graph_node(command);
        
        //create a ListNode with graphnode, wl, rl;
        
        if (command_t_no==1) {
            stream->head=init_list_node( node);
            stream->cursor=stream->head;
        }
        else {
            stream->cursor->next=init_list_node(node);
            stream->cursor=stream->cursor->next;
        }
        
        //1. generate RL, WL for command tree
        process_command( stream->cursor, stream->cursor->graph_node->command, &index_r, &index_w);//save the rl and wl inside
        
        stream->cursor->read_list[index_r]=NULL;
        stream->cursor->write_list[index_w]=NULL;
        
        //2. check if there is dependency between this current command tree and all command trees before it. update the before list
        if (command_t_no>1) {
            int i=1;
            list_node_t tmp=stream->head;
            
            while (i<command_t_no) {
                if (haveDependency(tmp, stream->cursor)) {
                    // update cursor's before list
                    int ssize=stream->cursor->graph_node->size_before_list;
                    stream->cursor->graph_node->before[ ssize ]=tmp->graph_node;
                    stream->cursor->graph_node->size_before_list++;
                }
                tmp=tmp->next;
                i++;
            }
        }
        //3.check if before list is NULL, then update the dependency graph
        if (stream->cursor->graph_node->size_before_list==0) {
            
            if (d->no_dependency->head==NULL) {
                d->no_dependency->head = init_queue_node();
                d->no_dependency->head->g=stream->cursor->graph_node;
                d->no_dependency->cursor=d->no_dependency->head;
            }
            else
            {
                d->no_dependency->cursor->next = init_queue_node();
                d->no_dependency->cursor->next->g= stream->cursor->graph_node;
                d->no_dependency->cursor =d->no_dependency->cursor->next;
            }
            
        }
        else{
            
            if (d->dependency->head==NULL) {
                d->dependency->head = init_queue_node();
                d->dependency->head->g=stream->cursor->graph_node;
                d->dependency->cursor=d->dependency->head;
            }
            else
            {
                d->dependency->cursor->next = init_queue_node();
                d->dependency->cursor->next->g= stream->cursor->graph_node;
                d->dependency->cursor =d->dependency->cursor->next;
            }
        }
        command_t_no++;
        
        
    }
    // set tail and cursor
    d->no_dependency->tail=d->no_dependency->cursor;
    d->no_dependency->cursor=d->no_dependency->head;
    d->dependency->tail=d->dependency->cursor;
    d->dependency->cursor=d->dependency->head;
    stream->tail=stream->cursor;
    stream->cursor=stream->head;
    return d;//
}
//build RL WL for each tree
void process_command(list_node_t node,command_t cmd, int *index_r, int * index_w)
{
    if (cmd->type==SIMPLE_COMMAND)
    {
        if (cmd->input!=NULL) {
            node->read_list[*index_r]=(char*) checked_malloc(sizeof(char)*100);
            strcpy(node->read_list[*index_r], cmd->input);
            *index_r = *index_r+1;
        }
        
        //store word[1]...word[n]
        //ignore options (starting with dash - )
        
        int i=1; //store from word[1]
        while (cmd->u.word[i]!=NULL)
        {
            if (cmd->u.word[i][0]!='-') {
                node->read_list[*index_r]=(char*)checked_malloc(sizeof(char)*100);
                strcpy(node->read_list[*index_r], cmd->u.word[i]);
                *index_r =*index_r+1;
            }
            i++;
        }
        // write list
        if (cmd->output!=NULL) {
            node->write_list[*index_w]=(char*) checked_malloc(sizeof(char)*100);
            strcpy(node->write_list[*index_w], cmd->output);
            *index_w=*index_w+1;
        }
    }
    
    else if (cmd->type==SUBSHELL_COMMAND)
    {
        
        if (cmd->input!=NULL) {
            node->read_list[*index_r]=(char*) checked_malloc(sizeof(char)*100);
            strcpy(node->read_list[*index_r], cmd->input);
            *index_r = *index_r+1;
        }
        
        // write list
        if (cmd->output!=NULL) {
            node->write_list[*index_w]=(char*) checked_malloc(sizeof(char)*100);
            strcpy(node->write_list[*index_w], cmd->output);
            *index_w =*index_w+1;
        }
        
        process_command(node,cmd->u.subshell_command, index_r, index_w);
    }
    else
    {
        process_command(node,cmd->u.command[0], index_r, index_w);
        process_command(node,cmd->u.command[1], index_r, index_w);
        
    }
}
