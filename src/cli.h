
#if !defined(__CLI_H__)

#define __CLI_H__

#include <stdio.h>
#include <stdarg.h>

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

typedef struct
{
    int (*fprintf)(void *, const char *fmt, va_list va);
    void *ctx;
}   CliOutput;

typedef struct CLI {
    char *buff;
    size_t size;
    size_t end;
    size_t cursor;
    bool escape;
    bool echo;

    CliCommand *head;
    CliOutput *output;
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
void cli_insert(CLI *cli, CliCommand **head, CliCommand *cmd);
void cli_process(CLI *cli, char c);

void cli_print(CLI *cli, const char *fmt, ...) __attribute__((format(printf,2,3)));
void cli_clear(CLI *cli);

// Default 'help' command handler
void cli_help(CLI *cli, CliCommand* cmd);

void cli_nowt(CLI *cli, CliCommand *cmd);

// accessing and parsing args

const char* cli_get_arg(CLI *cli, int offset);

bool cli_parse_int(const char *s, int *value, int base);
bool cli_parse_float(const char *s, float *value);

// helper commands for list manipulation

bool cli_remove(CliCommand **head, CliCommand *item);
CliCommand *cli_find(CliCommand **head, int (*fn)(CliCommand *cmd, void *arg), void *arg);

#if defined(__cplusplus)
}
#endif
#endif  //  __CLI_H__

//  FIN
