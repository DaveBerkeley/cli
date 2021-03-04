
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "debug.h"

#include "sprintf.h"

int Output::_puts(const char* s, int n)
{
    int count = 0;

    for (int i = 0; i < n; i++)
    {
        count += _putc(*s++);
    }

    return count;
}

// see http://www.cplusplus.com/reference/cstdio/printf/
//
// %[flags][width][.precision][length]specifier

Format::Format()
: flags(0), leading(0), width(0), precision(0), specifier(0)
{
    memset(length, 0, sizeof(length));
}

    /*
     *  get a +ve decimal number, or fetch '*' data from va_args
     */

int Format::get_num(const char **fmt, va_list va)
{
    // expects "*" or a number to be parsed

    const char *f = *fmt;
    int count = 0;
    int result = 0;

    if (*f == '*')
    {
        // fetch the '*' from the va list
        result = va_arg(va, int);
        count += 1;
    }
    else
    {
        // read the digit
        while (isdigit(*f))
        {
            result *= 10;
            const int digit = *f - '0';
            result += digit;
            f += 1;
            count += 1;
        }
    }

    *fmt += count;
    return result;
}

    /*
     *  Read the '%' format data
     */

void Format::get(const char **fmt, va_list va)
{
    // skip the '%' symbol
    ASSERT(**fmt == '%');
    *fmt += 1;

    // check for flags
    if (strchr("-+#", **fmt))
    {
        flags = **fmt;
        *fmt += 1;
    }
    if (strchr(" 0", **fmt))
    {
        leading = **fmt;
        *fmt += 1;
    }

    // check width
    width = get_num(fmt, va);

    // check precision
    if (**fmt == '.')
    {
        *fmt += 1;
        precision = get_num(fmt, va);
    }

    // check length
    if ((**fmt == 'l') || (**fmt == 'h'))
    {
        length[0] = **fmt;
        *fmt += 1;

        if (**fmt == length[0])
        {
            length[1] = **fmt;
            *fmt += 1;
        }
    }

    // check specifier
    if (strchr("sdufxpc", **fmt))
    {
        specifier = **fmt;
        *fmt += 1;
    }
}

    /*
     *
     */

static int __print_num(Output *output, unsigned long int number, int base)
{
    int count = 0;
    const int rem = (int) (number % base);
    const unsigned long int div = number / base;

    if (div)
    {
        // print the number recursively, div first, to reverse the order.
        count += __print_num(output, div, base);
    }
 
    if (rem > 9)
    {
        count += output->_putc((char) ('a' + rem - 10));
    }
    else
    {
        count += output->_putc((char) ('0' + rem));
    }

    return count;
}

    /*
     *  Return number of digits required to print number
     */

static int digits(unsigned long int number, int base)
{
    // return the number of digits required to represent the number
    if (number == 0)
    {
        return 1;
    }

    int count = 0;
    while (number)
    {
        count += 1;
        number /= base;
    }

    return count;
}

    /*
     *  Pad with a char
     */

static int print_pad(Output *output, const char c, int pad)
{
    int count = 0;

    for (int i = 0; i < pad; i++)
    {
        count += output->_putc(c);
    }

    return count;
}

    /*
     *  Print a number
     */

int _print_num(Output *output, const Format *format, unsigned long int n, int base, bool negative)
{
    int count = 0;

    // print leading sign
    if (format->flags == '+')
    {
        if (n && !negative)
        {
            count += output->_putc('+');
        }
    }

    // print leading '-'
    if (negative)
    {
        count += output->_putc('-');
    }

    // TODO : precision field
    // TODO : length field

    // optionally pad with leading '0' or ' '
    if ((format->leading == '0') || (format->leading == ' '))
    {
        int d = digits(n, base);
    
        if ((format->flags == '-') || (format->flags == '+'))
        {
            // make space for the sign char
            d += 1;
        }

        const int pad = format->width - d;
        count += print_pad(output, format->leading, pad);
    }

    count += __print_num(output, n, base);

    return count;
}

    /**
     * @brief sprintf() style formatting
     *
     * @param output outputter
     * @param fmt the format string
     * @param va_list args
     *
     * @return number of chars output
     */

int xvprintf(Output *output, const char* fmt, va_list va)
{
    int count = 0;

    ASSERT(output);

    while (*fmt)
    {
        if (*fmt != '%')
        {
            count += output->_putc(*fmt++);
            continue;
        }

        // keep pointer to start of '%'
        const char *start = fmt;

        Format f;
        f.get(& fmt, va);

        switch (f.specifier)
        {
            case 'd' :
            {
                int i = va_arg(va, int);
                if (i < 0)
                {
                    count += _print_num(output, & f, -i, 10, true);
                }
                else
                {
                    count += _print_num(output, & f, i, 10, false);
                }
                break;
            }
            case 'u' :
            {
                unsigned int i = va_arg(va, unsigned int);
                count += _print_num(output, & f, i, 10, false);
                break;
            }
            case 'x' :
            {
                if (f.flags == '#')
                {
                    count += output->_puts("0x", 2);
                }
                int i = va_arg(va, int);
                count += _print_num(output, & f, i, 16, false);
                break;
            }
            case 'p' :
            {
                count += output->_puts("0x", 2);
                void* v = va_arg(va, void*);
                count += _print_num(output, & f, (long unsigned int) v, 16, false);
                break;
            }
            case 'c' :
            {
                int c = va_arg(va, int);
                count += output->_putc((char) c);
                break;
            }
            case 's' :
            {
                char *s = va_arg(va, char*);
                int pad = f.width - (int) strlen(s);

                if (f.flags != '-')
                {
                    print_pad(output, ' ', pad);
                }

                count += output->_puts(s, (int) strlen(s));

                if (f.flags == '-')
                {
                    print_pad(output, ' ', pad);
                }
                break;
            }
            case 'f' :
            {
                // I don't want to rewrite all the float stuff,
                // so just sprintf these
                char f_fmt[8] = { 0 };
                char buff[32];
                const size_t size = fmt - start;
                double f = va_arg(va, double);
                ASSERT(size < sizeof(f_fmt));
                strncpy(f_fmt, start, size);
                snprintf(buff, sizeof(buff), f_fmt, f);
                
                count += output->_puts(buff, (int) strlen(buff));
                break;
            }
            default :
            {
                // TODO : Add other formats, eg float.
                ASSERT(0);
            }
        }
    }
    return count;
}

/**
 * @brief sprintf() style formatting
 *
 * @param output outputter
 * @param fmt the format string
 * @param ... args
 *
 * @return number of chars output
 */

extern "C" int xprintf(Output *output, const char* fmt, ...)
{
    va_list va;

    va_start (va, fmt);
    const int c = xvprintf(output, fmt, va);
    va_end (va);
    return c;
}

    /*
     *
     */

int Output::printf(const char* fmt, ...)
{
    Lock lock(mutex);
    va_list va;

    va_start (va, fmt);
    const int c = xvprintf(this, fmt, va);
    va_end (va);
    return c;
}

//  FIN
