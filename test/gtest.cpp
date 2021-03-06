
#include <gtest/gtest.h>

#include "debug.h"
#include "list.h"
#include "cli.h"
#include "io.h"

    /*
     *
     */

class IO
{
    Output memio = 0;
    char *mem_buf = 0;
    size_t mem_size = 0;

public:

    Output open()
    {
        memio = open_memstream(& mem_buf, & mem_size);
        return memio;
    }

    void close()
    {
        fclose(memio);
        free(mem_buf);
    }

    void reset()
    {
        // reset to start of buffer
        fseek(memio, 0, SEEK_SET);
        // ensure the buffer is '\0' terminated
        fwrite("\0", 1, 1, memio);
        fseek(memio, 0, SEEK_SET);
    }

    char* get()
    {
        // Ensures that all data is written to buffer
        // terminate it with a '\0' after the last char
        fwrite("\0", 1, 1, memio);
        fseek(memio, -1, SEEK_CUR);
        fflush(memio);
        return mem_buf;
    }
};

static IO io;

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

static CLI cli = {
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

TEST(CliGroup, Create)
{
    static CliCommand action = {
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

TEST(CliGroup, Close)
{
    static CliCommand action = {
        .cmd = "help",
        .handler = action_handler,
        .help = "help text",
    };
    static CliCommand more = {
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
    EXPECT_EQ(action.next, (void*)0);
    EXPECT_EQ(more.next, (void*)0);
}

static void cli_die(CLI *cli, CliCommand *cmd)
{
    UNUSED(cli);
    UNUSED(cmd);
    ASSERT(false);
}

TEST(CliGroup, Help)
{
#define HELP0 "one line of text" 
#define HELP1 "another help line"
#define HELP2 "some more help" 

    static CliCommand a0 = {
        .cmd = "help",
        .handler = cli_help,
        .help = HELP0,
    };
    static CliCommand a1 = {
        .cmd = "anything",
        .handler = cli_die,
        .help = HELP1,
    };
    static CliCommand a2 = {
        .cmd = "another",
        .handler = cli_die,
        .help = HELP2,
    };

    cli_init(& cli, 64, 0);
    cli_register(& cli, & a0);
    cli_register(& cli, & a1);
    cli_register(& cli, & a2);

    io.reset();
    cli_send(& cli, "help\r\n");

    // Buffer should be cleared
    EXPECT_STREQ("", cli.buff);

    // Check the output
    EXPECT_STREQ("help\r\n" "help : " HELP0 "\r\n" "anything : " HELP1 "\r\n" "another : " HELP2 "\r\n> ", io.get());

    cli_close(& cli);
}

TEST(CliGroup, HelpSub)
{
    static CliCommand a0 = {
        .cmd = "help",
        .handler = cli_help,
        .help = HELP0,
    };
    static CliCommand a1 = {
        .cmd = "anything",
        .handler = cli_die,
        .help = HELP1,
    };

    cli_init(& cli, 64, 0);
    cli_register(& cli, & a0);
    cli_register(& cli, & a1);

    // Check help <subcommand>
    io.reset();
    cli_send(& cli, "help anything\r\n");

    // Buffer should be cleared
    EXPECT_STREQ("", cli.buff);

    // Check the output
    EXPECT_STREQ("help anything\r\n" "anything : " HELP1 "\r\n> ", io.get());

    // Check help <unknown>
    io.reset();
    cli_send(& cli, "help nowt\r\n");

    // Buffer should be cleared
    EXPECT_STREQ("", cli.buff);

    // Check the output
    EXPECT_STREQ("help nowt\r\n" "'nowt' not found\r\n> ", io.get());

    cli_close(& cli);
}

TEST(CliGroup, Edit)
{
    static CliCommand a0 = {
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

    cli_send(& cli, "\b");
    EXPECT_STREQ("held", cli.buff);
    EXPECT_STREQ("heldx\b \b", io.get());

    cli_send(& cli, "\b");
    EXPECT_STREQ("hel", cli.buff);
    EXPECT_STREQ("heldx\b \b\b \b", io.get());

    cli_send(& cli, "p\r\n");
    // Buffer should be cleared
    EXPECT_STREQ("", cli.buff);

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

TEST(CliGroup, Context)
{
    static CliCommand a0 = {
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

TEST(CliGroup, EmptyLine)
{
    static CliCommand a0 = {
        .cmd = "help",
        .handler = cli_help,
        .help = "help!",
    };

    cli_init(& cli, 64, 0);
    cli_register(& cli, & a0);

    cli_send(& cli, "\r\n");

    // Buffer should be cleared
    EXPECT_STREQ("", cli.buff);

    cli_close(& cli);
}

TEST(CliGroup, OverflowLine)
{
    static CliCommand a0 = {
        .cmd = "help",
        .handler = cli_help,
        .help = "help!",
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
    struct Device *next;
}   Device;

static pList *next_dev(pList item)
{
    Device *dev = (Device*) item;
    return (pList*) & dev->next;
}

static int laser_value;
bool set_laser(int v) { laser_value = v; return true; }
int get_laser() { return laser_value; }
bool power_laser(bool on) { UNUSED(on); return true; }

static Device laser = { "laser", set_laser, get_laser, 0 };

static pList devices = 0;

static int dev_visit(pList w, void *arg)
{
    Device *dev = (Device*) w;
    CLI *cli = (CLI*) arg;

    // Print the device name
    cli_print(cli, "%s%s", dev->name, cli->eol);
    return 0;
}

static int dev_match(pList w, void *arg)
{
    Device *dev = (Device*) w;
    const char *s = (const char*) arg;
    return strcmp(s, dev->name) == 0;
}

void power(CLI *cli, CliCommand *cmd)
{
    UNUSED(cmd);

    // Is there a subcommand?
    const char *s = strtok_r(0, " ", & cli->strtok_save);
    LOG_DEBUG("'%s'", s);

    if (!strcmp(s, "?"))
    {
        //  List the devices
        list_visit(& devices, next_dev, dev_visit, cli, 0);
        return;
    }

    //  Expecting the device name

    Device *dev = (Device*) list_find(& devices, next_dev, dev_match, (void*) s, 0);
    if (!dev)
    {
        //  Not found
        LOG_DEBUG("not found %s", s);
        return;
    }

    LOG_DEBUG("found %s", s);

    s = strtok_r(0, " ", & cli->strtok_save);
    LOG_DEBUG("cmd=%s", s);

    if (!s)
    {
        // No command found
        LOG_DEBUG("no command");
        return;
    }

    LOG_DEBUG("power %s %s", dev->name, s);

    if (!strcmp(s, "?"))
    {
        // Query the device
        int v = dev->get();
        cli_print(cli, "%d%s", v, cli->eol);
        return;
    }

    // Set the device to the passed integer value
    char *end = 0;
    const long int v = strtol(s, & end, 10);
    if (*end != '\0')
    {
        // Number not fully converted, so an error
        LOG_DEBUG("error in value '%s'", s);
        return;
    }

    ASSERT(dev->set);
    const bool okay = dev->set((int) v);
    cli_print(cli, "%s%s", okay ? "ok" : "error", cli->eol);
}

    /*
     *
     */

TEST(CliGroup, Power)
{
    list_push(& devices, (pList) & laser, next_dev, 0);

    static CliCommand a0 = {
        .cmd = "power",
        .handler = power,
        .help = "power <device> <on>|<off>|?\npower ?",
    };

    cli_init(& cli, 64, 0);
    cli_register(& cli, & a0);

    // Query the devices
    io.reset();
    cli_send(& cli, "power ?\r\n");

    EXPECT_STREQ("", cli.buff);
    EXPECT_STREQ("power ?\r\nlaser\r\n> ", io.get());

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
    const char *s = strtok_r(0, " ", & cli->strtok_save);
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

TEST(CliGroup, Input)
{
    static CliCommand a0 = {
        .cmd = "hello",
        .handler = hello,
        .help = "hello",
    };
    static CliCommand a1 = {
        .cmd = "bye",
        .handler = bye,
        .help = "bye",
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
 
int main(int argc, char **argv) {
    log_open();

    cli.output = io.open();

    testing::InitGoogleTest(&argc, argv);
    const int result = RUN_ALL_TESTS();

    io.close();
    log_close();
    return result;
}

