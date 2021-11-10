
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <cli_debug.h>
#include "list.h"
#include "cli.h"

static pList* next_fn(pList item)
{
    CliCommand *cmd = (CliCommand *) item;
    return (pList*) & cmd->next;
}

    /**
     * @brief printf style output 
     */

void cli_print(CLI *cli, const char *fmt, ...)
{
    ASSERT(cli);
    ASSERT(cli->output);
    ASSERT(cli->output->fprintf);

    va_list va;
    va_start(va, fmt);
    cli->output->fprintf(cli->output->ctx, fmt, va);
    va_end(va);
}

    /**
     * @brief reset the CLI buffer / cursor
     */

void cli_clear(CLI *cli)
{
    cli->end = 0;
    cli->cursor = 0;
    cli->escape = false;
    cli->buff[0] = '\0';
    cli->nest = 0;
    for (int i = 0; i < CLI_MAX_ARGS; i++)
    {
        cli->args[i] = 0;
    }
}

    /**
     * @brief initialise the CLI structure
     *
     * allocates a text buffer \a size chars long
     *
     * sets the context CLI.ctx to \a ctx
     */

void cli_init(CLI *cli, size_t size, void *ctx)
{
    ASSERT(size);
    cli->buff = (char*) malloc(size+1);
    cli->buff[0] = '\0';
    cli->size = size;
    cli->head = 0;
    cli->ctx = ctx;

    cli_clear(cli);

    // Start with the initial prompt
    cli_print(cli, "%s", cli->prompt);
}

void cli_insert(CLI *cli, CliCommand **head, CliCommand *cmd)
{
    ASSERT(head);
    list_push((pList*) head, (pList) cmd, next_fn, cli->mutex);
}

    /**
     * @brief Inserts command \a cmd at the head of the command list
     *
     * @param cli the CLI struct
     * @param cmd the command to insert
     */

void cli_register(CLI *cli, CliCommand *cmd)
{
    cli_insert(cli, & cli->head, cmd);
}

    /*
     *
     */

static int match_cmd(pList w, void *arg)
{
    CliCommand *cmd = (CliCommand *) w;
    const char* match = (const char*) arg;

    return strcmp(cmd->cmd, match) == 0;
}

static CliCommand* _find_command(CLI *cli, CliCommand **head, const char* name)
{
    // Look up the command
    CliCommand *exec = (CliCommand*) list_find((pList*) head, next_fn, match_cmd, (void*) name, cli->mutex);
    return exec;
}

static CliCommand* find_command(CLI *cli, const char* name)
{
    return _find_command(cli, & cli->head, name);
}

static CliCommand* find_subcommand(CLI *cli, CliCommand *cmd, const char* name)
{
    return _find_command(cli, & cmd->subcommand, name);
}

static void not_found(CLI *cli, const char *cmd)
{
    cli_print(cli, "'%s' not found%s", cmd, cli->eol);
}

    /*
     *
     */

static bool execute(CLI *cli, CliCommand* cmd)
{
    ASSERT(cmd->handler);
    cmd->handler(cli, cmd);
    return true;
}

const char* cli_get_arg(CLI *cli, int offset)
{
    if ((offset < 0) || ((cli->nest + offset) >= CLI_MAX_ARGS))
    {
        return 0;
    }
    return cli->args[cli->nest + offset];
}

static bool run_command(CLI *cli, CliCommand* cmd)
{
    while (true)
    {
        if (!cmd->subcommand)
        {
            // Execute the command
            return execute(cli, cmd);
        }

        const char *s = cli_get_arg(cli, 0);
     
        if (!s)
        {
            // no args found
            return execute(cli, cmd);
        }
     
        // search subcommands looking or a match
        CliCommand *sub = find_subcommand(cli, cmd, s);
        if (!sub)
        {
            // no matching subcommand found
            return execute(cli, cmd);
        }

        // if we have positively found a subcommand, increment the nest
        cli->nest += 1;
        // test the matching subcommand
        cmd = sub;
    }
}

static void cli_execute(CLI *cli)
{
    // Extract the words in the buffer

    cli->nest = 0;
    char *save = 0;
    char *s = cli->buff;
    for (int i = 0; i < CLI_MAX_ARGS; i++)
    {
        const char* cmd = strtok_r(s, " ", & save);
        cli->args[i] = cmd;
        if (!cmd)
        {
            break;
        }
        s = 0;
    }

    const char *cmd = cli->args[cli->nest++];

    if (!cmd)
    {
        //  Empty line. Reply with a prompt
        cli_print(cli, "%s", cli->prompt);
        return;
    }

    // Look up the command
    CliCommand *exec = find_command(cli, cmd);

    if (!exec)
    {
        // Command not found
        not_found(cli, cmd);
        return;
    }

    run_command(cli, exec);
}

    /*
     *
     */

static void _help(CLI *cli, CliCommand *cmd)
{
    cli_print(cli, "%s : %s%s", cmd->cmd, cmd->help ? cmd->help : "", cli->eol);
}

static int visit_help(pList w, void *arg)
{
    // Callback function : called for each command in the list
    CliCommand *cmd = (CliCommand *) w;
    CLI *cli = (CLI*) arg;

    _help(cli, cmd);
    return 0;
}

static void _cli_help(CLI *cli, CliCommand* cmd, CliCommand **head, int offset)
{
    const char *s = cli_get_arg(cli, offset);

    if (s)
    {
        CliCommand *peer = _find_command(cli, head, s);
        // if there is a matching subcommand, nest
        if (peer)
        {
            return _cli_help(cli, peer, & peer->subcommand, offset+1);
        }

        // Can't find the command help has been requested for
        not_found(cli, s);
        return;
    }

    // If the help text is set, print it
    if (cmd->help)
    {
        _help(cli, cmd);
        return;
    }

    // Call visit_help() on all elements of the list
    ASSERT(head);
    list_visit((pList*) head, next_fn, visit_help, (void*) cli, cli->mutex);
}

    /**
     * @brief print help for the command \a cmd
     *
     * @param cli the CLI struct
     * @param cmd this command (usually 'help')
     *
     * The command.ctx pointer must point to the head of the list
     * of commands to provide help for.
     */

void cli_help(CLI *cli, CliCommand* cmd)
{
    _cli_help(cli, cmd, & cli->head, 0);
}

    /**
     * @brief null command handler : do nothing
     *
     * useful as the base for a list of subcommands
     */

void cli_nowt(CLI *cli, CliCommand *cmd)
{
    UNUSED(cli);
    UNUSED(cmd);
}

    /*
     *
     */

struct autocomplete
{
    CLI *cli;
    CliCommand *last;
    int count;
    int complete;
    size_t offset;
    bool print;
};

static int visit_auto(pList w, void *arg)
{
    // Callback function : called for each command in the list
    ASSERT(w);
    ASSERT(arg);
    CliCommand *cmd = (CliCommand *) w;
    struct autocomplete *ac = (struct autocomplete *) arg;
    CLI *cli = ac->cli;

    ASSERT(cli->end >= ac->offset);
    const char *s = & cli->buff[ac->offset];

    // are there any spaces in the command?
    const char *space = strchr(s, ' ');
    // completion with spaces needs to resolve the complete commands
    if (space)
    {
        if (!strncmp(s, cmd->cmd, (size_t) (space - s)))
        {
            // match this one completely typed command
            ac->last = cmd;
            ac->count = 1;
            // skip over all trailing ' ' for the next comparison
            for (; *space == ' '; space++)
                ;
            ac->offset += (size_t) (space - s);
            ac->complete += 1;
            return 1;
        }
        return 0;
    }

    if (!strncmp(s, cmd->cmd, (size_t) (cli->end - ac->offset)))
    {
        // matches the command so far
        ac->last = cmd;
        ac->count += 1;

        if (ac->print)
        {
            cli_print(cli, "%s%s", cmd->cmd, cli->eol);
        }
    }

    return 0;
}

static void cli_autocomplete(CLI *cli)
{
    // Check for partial match of command handlers
    struct autocomplete ac = { .cli = cli, .complete = 0, .offset = 0, .print = false };

    CliCommand **head = & cli->head;

    while (true)
    {
        // search through the ' ' seperated list of commands so far ..
        ac.count = 0;
        ac.last = 0;
        CliCommand *cmd = (CliCommand *) list_find((pList*) head, next_fn, visit_auto, (void*) & ac, cli->mutex);
        if (!cmd)
        {
            break;
        }

        // found completed this command. search for subcommands
        head = & cmd->subcommand;
    }

    if (ac.count == 0)
    {
        // No match. Do nothing
        return;
    }

    if (ac.count == 1)
    {
        // Single match. autocomplete this
        CliCommand *cmd = ac.last;
        // offset into the sole matching command
        ASSERT(cli->end >= ac.offset);
        const size_t offset = cli->end - ac.offset;
        const char *s = & cmd->cmd[offset];
        for (; *s; s++)
        {
            cli_process(cli, *s);
        }
        cli_process(cli, ' ');
        return;
    }

    // Print the partial matches
    ac.print = true;
    cli_print(cli, "%s", cli->eol);
    list_visit((pList*) head, next_fn, visit_auto, (void*) & ac, cli->mutex);
    cli_print(cli, "%s", cli->prompt);
    // restore the buffer so far ..
    cli_print(cli, "%s", cli->buff);
}

    /*
     *
     */

static void cli_draw_to_end(CLI *cli)
{
    const size_t more = cli->end - cli->cursor;

    if (more)
    {
        cli_print(cli, "%s", & cli->buff[cli->cursor]);
    }
    for (size_t i = 0; i < more; i++)
    {
        cli_print(cli, "\b");
    }
}

static void cli_edit(CLI *cli, char c)
{
    switch (c)
    {
        case 'C'    :   // cursor right
        {
            if (cli->cursor < cli->end)
            {
                cli_print(cli, "%c", cli->buff[cli->cursor]);
                cli->cursor += 1;
            }
            break;
        }
        case 'D'    :   // cursor left
        {
            if (cli->cursor > 0)
            {
                cli_print(cli, "\b");
                cli->cursor -= 1;
            }
            break;
        }
        default:    //  ignore all other ESC codes
        {
            break;
        }
    }
}

static void cli_backspace(CLI *cli)
{
    if (cli->cursor == 0)
    {
        // nothing to delete
        return;
    }

    // move chars down one
    memmove(& cli->buff[cli->cursor-1], & cli->buff[cli->cursor], cli->end - cli->cursor);

    cli->end -= 1;
    cli->buff[cli->end] = '\0';
    cli->cursor -= 1;
    // overwrite the deleted char
    cli_print(cli, " \b");
    cli_draw_to_end(cli);
}

    /**
     * @brief send char \a c to the command interpreter
     */

void cli_process(CLI *cli, char c)
{
    if (((size_t)(cli->end + 1)) >= cli->size)
    {
        //  line is full : ERROR
        cli_clear(cli);
        cli_print(cli, "%s%s", cli->eol, cli->prompt);
        return;
    }

    if (cli->escape)
    {
        // Process cursor commands
        cli_edit(cli, c);
        cli->escape = false;
        return;
    }

    if (c == 0x1b) // ASCII ESC
    {
        cli->escape = true;
        return;
    }

    if (c == '\t')
    {
        cli_autocomplete(cli);
        return;
    }

    // Echo the char
    cli_print(cli, "%c", c);

    // Just ignore carriage return
    if (c == '\r')
    {
        return;
    }

    // handle backspace
    if (c == '\b')
    {
        cli_backspace(cli);
        return;
    }

    if (c == '\n')
    {
        // Execute the line
        cli_execute(cli);
        cli_clear(cli);
        cli_print(cli, "%s", cli->prompt);
        return;
    }

    if (cli->cursor != cli->end)
    {
        // Move the rest of the buffer up one
        memmove(& cli->buff[cli->cursor+1], & cli->buff[cli->cursor], cli->end - cli->cursor);
    }

    // Append / insert the char
    cli->buff[cli->cursor] = c;

    cli->end += 1;
    cli->cursor += 1;
    cli->buff[cli->end] = '\0';
    cli_draw_to_end(cli);
}

    /**
     * @brief close the CLI command and free allocated data
     */

void cli_close(CLI *cli)
{
    ASSERT(cli);
    ASSERT(cli->buff);

    // Free the text input buffer
    free(cli->buff);
    cli->buff = 0;

    // Unlink all the actions
    cli->head = 0;
}

    /*
     *
     */

bool cli_parse_int(const char *s, int *value, int base)
{
    if (!s)
    {
        return false;
    }

    if ('\0' == *s)
    {
        // nothing in the string
        return false;
    }

    char *end = 0;
    long int val = strtol(s, & end, base);
    ASSERT(end);
    if ('\0' == *end)
    {
        *value = (int) val;
        return true;
    }

    return false;
}

    /*
     *
     */

bool cli_parse_float(const char *s, float *value)
{
    if (!s)
    {
        return false;
    }

    if ('\0' == *s)
    {
        // nothing in the string
        return false;
    }

    char *end = 0;
    float val = strtof(s, & end);
    ASSERT(end);
    if ('\0' == *end)
    {
        *value = val;
        return true;
    }

    return false;
}

    /*
     *
     */

bool cli_remove(CliCommand **head, CliCommand *item)
{
    ASSERT(head);
    ASSERT(item);
    return list_remove((pList*) head, (pList) item, next_fn, 0);
}

CliCommand *cli_find(CliCommand **head, int (*fn)(CliCommand *cmd, void *arg), void *arg)
{
    ASSERT(head);
    ASSERT(fn);
    return (CliCommand*) list_find((pList*) head, next_fn, (visitor) fn, arg, 0);
}

//  FIN
