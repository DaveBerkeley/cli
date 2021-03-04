
#include <stdlib.h>
#include <stdarg.h>

#include <gtest/gtest.h>

#include "sprintf.h"

    /*
     *  Test reading of "%" data
     */

static void _test(Format *f, const char** fmt, va_list va)
{
    f->get(fmt, va);
}

static void test(Format *f, const char** fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    _test(f, fmt, va);
    va_end(va);
}

TEST(Printf, Fmt)
{
    {
        Format format;

        const char* fmt = "%s";
        test(& format, & fmt);
        EXPECT_EQ('\0', *fmt);
        EXPECT_EQ('\0', format.flags);
        EXPECT_EQ(0, format.width);
        EXPECT_EQ(0, format.precision);
        EXPECT_STREQ("", format.length);
        EXPECT_EQ('s', format.specifier);
    }
    {
        Format format;

        const char* fmt = "%-3d";
        test(& format, & fmt);
        EXPECT_EQ('\0', *fmt);
        EXPECT_EQ('-', format.flags);
        EXPECT_EQ(3, format.width);
        EXPECT_EQ(0, format.precision);
        EXPECT_STREQ("", format.length);
        EXPECT_EQ('d', format.specifier);
    }
    {
        Format format;

        const char* fmt = "%+15.6f";
        test(& format, & fmt);
        EXPECT_EQ('\0', *fmt);
        EXPECT_EQ('+', format.flags);
        EXPECT_EQ(15, format.width);
        EXPECT_EQ(6, format.precision);
        EXPECT_STREQ("", format.length);
        EXPECT_EQ('f', format.specifier);
    }
    {
        Format format;

        const char* fmt = "%+15.6f";
        test(& format, & fmt);
        EXPECT_EQ('\0', *fmt);
        EXPECT_EQ('+', format.flags);
        EXPECT_EQ(15, format.width);
        EXPECT_EQ(6, format.precision);
        EXPECT_STREQ("", format.length);
        EXPECT_EQ('f', format.specifier);
    }
    {
        Format format;

        const char* fmt = "%lld";
        test(& format, & fmt);
        EXPECT_EQ('\0', *fmt);
        EXPECT_EQ('\0', format.flags);
        EXPECT_EQ(0, format.width);
        EXPECT_EQ(0, format.precision);
        EXPECT_STREQ("ll", format.length);
        EXPECT_EQ('d', format.specifier);
    }
    {
        Format format;

        const char* fmt = "%*d";
        test(& format, & fmt, 20);
        EXPECT_EQ('\0', *fmt);
        EXPECT_EQ('\0', format.flags);
        EXPECT_EQ(20, format.width);
        EXPECT_EQ(0, format.precision);
        EXPECT_STREQ("", format.length);
        EXPECT_EQ('d', format.specifier);
    }
}

    /*
     *
     */

class TestOutput : public Output
{
public:
    char buff[1024];
    size_t idx;

    TestOutput()
    : idx(0)
    {
        buff[0] = '\0';
    }

    void reset()
    {
        idx = 0;
    }

    virtual int _putc(char c)
    {
        EXPECT_TRUE(idx < sizeof(buff));
        buff[idx++] = c;
        if (idx < sizeof(buff))
        {
            buff[idx] = '\0';
        }
        return 1;
    }
};

    /*
     *
     */

TEST(Printf, Num)
{
    // int _print_num(Output *output, const Format *format, int number)

    {
        Format format;
        const char *fmt = "%d";
        test(& format, & fmt);

        TestOutput out;
        _print_num(& out, & format, 1234, 10, false);
        EXPECT_STREQ("1234", out.buff);
    }
    {
        Format format;
        const char *fmt = "%+d";
        test(& format, & fmt);

        TestOutput out;
        _print_num(& out, & format, 1234, 10, false);
        EXPECT_STREQ("+1234", out.buff);
    }
    {
        Format format;
        const char *fmt = "%+d";
        test(& format, & fmt);

        TestOutput out;
        _print_num(& out, & format, 1234, 10, true);
        EXPECT_STREQ("-1234", out.buff);
    }
    {
        Format format;
        const char *fmt = "%08d";
        test(& format, & fmt);

        TestOutput out;
        _print_num(& out, & format, 1234, 10, false);
        EXPECT_STREQ("00001234", out.buff);
    }
    {
        Format format;
        const char *fmt = "%+08d";
        test(& format, & fmt);

        TestOutput out;
        _print_num(& out, & format, 1234, 10, false);
        EXPECT_STREQ("+0001234", out.buff);
    }
    {
        Format format;
        const char *fmt = "%08x";
        test(& format, & fmt);

        TestOutput out;
        _print_num(& out, & format, 0xdeadface, 16, false);
        EXPECT_STREQ("deadface", out.buff);
    }
}

    /*
     *
     */

TEST(Printf, Test)
{
    // strings
    {
        // plain text : no formatting
        TestOutput out;
        out.printf("hello world\r\n");
        EXPECT_STREQ("hello world\r\n", out.buff);
    }
    {
        TestOutput out;
        out.printf("hello %s end", "dave");
        EXPECT_STREQ("hello dave end", out.buff);
    }
    {
        TestOutput out;
        out.printf("'%10s'", "string");
        EXPECT_STREQ("'    string'", out.buff);
    }
    {
        TestOutput out;
        out.printf("'%4s'", "string");
        EXPECT_STREQ("'string'", out.buff);
    }
    {
        TestOutput out;
        out.printf("'%-10s'", "string");
        EXPECT_STREQ("'string    '", out.buff);
    }

    // char
    {
        TestOutput out;
        out.printf("hello %c end", 'X');
        EXPECT_STREQ("hello X end", out.buff);
    }

    // integers
    {
        TestOutput out;
        out.printf("hello %d end", 1234);
        EXPECT_STREQ("hello 1234 end", out.buff);
    }

    // -ve integers
    {
        TestOutput out;
        out.printf("hello %d end", -1234);
        EXPECT_STREQ("hello -1234 end", out.buff);
    }

    // zero integers
    {
        TestOutput out;
        out.printf("hello %+d end", 0);
        EXPECT_STREQ("hello 0 end", out.buff);
    }

    // +ve integers
    {
        TestOutput out;
        out.printf("hello %+d end", 1);
        EXPECT_STREQ("hello +1 end", out.buff);
    }

    // unsigned integers
    {
        TestOutput out;
        out.printf("hello %u end", 0xffffffff);        
        EXPECT_STREQ("hello 4294967295 end", out.buff);
    }

    // hex
    {
        TestOutput out;
        out.printf("hello %x end", 0x1234);
        EXPECT_STREQ("hello 1234 end", out.buff);
    }

    // hex
    {
        TestOutput out;
        out.printf("hello %#x end", 0x1234);
        EXPECT_STREQ("hello 0x1234 end", out.buff);
    }

    // pointers    
    {
        TestOutput out;
        out.printf("hello %p end", 0x80001234);
        EXPECT_STREQ("hello 0x80001234 end", out.buff);
    }
}

TEST(Printf, Float)
{
    {
        const char *fmt = "%f";

        TestOutput out;
        xprintf(& out, fmt, 1.234);
        EXPECT_STREQ("1.234000", out.buff);
    }
    {
        const char *fmt = "%.2f";

        TestOutput out;
        xprintf(& out, fmt, 1.234);
        EXPECT_STREQ("1.23", out.buff);
    }
    {
        const char *fmt = "%.8f";

        TestOutput out;
        xprintf(& out, fmt, 1.234);
        EXPECT_STREQ("1.23400000", out.buff);
    }
    {
        const char *fmt = "%.8f";

        TestOutput out;
        xprintf(& out, fmt, 1.234e-5);
        EXPECT_STREQ("0.00001234", out.buff);
    }
}

//  FIN