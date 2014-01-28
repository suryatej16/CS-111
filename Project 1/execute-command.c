// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int
command_status (command_t c) //return status of the command
{
  return c->status;
}

void pipe_command(command_t c, bool time_travel) //executing pipe command
{
  int status;
  int fd[2]; //descriptors for pipe
  pid_t pid1;
  pid_t pid2;
  pid_t finished_process;
  if(pipe(fd) == -1)
    {
      fprintf(stderr, "Could not create a pipe.");
      exit(1);
    }
  pid1 = fork();
  if(pid1 < 0)
    {
      fprintf(stderr, "Fork failed.");
      exit(1);
    }
  if(pid1 > 0) //still in the parent shell process
    {
      pid2 = fork();
      if(pid2 < 0)
	{
	  fprintf(stderr, "Fork failed.");
	  exit(1);
	}
      if(pid2 > 0)
	{
	  close(fd[0]); //remove all of the parent's ties to the pipe
	  close(fd[1]);
	  finished_process = waitpid(-1, &status, 0); //wait for any process
	  if(finished_process == pid1)
	    {
	      c->status = status;
	      waitpid(pid2, &status, 0);
	      return;
	    }
	  if(finished_process == pid2)
	    {
	      waitpid(pid1, &status, 0);
	      c->status = status;
	      return;
	    }
	}
      if(pid2 == 0) //the first part of the pipe
	{
	  close(fd[0]); //close the read_end
	  if(dup2(fd[1], 1) == -1)
	    {
	      fprintf(stderr, "Could not perform dup2.");
	      exit(1);
	    }
	  execute_command(c->u.command[0], time_travel);
	  _exit(c->u.command[0]->status);
	}
    }
  if(pid1 == 0) //second part of the pipe
    {
      close(fd[1]); //close the write end
      if(dup2(fd[0], 0) == -1)
	{
	  fprintf(stderr, "Could not perform dup2.");
	  exit(1);
	}
      execute_command(c->u.command[1], time_travel);
      _exit(c->u.command[1]->status);
      }
}

void sequence_command(command_t c, bool time_travel)
{
  int status;
  pid_t pid1;
  pid_t pid2;
  pid1 = fork();
  if(pid1 < 0)
    {
      fprintf(stderr, "Fork failed.");
      exit(1);
    }
  if(pid1 > 0)
    {
      waitpid(pid1, &status, 0);
    }
  if(pid1 == 0)
    {
      pid2 = fork();
      if(pid2 < 0)
	{
	  fprintf(stderr, "Fork failed.");
	  exit(1);
	}
      if(pid2 > 0) //second part of the sequence command
	{
	  waitpid(pid2, &status, 0); 
	  execute_command(c->u.command[1], time_travel);
	  _exit(c->u.command[1]->status);
	}
      if(pid2 == 0)
	{
	  execute_command(c->u.command[0], time_travel); //first part of sequence command
	  _exit(c->u.command[0]->status);
	}
    }
}

void simple_command(command_t c, char **input)
{
  int status;
  pid_t pid = fork();
  if(pid < 0)
    {
      fprintf(stderr, "Fork failed.");
      exit(1);
    }
  if(pid > 0)
    {
      waitpid(pid, &status, 0); 
      c->status = status;
    }
  if(pid == 0)
    {
      if(c->input != NULL) //setup input and output for redirection
	{
	  int file_ptr1 = open(c->input, O_RDWR);
	  if(file_ptr1 < 0)
	    {
	      fprintf(stderr, "Error opening file: %s.", c->input);
	      _exit(1);
	    }
	  int exit_status1 = dup2(file_ptr1, 0);
	  if(exit_status1 < 0)
	    {
	      fprintf(stderr, "Dup2 error.");
	      _exit(1);//check for errors here
	    }
	  int exit_status2 = close(file_ptr1);
	  if(exit_status2 < 0)
	    {
	      fprintf(stderr, "Problem closing file.");
	      _exit(1);
	    }
	}
      if(c->output != NULL)
	{
	  int file_ptr2 = open(c->output, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
	  if(file_ptr2 < 0)
	    {
	      fprintf(stderr, "Error writing to file.");
	      _exit(1);
	    }
	  int exit_status3 = dup2(file_ptr2, 1);
	  if(exit_status3 < 0)
	    {
	      fprintf(stderr, "Dup2 error."); //check for errors here
	      _exit(1);
	    }
	  int exit_status4 = close(file_ptr2);
	  if( exit_status4 < 0)
	    {
	      fprintf(stderr, "Problem closing file.");
	      _exit(1);
	    }
	}
      execvp(input[0], input);
      fprintf(stderr, "Invalid command.");
      _exit(1);
    }
}


//Helps to parse string into args[] for execvp
//Parses on spaces and null token
//Returns char** args

char** 
parse(char* string)
{
  if(strlen(string) == 1) {}
    //do something for one character?
  char* start = string;
  char* end = string;
  char* nullend = &string[strlen(string)];
  char** args = (char**) malloc(5*sizeof(char*));
  int size_args = 5;
  int i = 0;
  while(end != nullend) {
    while(*end != ' ' &&  end != nullend) {
      end++;
    }
    int length = 1+(end - start);
    args[i] = malloc(length*sizeof(char));
    memcpy(args[i], start, length);
    args[i][length-1] = '\0';
    if(end == nullend)
      break;
    end++;
    start = end;
    i++;
    if(i == size_args) {
      size_args *= 2;
      args = realloc(args, size_args*sizeof(char*)); 
    }
  }
  if(i == size_args-1)
    args = realloc(args, (size_args+1)*sizeof(char*));
  return args;
}


void
subshell_execute_command (command_t c, bool time_travel, char* output)
{
  /*  error (1, 0, "command execution not yet implemented");*/
  if(c->type == SIMPLE_COMMAND) {
    char** args = parse(*(c->u.word));
    int actual_size = 0;
    char** ptr = args;
    while(*ptr != NULL) {
      actual_size++;
      ptr++;
    }
    simple_command(c, args);
  } else 
  if(c->type == AND_COMMAND) {
    execute_command(c->u.command[0], time_travel);
    if(c->u.command[0]->status != 0){
      c->status = c->u.command[0]->status;
      return;
    }
    execute_command(c->u.command[1], time_travel);
    c->status = c->u.command[1]->status;
  } else if(c->type == OR_COMMAND) {
    subshell_execute_command(c->u.command[0], time_travel, output);
    if(c->u.command[0]->status != 0){
      subshell_execute_command(c->u.command[1], time_travel, output);
      c->status = c->u.command[1]->status;
    }
    c->status = c->u.command[0]->status;
  } else
  if(c->type == PIPE_COMMAND) {
    pipe_command(c, time_travel);
  } else 
  if(c->type == SEQUENCE_COMMAND) {
    sequence_command(c, time_travel);
  } else
  if(c->type == SUBSHELL_COMMAND) {
    subshell_execute_command(c->u.subshell_command, time_travel, output);
     c->status = c->u.subshell_command->status;
  }
}

void
execute_command (command_t c, bool time_travel)
{
  /*  error (1, 0, "command execution not yet implemented");*/
  if(c->type == SIMPLE_COMMAND) {
    char** args = parse(*(c->u.word));
    int actual_size = 0;
    char** ptr = args;
    while(*ptr != NULL) {
      actual_size++;
      ptr++;
    }
    simple_command(c, args);
  } else 
  if(c->type == AND_COMMAND) {
    execute_command(c->u.command[0], time_travel);
    if(c->u.command[0]->status != 0)
      {
	c->status = c->u.command[0]->status;
	return;
      }
    execute_command(c->u.command[1], time_travel);
    c->status = c->u.command[1]->status;
  } else if(c->type == OR_COMMAND) {
    execute_command(c->u.command[0], time_travel);
    if(c->u.command[0]->status != 0) {
     execute_command(c->u.command[1], time_travel);
      c->status = c->u.command[1]->status;
    }
    c->status = c->u.command[0]->status;
  }else
  if(c->type == PIPE_COMMAND) {
    pipe_command(c, time_travel);
  } else 
  if(c->type == SEQUENCE_COMMAND) {
    sequence_command(c, time_travel);
  } else 
  if(c->type == SUBSHELL_COMMAND) {
    subshell_execute_command(c->u.subshell_command, time_travel, c->output);
    c->status = c->u.subshell_command->status;
  }
}

int
create_dependencies(command_node_t no_dependency_head, command_node_t dependency_head, command_node_t temp)
{
  int marked_dependent;
  marked_dependent = 0;
  command_node_t ptr;
  ptr = no_dependency_head;
  //checking our one command through the no_dependency list
  while(ptr != NULL)
    {
      int is_dependent; //flag to see if it is dependent
      is_dependent = 0;
      int i;
      for(i = 0; i < ptr->read_dependencies_position; i++) //first we go through all of the independent command's read dependencies
	{
	  int j;
	  for(j = 0; j < temp->write_dependencies_position; j++) //we then cross-reference with temp's write dependencies
	    {
	      if(*(temp->write_dependencies[j]) == *(ptr->read_dependencies[i])) //if any of our write dependencies match its read dependency, we're done
		{
		  is_dependent = 1; 
		  break;
		}
	    }
	  if(is_dependent == 1) //do not look for any more read/write dependencies
	    {
	      break;
	    }
	}
      if(is_dependent == 1) //if there is a read/write dependency
	{
	  marked_dependent = 1;
	  if(ptr->future_dep_position == ptr->future_dep_size)
	    {
	      ptr->future_dep_size += 5;
	      ptr->future_dependents = realloc(ptr->future_dependents, ptr->future_dep_size*(sizeof(command_node_t)));
	    }
	  ptr->future_dependents[ptr->future_dep_position] = temp; //add temp to future dependency
	  ptr->future_dep_position++;
	  if(temp->prior_dep_position == temp->prior_dep_size)
	    {
	      temp->prior_dep_size += 5;
	      temp->prior_dependencies = realloc(temp->prior_dependencies, temp->prior_dep_size*(sizeof(command_node_t)));
	    }
	  temp->prior_dependencies[temp->prior_dep_position] = ptr; //add the independent node to temp's prior dependency
	  temp->prior_dep_position++;
	  is_dependent = 0; 
	  ptr = ptr->next; //move on to the next independent command
	  continue;
	}

      for(i = 0; i < ptr->write_dependencies_position; i++) //checking for write/write or write/read dependencies
	{
	  int k;
	  for(k = 0; k < temp->read_dependencies_position; k++)
	    {
	      if(*(temp->read_dependencies[k]) == *(ptr->write_dependencies[i])) //if there is a write/read dependency
		{
		  is_dependent = 1;
		  break;
		}
	    }
	  if(is_dependent == 1) //no need to cheak for write/write dependency
	    {
	      break;
	    }
	  for(k = 0; k < temp->write_dependencies_position; k++) 
	    {
	      if(*(temp->write_dependencies[k]) == *(ptr->write_dependencies[i])) //if there is a write/write dependency
		{
		  is_dependent = 1;
		  break;
		}
	    }
	  if(is_dependent == 1)
	    {
	      break;
	    }
	}
      if(is_dependent == 1)
	{
	  marked_dependent = 1;
	  if(ptr->future_dep_position == ptr->future_dep_size)
	    {
	      ptr->future_dep_size += 5;
	      ptr->future_dependents = realloc(ptr->future_dependents, ptr->future_dep_size*(sizeof(command_node_t)));
	    }
	  ptr->future_dependents[ptr->future_dep_position] = temp; //add temp to future dependency
	  ptr->future_dep_position++;
	  if(temp->prior_dep_position == temp->prior_dep_size)
	    {
	      temp->prior_dep_size += 5;
	      temp->prior_dependencies = realloc(temp->prior_dependencies, temp->prior_dep_size*(sizeof(command_node_t)));
	    }
	  temp->prior_dependencies[temp->prior_dep_position] = ptr; //add the independent node to temp's prior dependency
	  temp->prior_dep_position++;
	  is_dependent = 0; 
	  ptr = ptr->next; //move on to the next independent command
	  continue;
	}
      ptr = ptr->next;
    }

  ptr = dependency_head; //now go through the dependent list and check for dependencies
  while(ptr != NULL)
    {
      int is_dependent; //flag to see if it is dependent
      is_dependent = 0;
      int i;
      for(i = 0; i < ptr->read_dependencies_position; i++) //first we go through all of the independent command's read dependencies
	{
	  int j;
	  for(j = 0; j < temp->write_dependencies_position; j++) //we then cross-reference with temp's write dependencies
	    {
	      if(*(temp->write_dependencies[j]) == *(ptr->read_dependencies[i])) //if any of our write dependencies match its read dependency, we're done
		{
		  is_dependent = 1; 
		  break;
		}
	    }
	  if(is_dependent == 1) //do not look for any more read/write dependencies
	    {
	      break;
	    }
	}
      if(is_dependent == 1) //if there is a read/write dependency
	{
	  marked_dependent = 1;
	  if(ptr->future_dep_position == ptr->future_dep_size)
	    {
	      ptr->future_dep_size += 5;
	      ptr->future_dependents = realloc(ptr->future_dependents, ptr->future_dep_size*(sizeof(command_node_t)));
	    }
	  ptr->future_dependents[ptr->future_dep_position] = temp; //add temp to future dependency
	  ptr->future_dep_position++;
	  if(temp->prior_dep_position == temp->prior_dep_size)
	    {
	      temp->prior_dep_size += 5;
	      temp->prior_dependencies = realloc(temp->prior_dependencies, temp->prior_dep_size*(sizeof(command_node_t)));
	    }
	  temp->prior_dependencies[temp->prior_dep_position] = ptr; //add the independent node to temp's prior dependency
	  temp->prior_dep_position++;
	  is_dependent = 0; 
	  ptr = ptr->next; //move on to the next independent command
	  continue;
	}
      
      for(i = 0; i < ptr->write_dependencies_position; i++) //checking for write/write or write/read dependencies
	{
	  int k;
	  for(k = 0; k < temp->read_dependencies_position; k++)
	    {
	      if(*(temp->read_dependencies[k]) == *(ptr->write_dependencies[i])) //if there is a write/read dependency
		{
		  is_dependent = 1;
		  break;
		}
	    }
	  if(is_dependent == 1) //no need to cheak for write/write dependency
	    {
	      break;
	    }
	  for(k = 0; k < temp->write_dependencies_position; k++) 
	    {
	      if(*(temp->write_dependencies[k]) == *(ptr->write_dependencies[i])) //if there is a write/write dependency
		{
		  is_dependent = 1;
		  break;
		}
	    }
	  if(is_dependent == 1)
	    {
	      break;
	    }
	}
      if(is_dependent == 1)
	{
	  marked_dependent = 1;
	  if(ptr->future_dep_position == ptr->future_dep_size)
	    {
	      ptr->future_dep_size += 5;
	      ptr->future_dependents = realloc(ptr->future_dependents, ptr->future_dep_size*(sizeof(command_node_t)));
	    }
	  ptr->future_dependents[ptr->future_dep_position] = temp; //add temp to future dependency
	  ptr->future_dep_position++;
	  if(temp->prior_dep_position == temp->prior_dep_size)
	    {
	      temp->prior_dep_size += 5;
	      temp->prior_dependencies = realloc(temp->prior_dependencies, temp->prior_dep_size*(sizeof(command_node_t)));
	    }
	  temp->prior_dependencies[temp->prior_dep_position] = ptr; //add the independent node to temp's prior dependency
	  temp->prior_dep_position++;
	  is_dependent = 0; 
	  ptr = ptr->next; //move on to the next independent command
	  continue;
	}
      ptr = ptr->next;
    }
  if(marked_dependent == 1) //add command node to either no dependency or dependency list
    {
      ptr = dependency_head;
      if(dependency_head == NULL) //used to be dependency_head = temp. need to figure out why that is wrong
	{
	  return 1;
	}
      else
	{
	  while(ptr->next != NULL)
	    {
	      ptr = ptr->next;
	    }
	  ptr->next = temp;
	  return 2;
	}
    }
  else
    {
      ptr = no_dependency_head;
      while(ptr->next != NULL) //stop at the last node
	{
	  ptr = ptr->next;
	}
      ptr->next = temp;
      return 0;
    }
  return 0;
}


//add all read/write dependencies of an individual command_node
void  
add_dependency_words(command_node_t x, command_t y)
{
  int i;
  char **temp;
  switch(y->type)
    {
    case AND_COMMAND:
    case OR_COMMAND:
    case SEQUENCE_COMMAND:
    case PIPE_COMMAND:
      add_dependency_words(x, y->u.command[0]);
      add_dependency_words(x, y->u.command[1]);
      break;
    case SUBSHELL_COMMAND:
      add_dependency_words(x, y->u.subshell_command);
      break;
    case SIMPLE_COMMAND:
      temp = parse(*(y->u.word));
      for(i = 1; temp[i] != '\0'; i++) //for simple command, add all files to read dependency
	{
	  if(x->read_dependencies_position == x->read_dependencies_size)
	    {
	      x->read_dependencies_size += 5;
	      x->read_dependencies = realloc(x->read_dependencies, x->read_dependencies_size*(sizeof(char*)));
	    }
	  x->read_dependencies[x->read_dependencies_position] = temp[i]; //add the dependencies to the correct position
	  x->read_dependencies_position++;
	}
      if(y->input != NULL) //also add input to read dependency
	{
	  if(x->read_dependencies_position == x->read_dependencies_size)
	    {
	      x->read_dependencies_size += 5;
	      x->read_dependencies = realloc(x->read_dependencies, x->read_dependencies_size*(sizeof(char*)));
	    } 
	  x->read_dependencies[x->read_dependencies_position] = y->input;
	  x->read_dependencies_position++;
	}
      if(y->output != NULL) //if there is redirection output, add to write dependency
	{
	  if(x->write_dependencies_position == x->write_dependencies_size)
	    {
	      x->write_dependencies_size += 5;
	      x->write_dependencies = realloc(x->write_dependencies, x->write_dependencies_size*(sizeof(char*)));
	    } 
	  x->write_dependencies[x->write_dependencies_position] = y->output; //add the dependencies to the correct position
	  x->write_dependencies_position++;
	}
      break;
    }
}


//executing a command stream in parallel
void
execute_time_travel(command_stream_t command_stream, command_t last_command)
{
  command_t command;
  command_node_t dependency_head;
  command_node_t no_dependency_head;
  int command_number;
  dependency_head = NULL;
  no_dependency_head = NULL;
  command_number = 0;
  //read command stream one command at a time
  while((command = read_command_stream(command_stream)))
    {
      if(command_number == 0) //first command is automatically sent to no dependency list
	{
	  no_dependency_head = malloc(sizeof(struct command_node));
       	  no_dependency_head->read_dependencies = malloc(5*sizeof(char*));
	  no_dependency_head->write_dependencies = malloc(5*sizeof(char*));
	  no_dependency_head->cmd = command;
	  no_dependency_head->prior_dependencies = malloc(5*sizeof(command_node_t));
	  no_dependency_head->future_dependents = malloc(5*sizeof(command_node_t));
	  no_dependency_head->pid = -1; //set pid to a negative number to clarify in execution
	  no_dependency_head->read_dependencies_position = 0;
	  no_dependency_head->read_dependencies_size = 5;
	  no_dependency_head->write_dependencies_position = 0;
	  no_dependency_head->write_dependencies_size = 5;
	  no_dependency_head->next = NULL;
	  no_dependency_head->prior_dep_size = 5;
	  no_dependency_head->prior_dep_position = 0;
	  no_dependency_head->future_dep_size = 5;
	  no_dependency_head->future_dep_position = 0;
	  add_dependency_words(no_dependency_head, command);
	  command_number++;
	  continue;
	}
      //for all the commands afterwards, initialize, add words to dependency list and construct dependency graph
      command_node_t temp = malloc(sizeof(struct command_node));
      temp->read_dependencies = malloc(5*sizeof(char*));
      temp->write_dependencies = malloc(5*sizeof(char*));
      temp->cmd = command;
      temp->prior_dependencies = malloc(5*sizeof(command_node_t));
      temp->future_dependents = malloc(5*sizeof(command_node_t));
      temp->pid = -1;
      temp->read_dependencies_position = 0;
      temp->read_dependencies_size = 5;
      temp->write_dependencies_position = 0;
      temp->write_dependencies_size = 5;
      temp->next = NULL;
      temp->prior_dep_size = 5;
      temp->prior_dep_position = 0;
      temp->future_dep_size = 5;
      temp->future_dep_position = 0;
      add_dependency_words(temp, command);
      int dep;
      dep = create_dependencies(no_dependency_head, dependency_head, temp);
      if(dep == 1) //must be a better way to do this, if this is the first command in the dependency graph, just set it to temp
	{
	  dependency_head = temp;
	}
      temp = NULL;
     }
    //Now we have two lists, one for independent and dependent commands
    //Start searching through the independent commands
    while(no_dependency_head != NULL) {
      pid_t pid1; 
      pid1 = fork();
      if(pid1 < 0) {
        fprintf(stderr, "Fork failure3");
        _exit(1);
    //If this is the child process, then start executing the command
      } else if(pid1 == 0) {
        execute_command(no_dependency_head->cmd, 0);
        _exit(1);
    //For parent class, set pid = pid, so we know the command has started, and move onto the next command in the list
      } else {
        no_dependency_head->pid = pid1;
        last_command = no_dependency_head->cmd; 
        no_dependency_head = no_dependency_head->next;  
      }
    }
//Find the end of the dependent node list incase we need to move a node to the back
    command_node_t end_ptr = dependency_head;
    if(end_ptr != NULL) {
      while(end_ptr->next != NULL) 
        end_ptr = end_ptr->next;
    }
//For the dependent list, check if the previous commands it depends on have started
//Else, push it to the back of the list
    int pos = 0;
    while(dependency_head != NULL) {
      int pushed_back = 0;
      for(; pos < dependency_head->prior_dep_position; pos++) {
        if (dependency_head->prior_dependencies[pos]->pid == -1){
          command_node_t copy = dependency_head;
          end_ptr = copy;
          dependency_head = dependency_head->next;
          pushed_back = 1;
          break;
        }
      }
      if(pushed_back)
        continue;   
      pos = 0; 
      int status;
      pid_t pid2;
      pid2 = fork();
      if(pid2 < 0) {
        fprintf(stderr, "Fork failure");
        _exit(1);
//For the child, make sure that the previous commands have finished executing, THEN execute the current
//command
      } else if(pid2 == 0) {
        for(; pos < dependency_head->prior_dep_position; pos++) {
          waitpid(dependency_head->prior_dependencies[pos]->pid, status, 0);
        }
        execute_command(dependency_head->cmd, 0);
        _exit(1);
//Set the dependent node pid = pid so we know it has started/done, move onto next command in list
      } else {
        dependency_head->pid = pid2;
        last_command = dependency_head->cmd;
        dependency_head = dependency_head->next;
      }
    } 
      
}
