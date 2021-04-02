
#if !defined(__CLI_H__)

#define __CLI_H__

#include <stdio.h>

#include "io.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct CLI;
struct Mutex;

typedef struct CliCommand {
    const char *cmd;
    void (*handler)(struct CLI *cli, struct CliCommand *cmd);
    const char *help;

    // Subcommands
    struct CliCommand *subcommand; // TODO
    void *ctx;

    // linked list when registered to a cli
    struct CliCommand *next;
}   CliCommand;

#define CLI_MAX_ARGS 8

typedef struct CLI {
    char *buff;
    size_t size;
    int cursor;
    char *strtok_save;
    CliCommand *head;
    FILE *output;
    const char* prompt;
    const char* eol;
    struct Mutex *mutex; // can be null
    void *ctx; // context

    // used by cli_execute to break input into parts
    const char *args[CLI_MAX_ARGS];
    int nest;
}   CLI;

void cli_init(CLI *cli, size_t size, void *ctx);
void cli_close(CLI *cli);

void cli_register(CLI *cli, CliCommand *cmd);
void cli_process(CLI *cli, char c);

void cli_print(CLI *cli, const char *fmt, ...) __attribute__((format(printf,2,3)));

// Default 'help' command handler
void cli_help(CLI *cli, CliCommand* cmd);

#if defined(__cplusplus)
}
#endif
#endif  //  __CLI_H__

//  FIN
