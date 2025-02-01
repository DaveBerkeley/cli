
#include <gtest/gtest.h>

#include <cli_debug.h>
#include "../src/list.h"
#include "../src/cli.h"
#include "../src/io.h"
#include "test_io.h"

    /*
     *
     */

static void cli_send(CLI *cli, const char* s)
{
    for (; *s; s++)
    {
        cli_process(cli, *s);
    }
}

    /*
     *
     */

CLI cli = {
    .output = 0,
    .prompt = "> ",
    .eol = "\r\n",
};

    /*
     *
     */

static int got_action = false;

static void action_handler(CLI *cli, CliCommand *cmd)
{
    UNUSED(cli);
    UNUSED(cmd);
    got_action = true;
}

TEST(CLI, Create)
{
    CliCommand action = {
        .cmd = "help",
        .handler = action_handler,
        .help = "help text",
    };

    got_action = false;
    cli_init(& cli, 64, 0);
    cli_register(& cli, & action);

    // Check that chars are stored in the buffer
    EXPECT_STREQ("", cli.buff);
    cli_send(& cli, "help");
    EXPECT_STREQ("help", cli.buff);

    cli_close(& cli);
}

TEST(CLI, Close)
{
    CliCommand action = {
        .cmd = "help",
        .handler = action_handler,
        .help = "help text",
    };
    CliCommand more = {
        .cmd = "more",
        .handler = action_handler,
        .help = "help text",
    };

    got_action = false;
    cli_init(& cli, 64, 0);
    cli_register(& cli, & action);
    cli_register(& cli, & more);

    // Check that chars are stored in the buffer
    EXPECT_STREQ("", cli.buff);
    cli_send(& cli, "help");
    EXPECT_STREQ("help", cli.buff);

    cli_close(& cli);
    // Check the actions are unlinked
    EXPECT_EQ(cli.head, (void*)0);
}

static void cli_die(CLI *cli, CliCommand *cmd)
{
    UNUSED(cli);
    UNUSED(cmd);
    ASSERT(false);
}

TEST(CLI, Help)
{
#define HELP0 "one line of text" 
#define HELP1 "another help line"
#define HELP2 "some more help" 

    CliCommand a0 = {
        .cmd = "help",
        .handler = cli_help,
        .help = HELP0,
    };
    CliCommand a1 = {
        .cmd = "anything",
        .handler = cli_die,
        .help = HELP1,
    };
    CliCommand a2 = {
        .cmd = "another",
        .handler = cli_die,
        .help = HELP2,
    };

    cli_init(& cli, 64, 0);
    cli_register(& cli, & a2);
    cli_register(& cli, & a1);
    cli_register(& cli, & a0);

    io.reset();
    cli_send(& cli, "help\r\n");
    EXPECT_STREQ("help\r\n" "help : " HELP0 "\r\n> ", io.get());

    io.reset();
    cli_send(& cli, "help anything\r\n");
    EXPECT_STREQ("help anything\r\n" "anything : " HELP1 "\r\n> ", io.get());

    io.reset();
    cli_send(& cli, "help another\r\n");
    EXPECT_STREQ("help another\r\n" "another : " HELP2 "\r\n> ", io.get());

    io.reset();
    cli_send(& cli, "help xx\r\n");
    EXPECT_STREQ("help xx\r\n" "'xx' not found\r\n> ", io.get());

    io.reset();
    cli_send(& cli, "help another xx\r\n");
    EXPECT_STREQ("help another xx\r\n" "'xx' not found\r\n> ", io.get());

    // check that cli_help with no help text lists the available commands
    io.reset();
    a0.help = 0;
    cli_send(& cli, "help\r\n");
    EXPECT_STREQ("help\r\n" "help : \r\nanything : " HELP1 "\r\nanother : " HELP2 "\r\n> ", io.get());

    cli_close(& cli);
}

TEST(CLI, HelpSub)
{
    CliCommand a2 = {
        .cmd = "another",
        .handler = cli_die,
        .help = HELP2,
    };
    CliCommand a1 = {
        .cmd = "anything",
        .handler = cli_die,
        .help = HELP1,
        .subcommand = & a2,
    };
    CliCommand a0 = {
        .cmd = "help",
        .handler = cli_help,
        .help = HELP0,
    };

    cli_init(& cli, 64, 0);
    cli_register(& cli, & a0);
    cli_register(& cli, & a1);

    // Check help <command>
    io.reset();
    cli_send(& cli, "help anything\r\n");
    EXPECT_STREQ("", cli.buff);
    EXPECT_STREQ("help anything\r\n" "anything : " HELP1 "\r\n> ", io.get());

    // Check help <unknown>
    io.reset();
    cli_send(& cli, "help nowt\r\n");
    EXPECT_STREQ("", cli.buff);
    EXPECT_STREQ("help nowt\r\n" "'nowt' not found\r\n> ", io.get());

    // Check help <command> <subcommand>
    io.reset();
    cli_send(& cli, "help anything another\r\n");
    EXPECT_STREQ("", cli.buff);
    EXPECT_STREQ("help anything another\r\n" "another : " HELP2 "\r\n> ", io.get());

    cli_close(& cli);
}

TEST(CLI, Backspace)
{
    CliCommand a0 = {
        .cmd = "help",
        .handler = cli_help,
        .help = "help!",
    };

    cli_init(& cli, 64, 0);
    cli_register(& cli, & a0);

    io.reset();
    cli_send(& cli, "heldx");
    EXPECT_STREQ("heldx", cli.buff);
    EXPECT_STREQ("heldx", io.get());
    EXPECT_EQ(5, cli.end);
    EXPECT_EQ(5, cli.cursor);

    io.reset();
    cli_send(& cli, "\b");
    EXPECT_STREQ("held", cli.buff);
    EXPECT_STREQ("\b \b", io.get());
    EXPECT_EQ(4, cli.end);
    EXPECT_EQ(4, cli.cursor);

    io.reset();
    cli_send(& cli, "\b");
    EXPECT_STREQ("hel", cli.buff);
    EXPECT_STREQ("\b \b", io.get());
    EXPECT_EQ(3, cli.end);
    EXPECT_EQ(3, cli.cursor);

    cli_send(& cli, "p\r\n");
    // Buffer should be cleared
    EXPECT_STREQ("", cli.buff);
    EXPECT_EQ(0, cli.end);
    EXPECT_EQ(0, cli.cursor);

    // backspace on empty buffer does nothing
    io.reset();
    cli_send(& cli, "\b");
    EXPECT_STREQ("\b", io.get());
    EXPECT_STREQ("", cli.buff);
    EXPECT_EQ(0, cli.end);
    EXPECT_EQ(0, cli.cursor);

    cli_close(& cli);
}

static const char *ctx_text = "hello world";
static bool ctx_ran = false;

void check_ctx(CLI *cli, CliCommand *cmd)
{
    UNUSED(cmd);
    ctx_ran = true;
    EXPECT_EQ(cli->ctx, ctx_text);
}

TEST(CLI, Context)
{
    CliCommand a0 = {
        .cmd = "help",
        .handler = check_ctx,
        .help = "help!",
    };

    cli_init(& cli, 64, (void*) ctx_text);
    cli_register(& cli, & a0);

    ctx_ran = false;
    cli_send(& cli, "help\r\n");
    EXPECT_TRUE(ctx_ran);

    // Buffer should be cleared
    EXPECT_STREQ("", cli.buff);

    cli_close(& cli);
}

TEST(CLI, EmptyLine)
{
    CliCommand a0 = {
        .cmd = "help",
        .handler = cli_help,
    };

    cli_init(& cli, 64, 0);
    cli_register(& cli, & a0);

    cli_send(& cli, "\r\n");

    // Buffer should be cleared
    EXPECT_STREQ("", cli.buff);

    cli_close(& cli);
}

TEST(CLI, OverflowLine)
{
    CliCommand a0 = {
        .cmd = "help",
        .handler = cli_help,
    };

    io.reset();
    cli_init(& cli, 10, 0);
    cli_register(& cli, & a0);

    for (int i = 0; i < 10; i++)
    {
        cli_send(& cli, "x");
    }

    // Currently silently ignores the too-long command
    EXPECT_STREQ("> xxxxxxxxx\r\n> ", io.get());
    // Buffer should be cleared
    EXPECT_STREQ("", cli.buff);

    cli_close(& cli);
}

    /*
     *
     */

typedef struct Device
{
    const char* name;
    bool (*set)(int value);
    int  (*get)();
    bool (*power)(bool on);
}   Device;

static int laser_value;
bool set_laser(int v) { laser_value = v; return true; }
int get_laser() { return laser_value; }
bool power_laser(bool on) { UNUSED(on); return true; }

static Device laser = { "laser", set_laser, get_laser, 0 };

static void dev_power(CLI *cli, CliCommand *cmd)
{
    ASSERT(cmd->ctx);
    Device *dev = (Device*) cmd->ctx;

    const char *s = cli_get_arg(cli, 0);

    if (!strcmp(s, "?"))
    {
        // query the device
        ASSERT(dev->get);
        cli_print(cli, "%d%s", dev->get(), cli->eol);
        return;
    }

    char *end = 0;
    const long int v = strtol(s, & end, 10);
    if (*end != '\0')
    {
        // Number not fully converted, so an error
        LOG_DEBUG("error in value '%s'", s);
        return;
    }

    ASSERT(dev->set);
    const bool ok = dev->set((int) v);
    cli_print(cli, "%s%s", ok ? "ok" : "error", cli->eol);
}

    /*
     *
     */

TEST(CLI, Power)
{
    CliCommand s0 = {
        .cmd = "laser",
        .handler = dev_power,
        .help = "laser <on>|<off>|?",
        .ctx = & laser,
    };
    CliCommand a0 = {
        .cmd = "power",
        .handler = cli_help,
        .help = "power <device>",
        .subcommand = & s0,
    };

    laser_value = 0;
    cli_init(& cli, 64, 0);
    cli_register(& cli, & a0);

    // Query the device
    io.reset();
    cli_send(& cli, "power laser ?\r\n");

    EXPECT_STREQ("", cli.buff);
    EXPECT_STREQ("power laser ?\r\n0\r\n> ", io.get());

    // Set the device
    io.reset();
    cli_send(& cli, "power laser 1\r\n");
    EXPECT_STREQ("", cli.buff);
    EXPECT_STREQ("power laser 1\r\nok\r\n> ", io.get());
    EXPECT_EQ(1, laser_value); 

    // Set the device
    io.reset();
    cli_send(& cli, "power laser 10\r\n");
    EXPECT_STREQ("", cli.buff);
    EXPECT_STREQ("power laser 10\r\nok\r\n> ", io.get());
    EXPECT_EQ(10, laser_value); 

    io.reset();
    cli_close(& cli);
}

    /*
     *
     */

struct ctx {
    bool done;
};

static void hello(CLI *cli, CliCommand *cmd)
{
    UNUSED(cmd);
    const char *s = cli->args[1];
    EXPECT_STREQ("world", s);
    LOG_DEBUG("%s", s);
}

static void bye(CLI *cli, CliCommand *cmd)
{
    UNUSED(cmd);
    LOG_DEBUG("");

    struct ctx *ctx = (struct ctx*) cli->ctx;
    ctx->done = true;
}

TEST(CLI, Input)
{
    CliCommand a0 = {
        .cmd = "hello",
        .handler = hello,
    };
    CliCommand a1 = {
        .cmd = "bye",
        .handler = bye,
    };

    struct ctx ctx = { .done = false };

    cli_init(& cli, 64, & ctx);
    cli_register(& cli, & a0);
    cli_register(& cli, & a1);

    io.reset();

    FILE *in = fopen("test/input.txt", "r");

    while (!feof(in))
    {
        char buff[64];

        const char *s = fgets(buff, sizeof(buff), in);
        if (!s)
        {
            break;
        }

        cli_send(& cli, buff);
    }

    fclose(in);

    EXPECT_TRUE(ctx.done);

    cli_close(& cli);
}

    /*
     *
     */

TEST(CLI, AutoComplete)
{
    CliCommand s2 = {
        .cmd = "three",
        .handler = cli_nowt,
    };
    CliCommand s1 = {
        .cmd = "two",
        .handler = cli_nowt,
        .subcommand = & s2,
    };
    CliCommand s0 = {
        .cmd = "one",
        .handler = cli_nowt,
        .subcommand = & s1,
    };
    CliCommand a0 = {
        .cmd = "hello",
        .handler = cli_nowt,
        .subcommand = & s0,
    };
    CliCommand a1 = {
        .cmd = "bye",
        .handler = cli_nowt,
    };
    CliCommand a2 = {
        .cmd = "partial",
        .handler = cli_nowt,
    };
    CliCommand a3 = {
        .cmd = "part",
        .handler = cli_nowt,
    };
    CliCommand x0 = {
        .cmd = "xx",
        .handler = cli_nowt,
    };
    CliCommand x1 = {
        .cmd = "yy",
        .handler = cli_nowt,
        .next = & x0,
    };
    CliCommand x2 = {
        .cmd = "zz",
        .handler = cli_nowt,
        .next = & x1,
    };
    CliCommand a4 = {
        .cmd = "abcd",
        .handler = cli_nowt,
        .subcommand = & x2,
    };

    cli_init(& cli, 64, 0);
    cli_register(& cli, & a4);
    cli_register(& cli, & a3);
    cli_register(& cli, & a2);
    cli_register(& cli, & a1);
    cli_register(& cli, & a0);

    io.reset();
    // should list all the commands
    cli_process(& cli, '\t');
    EXPECT_STREQ("\r\nhello\r\nbye\r\npartial\r\npart\r\nabcd\r\n> ", io.get());

    // 'h' '\t' should complete 'hello '
    io.reset();
    cli_send(& cli, "h\t");
    EXPECT_STREQ("hello ", io.get());
    cli_send(& cli, "\r\n"); // complete the command

    // "par\t" should match 'part' and 'partial'
    io.reset();
    cli_send(& cli, "par\t");
    EXPECT_STREQ("par\r\npartial\r\npart\r\n> par", io.get());
    cli_send(& cli, "\r\n"); // complete the command

    // "part\t" should match 'part' and 'partial'
    io.reset();
    cli_send(& cli, "part\t");
    EXPECT_STREQ("part\r\npartial\r\npart\r\n> part", io.get());
    cli_send(& cli, "\r\n"); // complete the command

    // "parti\t" should match 'partial'
    io.reset();
    cli_send(& cli, "parti\t");
    EXPECT_STREQ("partial ", io.get());
    cli_send(& cli, "\r\n"); // complete the command

    // "partial \t" should match 'partial' and do nothing
    io.reset();
    cli_send(& cli, "partial \t");
    EXPECT_STREQ("partial ", io.get());
    cli_send(& cli, "\r\n"); // complete the command

    // test autocomplete for subcommands

    // "part \t" should invoke subcommand handler
    io.reset();
    cli_send(& cli, "part \t");
    EXPECT_STREQ("part ", io.get());
    cli_send(& cli, "\r\n"); // complete the command

    // test autocomplete for subcommands

    io.reset();
    cli_send(& cli, "hello o\t");
    EXPECT_STREQ("hello one ", io.get());
    cli_send(& cli, "\r\n"); // complete the command

    io.reset();
    cli_send(& cli, "hello  o\t");
    EXPECT_STREQ("hello  one ", io.get());
    cli_send(& cli, "\r\n"); // complete the command

    io.reset();
    cli_send(& cli, "hello one tw\t");
    EXPECT_STREQ("hello one two ", io.get());
    cli_send(& cli, "\r\n"); // complete the command

    io.reset();
    cli_send(& cli, "hello one two \t");
    EXPECT_STREQ("hello one two three ", io.get());
    cli_send(& cli, "\r\n"); // complete the command

    io.reset();
    cli_send(& cli, "hello one two th\t");
    EXPECT_STREQ("hello one two three ", io.get());
    cli_send(& cli, "\r\n"); // complete the command

    // no completion possible past error
    io.reset();
    cli_send(& cli, "hello one tx\t");
    EXPECT_STREQ("hello one tx", io.get());
    cli_send(& cli, "\r\n"); // complete the command

    // complete 'hello' command
    io.reset();
    cli_send(& cli, "hello\t");
    EXPECT_STREQ("hello ", io.get());
    cli_send(& cli, "\r\n"); // complete the command

    io.reset();
    cli_send(& cli, "hello \t");
    EXPECT_STREQ("hello one ", io.get());
    cli_send(& cli, "\r\n"); // complete the command

    io.reset();
    cli_send(& cli, "abcd \t");
    EXPECT_STREQ("abcd \r\nzz\r\nyy\r\nxx\r\n> abcd ", io.get());
    cli_send(& cli, "\r\n"); // complete the command

    cli_close(& cli);
}

    /*
     *
     */

static void sub(CLI *cli, CliCommand *cmd)
{
    void *ctx = cmd->ctx;
    int v = * (int *) ctx;
    const char *s = cli_get_arg(cli, 0);
    s = s ? s : "";
    cli_print(cli, "got %s '%s' %d\r\n", cmd->cmd, s, v);
}

TEST(CLI, Subcommand)
{
    int i0 = 3;
    CliCommand a0 = {
        .cmd = "three",
        .handler = sub,
        .subcommand = 0,
        .ctx = & i0,
    };
    int i1 = 2;
    CliCommand a1 = {
        .cmd = "two",
        .handler = sub,
        .subcommand = & a0,
        .ctx = & i1,
    };
    int i2 = 1;
    CliCommand a2 = {
        .cmd = "one",
        .handler = sub,
        .subcommand = & a1,
        .ctx = & i2,
    };
    int i3 = 0;
    CliCommand a3 = {
        .cmd = "top",
        .handler = sub,
        .subcommand = & a2,
        .ctx = & i3,
    };

    cli_init(& cli, 64, 0);
    cli_register(& cli, & a3);

    io.reset();

    io.reset();
    cli_send(& cli, "top\r\n");
    EXPECT_STREQ("top\r\ngot top '' 0\r\n> ", io.get());

    io.reset();
    cli_send(& cli, "top xx\r\n");
    EXPECT_STREQ("top xx\r\ngot top 'xx' 0\r\n> ", io.get());

    io.reset();
    cli_send(& cli, "top one\r\n");
    EXPECT_STREQ("top one\r\ngot one '' 1\r\n> ", io.get());

    io.reset();
    cli_send(& cli, "top one xx\r\n");
    EXPECT_STREQ("top one xx\r\ngot one 'xx' 1\r\n> ", io.get());

    io.reset();
    cli_send(& cli, "top one two\r\n");
    EXPECT_STREQ("top one two\r\ngot two '' 2\r\n> ", io.get());

    io.reset();
    cli_send(& cli, "top one two xx\r\n");
    EXPECT_STREQ("top one two xx\r\ngot two 'xx' 2\r\n> ", io.get());

    io.reset();
    cli_send(& cli, "top one two three\r\n");
    EXPECT_STREQ("top one two three\r\ngot three '' 3\r\n> ", io.get());

    io.reset();
    cli_send(& cli, "top one two three xx\r\n");
    EXPECT_STREQ("top one two three xx\r\ngot three 'xx' 3\r\n> ", io.get());

    cli_close(& cli);
}

static void cli_source(CLI *cli, CliCommand *cmd)
{
    UNUSED(cmd);

    const char* fname = cli_get_arg(cli, 0);
    if (!fname)
    {
        cli_print(cli, "no file specified");
        return;
    }

    FILE *f = fopen(fname, "r");
    if (!f)
    {
        cli_print(cli, "error opening file '%s'", fname);
        return;
    }

    cli_clear(cli);

    while (!feof(f))
    {
        char buff[32];
        char *s = fgets(buff, sizeof(buff), f);
        if (!s)
        {
            break;
        }

        for (; *s; s++)
        {
            cli_process(cli, *s);
        }
    }

    fclose(f);
}

static void echo(CLI *cli, CliCommand *cmd)
{
    UNUSED(cmd);
    for (int i = 0; ; i++)
    {
        const char *s = cli_get_arg(cli, i);
        if (!s)
        {
            break;
        }

        if (i)
        {
            cli_print(cli, " ");
        }
        cli_print(cli, "%s", s);
    }
    cli_print(cli, "%s", cli->eol);
}

TEST(CLI, File)
{
    CliCommand a0 = {
        .cmd = "three",
        .handler = echo,
    };
    CliCommand a1 = {
        .cmd = "two",
        .handler = echo,
    };
    CliCommand a2 = {
        .cmd = "one",
        .handler = echo,
        .help = "1111",
    };
    CliCommand a3 = {
        .cmd = "file",
        .handler = cli_source,
    };
    CliCommand a4 = {
        .cmd = "help",
        .handler = cli_help,
    };

    cli_init(& cli, 64, 0);
    cli_register(& cli, & a4);
    cli_register(& cli, & a3);
    cli_register(& cli, & a2);
    cli_register(& cli, & a1);
    cli_register(& cli, & a0);

    io.reset();
    cli_send(& cli, "file test/redirect.txt\r\n");

    char *s = strdup(io.get());
    io.reset();

    char *d = 0;
    size_t size = 0;

    FILE *f = open_memstream(& d, & size);

    // Mimic the output
    fprintf(f, "file test/redirect.txt\r\n");
    fprintf(f, "help one\none : 1111\r\n> ");
    fprintf(f, "one xx\nxx\r\n> ");
    fprintf(f, "two yy\nyy\r\n> ");
    fprintf(f, "three zz 1234 5678\nzz 1234 5678\r\n> ");
    fprintf(f, "> ");
    fwrite("", 1, 1, f); // '\0' terminate
    fflush(f);

    EXPECT_STREQ(d, s);
    free(s);
    fclose(f);
    free(d);

    cli_close(& cli);
}

TEST(CLI, Two)
{
    CliCommand a0 = {
        .cmd = "three",
        .handler = echo,
    };
    CliCommand a1 = {
        .cmd = "two",
        .handler = echo,
    };
    CliCommand a2 = {
        .cmd = "one",
        .handler = echo,
    };

    CLI cli1 = {
        .output = cli.output,
        .prompt = "> ",
        .eol = "\r\n",
    };
    CLI cli2 = {
        .output = cli.output,
        .prompt = "> ",
        .eol = "\r\n",
    };
    cli_init(& cli1, 64, 0);
    cli_init(& cli2, 64, 0);

    CliCommand *head = 0;
    cli_insert(& cli, & head, & a2);
    cli_insert(& cli, & head, & a1);
    cli_insert(& cli, & head, & a0);

    // give both CLI instances the same command tree
    cli1.head = head;
    cli2.head = head;

    // check both instances

    io.reset();
    cli_send(& cli1, "one a b c");
    EXPECT_STREQ("one a b c", io.get());

    io.reset();
    cli_send(& cli2, "two");
    EXPECT_STREQ("two", io.get());

    // complete the command in the first instance
    io.reset();
    cli_send(& cli1, "\n");
    EXPECT_STREQ("\na b c\r\n> ", io.get());

    // complete the command in the second instance
    io.reset();
    cli_send(& cli2, " xx\n");
    EXPECT_STREQ(" xx\nxx\r\n> ", io.get());

    cli_close(& cli1);
    cli_close(& cli2);
}

    /*
     *
     */

TEST(CLI, Edit)
{
    CliCommand a0 = {
        .cmd = "nowt",
        .handler = cli_nowt,
    };

    cli_init(& cli, 64, 0);
    cli_register(& cli, & a0);

    io.reset();
    cli_send(& cli, "help");
    EXPECT_STREQ("help", io.get());
    EXPECT_STREQ("help", cli.buff);
    EXPECT_EQ(4, cli.end);
    EXPECT_EQ(4, cli.cursor);

    // \eD cursor left
    // \eC cursor right

    io.reset();
    cli_send(& cli, "\eD");
    EXPECT_STREQ("\b", io.get());
    EXPECT_EQ(4, cli.end);
    EXPECT_EQ(3, cli.cursor);

    io.reset();
    cli_send(& cli, "\eD");
    EXPECT_STREQ("\b", io.get());
    EXPECT_EQ(4, cli.end);
    EXPECT_EQ(2, cli.cursor);

    // test insert
    io.reset();
    cli_send(& cli, "X");
    EXPECT_STREQ("heXlp", cli.buff);
    EXPECT_STREQ("Xlp\b\b", io.get());
    EXPECT_EQ(5, cli.end);
    EXPECT_EQ(3, cli.cursor);

    io.reset();
    cli_send(& cli, "Y");
    EXPECT_STREQ("heXYlp", cli.buff);
    EXPECT_STREQ("Ylp\b\b", io.get());
    EXPECT_EQ(6, cli.end);
    EXPECT_EQ(4, cli.cursor);

    io.reset();
    cli_send(& cli, "\eD");
    EXPECT_STREQ("\b", io.get());
    EXPECT_STREQ("heXYlp", cli.buff);
    EXPECT_EQ(6, cli.end);
    EXPECT_EQ(3, cli.cursor);

    io.reset();
    cli_send(& cli, "\b");
    EXPECT_STREQ("\b \bYlp\b\b\b", io.get());
    EXPECT_STREQ("heYlp", cli.buff);
    EXPECT_EQ(5, cli.end);
    EXPECT_EQ(2, cli.cursor);

    io.reset();
    cli_send(& cli, "\eC");
    EXPECT_STREQ("Y", io.get());
    EXPECT_STREQ("heYlp", cli.buff);
    EXPECT_EQ(5, cli.end);
    EXPECT_EQ(3, cli.cursor);

    io.reset();
    cli_send(& cli, "\b");
    EXPECT_STREQ("\b \blp\b\b", io.get());
    EXPECT_STREQ("help", cli.buff);
    EXPECT_EQ(4, cli.end);
    EXPECT_EQ(2, cli.cursor);

    io.reset();
    cli_send(& cli, "\eD");
    EXPECT_STREQ("\b", io.get());
    EXPECT_STREQ("help", cli.buff);
    EXPECT_EQ(4, cli.end);
    EXPECT_EQ(1, cli.cursor);

    io.reset();
    cli_send(& cli, "\eD");
    EXPECT_STREQ("\b", io.get());
    EXPECT_STREQ("help", cli.buff);
    EXPECT_EQ(4, cli.end);
    EXPECT_EQ(0, cli.cursor);

    // << to the start of line
    io.reset();
    cli_send(& cli, "\eD");
    EXPECT_STREQ("", io.get());
    EXPECT_STREQ("help", cli.buff);
    EXPECT_EQ(4, cli.end);
    EXPECT_EQ(0, cli.cursor);

    io.reset();
    cli_send(& cli, "\eC");
    EXPECT_STREQ("h", io.get());
    EXPECT_STREQ("help", cli.buff);
    EXPECT_EQ(4, cli.end);
    EXPECT_EQ(1, cli.cursor);

    io.reset();
    cli_send(& cli, "\eC");
    EXPECT_STREQ("e", io.get());
    EXPECT_STREQ("help", cli.buff);
    EXPECT_EQ(4, cli.end);
    EXPECT_EQ(2, cli.cursor);

    io.reset();
    cli_send(& cli, "\eC");
    EXPECT_STREQ("l", io.get());
    EXPECT_STREQ("help", cli.buff);
    EXPECT_EQ(4, cli.end);
    EXPECT_EQ(3, cli.cursor);

    io.reset();
    cli_send(& cli, "\eC");
    EXPECT_STREQ("p", io.get());
    EXPECT_STREQ("help", cli.buff);
    EXPECT_EQ(4, cli.end);
    EXPECT_EQ(4, cli.cursor);

    // >> at end of line, do nothing
    io.reset();
    cli_send(& cli, "\eC");
    EXPECT_STREQ("", io.get());
    EXPECT_STREQ("help", cli.buff);
    EXPECT_EQ(4, cli.end);
    EXPECT_EQ(4, cli.cursor);

    // insert at end of line
    io.reset();
    cli_send(& cli, "x");
    EXPECT_STREQ("x", io.get());
    EXPECT_STREQ("helpx", cli.buff);
    EXPECT_EQ(5, cli.end);
    EXPECT_EQ(5, cli.cursor);
    // complete the command
    cli_send(& cli, "\n");

    // ignore cursor up and down
    io.reset();
    // set up arbitrary data
    cli_send(& cli, "abc");
    cli_send(& cli, "\eD"); // <<
    EXPECT_STREQ("abc\b", io.get());
    EXPECT_STREQ("abc", cli.buff);
    EXPECT_EQ(3, cli.end);
    EXPECT_EQ(2, cli.cursor);

    cli_send(& cli, "\eA"); // cursor up
    EXPECT_STREQ("abc\b", io.get());
    EXPECT_STREQ("abc", cli.buff);
    EXPECT_EQ(3, cli.end);
    EXPECT_EQ(2, cli.cursor);

    cli_send(& cli, "\eB"); // cursor down
    EXPECT_STREQ("abc\b", io.get());
    EXPECT_STREQ("abc", cli.buff);
    EXPECT_EQ(3, cli.end);
    EXPECT_EQ(2, cli.cursor);

    cli_close(& cli);
}

    /*
     *
     */

class GPIO 
{
public:
    bool state;

    GPIO() : state(false) { }

    void set(bool on)
    {
        state = on;
    }
};

static GPIO pa1, pb2, pd5;

static void gpio_out(CLI *cli, CliCommand *cmd)
{
    GPIO *gpio = (GPIO*) cmd->ctx;
    ASSERT(gpio);

    const char *s = cli_get_arg(cli, 0);

    int value = -1;

    if (s)
    {
        if (!strcmp(s, "0"))
        {
            value = 0;
        }
        if (!strcmp(s, "1"))
        {
            value = 1;
        }
    }

    if (value == -1)
    {
        cli_print(cli, "expected 0|1%s", cli->eol);
        return;
    }

    gpio->set(value);
    cli_print(cli, "ok%s", cli->eol);
}

class GpioCmd
{
public:
    CliCommand cmd;

    GpioCmd(const char *name, GPIO *gpio)
    {
        memset(& cmd, 0, sizeof(cmd));
        cmd.cmd = name;
        cmd.handler = gpio_out;
        cmd.ctx = gpio;
    }

    void add(CLI *cli, CliCommand *parent)
    {
        cli_insert(cli, & parent->subcommand, & cmd);
    }
};

TEST(CLI, GPIO)
{
    GpioCmd g0("PD5", & pd5);
    GpioCmd g1("PB2", & pb2);
    GpioCmd g2("PA1", & pa1);

    CliCommand a0 = {
        .cmd = "gpio",
        .handler = cli_nowt,
    };

    cli_init(& cli, 64, 0);
    cli_register(& cli, & a0);
    g0.add(& cli, & a0);
    g1.add(& cli, & a0);
    g2.add(& cli, & a0);

    cli_send(& cli, "gpio\n");
    EXPECT_FALSE(pa1.state);
    EXPECT_FALSE(pb2.state);
    EXPECT_FALSE(pd5.state);

    cli_send(& cli, "gpio PA1\n");
    EXPECT_FALSE(pa1.state);
    EXPECT_FALSE(pb2.state);
    EXPECT_FALSE(pd5.state);

    cli_send(& cli, "gpio PA1 1\n");
    EXPECT_TRUE(pa1.state);
    EXPECT_FALSE(pb2.state);
    EXPECT_FALSE(pd5.state);

    cli_send(& cli, "gpio PB2 1\n");
    EXPECT_TRUE(pa1.state);
    EXPECT_TRUE(pb2.state);
    EXPECT_FALSE(pd5.state);

    cli_send(& cli, "gpio PD5 0\n");
    EXPECT_TRUE(pa1.state);
    EXPECT_TRUE(pb2.state);
    EXPECT_FALSE(pd5.state);

    cli_send(& cli, "gpio PA1 3\n");
    EXPECT_TRUE(pa1.state);
    EXPECT_TRUE(pb2.state);
    EXPECT_FALSE(pd5.state);

    cli_send(& cli, "gpio PA1 0\n");
    EXPECT_FALSE(pa1.state);
    EXPECT_TRUE(pb2.state);
    EXPECT_FALSE(pd5.state);

    cli_send(& cli, "gpio PD5 1\n");
    EXPECT_FALSE(pa1.state);
    EXPECT_TRUE(pb2.state);
    EXPECT_TRUE(pd5.state);

    cli_send(& cli, "gpio PA1 1\n");
    EXPECT_TRUE(pa1.state);
    EXPECT_TRUE(pb2.state);
    EXPECT_TRUE(pd5.state);

    cli_send(& cli, "gpio PA1 0\n");
    EXPECT_FALSE(pa1.state);
    EXPECT_TRUE(pb2.state);
    EXPECT_TRUE(pd5.state);

    cli_send(& cli, "gpio PB2 0\n");
    EXPECT_FALSE(pa1.state);
    EXPECT_FALSE(pb2.state);
    EXPECT_TRUE(pd5.state);

    cli_send(& cli, "gpio PD5 0\n");
    EXPECT_FALSE(pa1.state);
    EXPECT_FALSE(pb2.state);
    EXPECT_FALSE(pd5.state);

    // check autocomplete
    io.reset();
    cli_send(& cli, "gpio \t");
    EXPECT_STREQ("gpio \r\nPA1\r\nPB2\r\nPD5\r\n> gpio ", io.get());
    // check that "gpio \n" does nothing
    io.reset();
    cli_send(& cli, "\n");
    EXPECT_STREQ("\n> ", io.get());

    cli_close(& cli);
}

    /*
     *
     */

TEST(CLI, ParseInt)
{
    // Test :
    //bool cli_parse_int(const char *s, int *value, int base);

    int value;
    bool ok;

    ok = cli_parse_int("1234", & value, 10);
    EXPECT_TRUE(ok);
    EXPECT_EQ(1234, value);

    ok = cli_parse_int("-1234", & value, 10);
    EXPECT_TRUE(ok);
    EXPECT_EQ(-1234, value);

    ok = cli_parse_int("0", & value, 10);
    EXPECT_TRUE(ok);
    EXPECT_EQ(0, value);

    ok = cli_parse_int("", & value, 10);
    EXPECT_FALSE(ok);

    ok = cli_parse_int(0, & value, 10);
    EXPECT_FALSE(ok);

    // leading space is okay
    ok = cli_parse_int("  123", & value, 10);
    EXPECT_TRUE(ok);
    EXPECT_EQ(123, value);

    // trailing space is not
    ok = cli_parse_int("123 ", & value, 10);
    EXPECT_FALSE(ok);

    // floats should fail
    ok = cli_parse_int("12.34", & value, 10);
    EXPECT_FALSE(ok);

    // strings should fail
    ok = cli_parse_int("hello123", & value, 10);
    EXPECT_FALSE(ok);

    // strings should fail
    ok = cli_parse_int("123hello", & value, 10);
    EXPECT_FALSE(ok);

    // Hex works
    ok = cli_parse_int("0x1234", & value, 16);
    EXPECT_TRUE(ok);
    EXPECT_EQ(0x1234, value);

    // Hex works
    ok = cli_parse_int("0X1234", & value, 16);
    EXPECT_TRUE(ok);
    EXPECT_EQ(0x1234, value);

    // Hex works : without the leading 0x
    ok = cli_parse_int("1234", & value, 16);
    EXPECT_TRUE(ok);
    EXPECT_EQ(0x1234, value);
}

    /*
     *
     */

TEST(CLI, ParseLong)
{
    // Test :
    //bool cli_parse_long(const char *s, long *value, int base);

    long value;
    bool ok;

    ok = cli_parse_long("1234", & value, 10);
    EXPECT_TRUE(ok);
    EXPECT_EQ(1234, value);

    ok = cli_parse_long("-1234", & value, 10);
    EXPECT_TRUE(ok);
    EXPECT_EQ(-1234, value);

    ok = cli_parse_long("0", & value, 10);
    EXPECT_TRUE(ok);
    EXPECT_EQ(0, value);

    ok = cli_parse_long("", & value, 10);
    EXPECT_FALSE(ok);

    ok = cli_parse_long(0, & value, 10);
    EXPECT_FALSE(ok);

    // leading space is okay
    ok = cli_parse_long("  123", & value, 10);
    EXPECT_TRUE(ok);
    EXPECT_EQ(123, value);

    // trailing space is not
    ok = cli_parse_long("123 ", & value, 10);
    EXPECT_FALSE(ok);

    // floats should fail
    ok = cli_parse_long("12.34", & value, 10);
    EXPECT_FALSE(ok);

    // strings should fail
    ok = cli_parse_long("hello123", & value, 10);
    EXPECT_FALSE(ok);

    // strings should fail
    ok = cli_parse_long("123hello", & value, 10);
    EXPECT_FALSE(ok);

    // Hex works
    ok = cli_parse_long("0x1234", & value, 16);
    EXPECT_TRUE(ok);
    EXPECT_EQ(0x1234, value);

    // Hex works
    ok = cli_parse_long("0X1234", & value, 16);
    EXPECT_TRUE(ok);
    EXPECT_EQ(0x1234, value);

    // Hex works : without the leading 0x
    ok = cli_parse_long("1234", & value, 16);
    EXPECT_TRUE(ok);
    EXPECT_EQ(0x1234, value);

    ok = cli_parse_long("0xffffffff", & value, 0);
    EXPECT_TRUE(ok);
    EXPECT_EQ(0xffffffff, value);
}

    /*
     *
     */

TEST(CLI, ParseFloat)
{
    float value;
    bool ok;

    struct Data {
        const char *text;
        float value;
    };

    const struct Data good_data[] = {
        {   "1234",         1234.0f },
        {   "-1234",        -1234.0f },
        {   "0",            0.0f },
        {   "0.0",          0.0f },
        {   "-0.0",         0.0f },
        {   "0.123456",     0.123456f },
        {   "0.000001",     0.000001f },
        {   "1.2e2",        120.0f },
        {   "-1.2e2",       -120.0f },
        {   "1.2e-5",       0.000012f },
        {   "0x0",          0.0f }, // Hex works
        {   "0x100",        256.0f },
        {   0,              0 },
    };

    for (const struct Data *d = good_data; d->text; d++)
    {
        //LOG_DEBUG("%s", d->text);
        ok = cli_parse_float(d->text, & value);
        EXPECT_TRUE(ok);
        EXPECT_EQ(d->value, value);
    }

    const struct Data bad_data[] = {
        {   "123x", },
        {   "a0", },
        {   "0 0", },
        {   "1.2f", },
        {   "--1", },
        {   0,      },
    };

    for (const struct Data *d = bad_data; d->text; d++)
    {
        //LOG_DEBUG("%s", d->text);
        ok = cli_parse_float(d->text, & value);
        EXPECT_FALSE(ok);
    }
}

    /*
     *
     */

static void echo_cmd(CLI *cli, CliCommand *cmd)
{
    cli_print(cli, "%s", cmd->cmd);

    for (int i = 0; i < CLI_MAX_ARGS; i++)
    {
        const char *s = cli_get_arg(cli, i);
        if (!s)
        {
            break;
        }

        cli_print(cli, " %s", s);
    }

    cli_print(cli, "%s", cli->eol);
}

TEST(CLI, Echo)
{
    io.reset();

    CliCommand hello = {
        .cmd = "hello",
        .handler = echo_cmd,
    };

    cli_init(& cli, 64, 0);
    cli_register(& cli, & hello);

    // Normal case (echo enabled)

    // initial prompt
    EXPECT_STREQ("> ", io.get());
    io.reset();

    cli_send(& cli, "hello world");
    EXPECT_STREQ("hello world", io.get());
    io.reset();
 
    cli_send(& cli, "\n");
    EXPECT_STREQ("\nhello world\r\n> ", io.get());
    io.reset();

    // test with echo off 
    cli.echo = false;

    cli_send(& cli, "hello world");
    EXPECT_STREQ("", io.get());
    io.reset();

    // backspace over " world"
    cli_send(& cli, "\b\b\b\b\b\b");
    EXPECT_STREQ("", io.get());
    io.reset();

    cli_send(& cli, "\n");
    EXPECT_STREQ("hello\r\n> ", io.get());
    io.reset();

    cli_send(& cli, "hello world");
    EXPECT_STREQ("", io.get());
    io.reset();

    cli_send(& cli, " more args\n");
    EXPECT_STREQ("hello world more args\r\n> ", io.get());
    io.reset();

    cli_close(& cli);
}

//  FIN
