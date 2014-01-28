// UCLA CS 111 Lab 1 command internals
#include <sys/types.h>
enum command_type
  {
    AND_COMMAND,         // A && B
    SEQUENCE_COMMAND,    // A ; B
    OR_COMMAND,          // A || B
    PIPE_COMMAND,        // A | B
    SIMPLE_COMMAND,      // a simple command
    SUBSHELL_COMMAND,    // ( A )
  };

//nodes in the dependency graph
struct command_node
{
  char **read_dependencies;
  char **write_dependencies;
  command_t cmd;
  command_node_t *prior_dependencies;
  command_node_t *future_dependents;
  pid_t pid;
  int read_dependencies_position;
  int read_dependencies_size;
  int write_dependencies_position;
  int write_dependencies_size;
  command_node_t next;
  int prior_dep_size;
  int prior_dep_position;
  int future_dep_size;
  int future_dep_position;
};

//dependency graph
struct command_graph
{
  command_node_t no_dependency;
  command_node_t dependency;
};

// Data associated with a command.
struct command
{
  enum command_type type;

  // Exit status, or -1 if not known (e.g., because it has not exited yet).
  int status;

  // I/O redirections, or null if none.
  char *input;
  char *output;

  union
  {
    // for AND_COMMAND, SEQUENCE_COMMAND, OR_COMMAND, PIPE_COMMAND:
    struct command *command[2];

    // for SIMPLE_COMMAND:
    char **word;

    // for SUBSHELL_COMMAND:
    struct command *subshell_command;
  } u;
};
