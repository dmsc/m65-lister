/* m65.c
 * Decodes a tokenized MAC/65 file from stdin to stdout.
 * (c) 2009-2023 Daniel Serpell.
 */

#include "tokens.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int at_eol = 0;  // Use ATASCII EOL
static int cv_rem = 0;  // Convert comments to ASCII
static int cv_str = 0;  // Convert strings
static int do_num = 1;  // Show line numbers
static int tab_p1 = 8;  // Column of first tab - before instruction
static int tab_p2 = 12; // Column of second tab - before argument
static int tab_p3 = 20; // Column of third tab - before comment

// Current tab positions - modified by IF/ENDIF
static int tab1, tab2;
// Current file name
static const char *fname;
// If any file gave an error
static int do_error = 0;

static unsigned char *getm65line(FILE *f)
{
    int c1, c2, l, i;
    unsigned char *line;
    if( feof(f) )
        return 0;
    if( (c1 = getc(f)) == EOF )
        return 0;
    if( (c2 = getc(f)) == EOF || (l = getc(f)) == EOF )
    {
        fprintf(stderr, "%s: truncated file.\n", fname);
        do_error = 1;
        return 0;
    }
    if( l < 3 )
    {
        fprintf(stderr, "%s: line too short.\n", fname);
        do_error = 1;
        return 0;
    }
    line    = malloc(l);
    if( !line )
    {
        fprintf(stderr, "%s: memory error.\n", fname);
        do_error = 1;
        return 0;
    }
    line[0] = c1;
    line[1] = c2;
    line[2] = l;
    for( i = 3; i < l; i++ )
    {
        int c = getc(f);
        if( c == EOF )
        {
            fprintf(stderr, "%s: truncated line %d.\n", fname, c1 + c2 * 256);
            do_error = 1;
            free(line);
            return 0;
        }
        line[i] = c;
    }
    return line;
}

static int m65_comment(unsigned char *ld, unsigned char *end)
{
    // Comment, print until end of line
    int len = (end - ld);
    while( ld < end )
    {
        int c = (*ld++) & 0xFF;
        if( cv_rem )
        {
            c &= 0x7F;
            if( c < ' ' || c == 0x7F )
                c = '.';
        }
        putchar(c);
    }
    return len;
}

static int m65_conv_str(unsigned char *ld, unsigned char *end)
{
    int clen = 0;
    int i    = 1;
    int last = 0;
    int len  = 1 + (*ld & 0x7F);
    // We are just after a quote - check if we have the full string
    if( (*ld & 0x80) == 0 )
        return -1;
    if( ld + len >= end )
        return -1;
    if( ld[len] != 65 )
        return -1;

    // Ok, convert:
    while( i < len )
    {
        int sub = i;
        // Extract printable characters
        while( i < len && ld[i] >= ' ' && ld[i] < 0x7F && ld[i] != '"' )
            i++;
        if( i - sub )
        {
            if( last )
                clen += printf(",");
            if( i - sub == 1 && ld[sub] != '\'' && ld[sub] != ' ' )
                clen += printf("'%c", ld[sub]);
            else
                clen += printf("\"%.*s\"", i - sub, ld + sub);
            last = 1;
        }
        // Now, extract non-printable characters
        while( i < len && (ld[i] < ' ' || ld[i] >= 0x7F || ld[i] == '"') )
        {
            if( last )
                clen += printf(",");
            if( ld[i] == '"' )
                clen += printf("'%c", ld[i]);
            else if( ld[i] & 0x80 )
                clen += printf("$%02X", ld[i]);
            else
                clen += printf("%d", ld[i]);
            last = 1;
            i++;
        }
    }
    return clen;
}

static int put_tab(int xp, int tpos)
{
    do
    {
        putchar(' ');
        xp++;
    } while( xp <= tpos );
    return xp;
}

static int m65line(FILE *f)
{
    unsigned char *ld, *end;
    int in_quote = 0;
    int line;
    int xp = 0;
    unsigned char *data = getm65line(f);
    if( !data )
        return 0;

    ld   = data;
    line = ld[0] + ld[1] * 256;
    end  = ld + ld[2];
    ld += 3;

    if( do_num )
    {
        if( line < 100 )
            xp += printf("%02d ", line);
        else if( line < 10000 )
            xp += printf("%04d ", line);
        else
            xp += printf("%06d ", line);
    }

    while( ld < end )
    {
        int cmd = *ld;
        ld++;

        // Process command
        if( cmd & 0x80 )
        {
            // get label
            for( cmd &= 0x7f; ld < end && cmd > 0; ld++, cmd--, xp++ )
                putchar(*ld);
            // Get new command
            continue;
        }
        else if( cmd == 88 || cmd == 0 )
        {
            xp += m65_comment(ld, end);
            ld = end;
            // End line
            continue;
        }

        if( cmd == 1 ) // IF indent
        {
            tab1 += 2;
            tab2 += 2;
        }

        // First TAB after label
        xp = put_tab(xp, tab1);

        // Print statement
        if( cmd >= 1 && cmd < 96 )
        {
            xp += printf("%s", toks[cmd]);
        }
        else
        {
            xp += printf(" {ERR:%d} ", cmd);
            fprintf(stderr, "%s: unknown token at line %d\n", fname, line);
            do_error = 1;
        }

        if( cmd == 7 )
        {
            // Macro: special case and put before tab
            if( ld < end )
                for( int fn = 0x7F & *ld++; ld < end && fn > 0; ld++, fn--, xp++ )
                    putchar(*ld);
        }

        // Second TAB after statement
        xp = put_tab(xp, tab2);

        // Get arguments
        while( ld < end )
        {
            int fn = *ld;
            ld++;
            if( fn & 0x80 )
            {
                // literal value
                for( fn &= 0x7f; ld < end && fn > 0; ld++, fn--, xp++ )
                    putchar(*ld);
            }
            else if( fn == 5 )
            {
                int k = 0;
                if( ld < end )
                {
                    k = *ld;
                    ld++;
                }
                if( ld < end )
                {
                    k += 256 * (*ld);
                    ld++;
                }
                xp += printf("$%04X", k);
            }
            else if( fn == 6 )
            {
                int k = 0;
                if( ld < end )
                {
                    k = *ld;
                    ld++;
                }
                xp += printf("$%02X", k);
            }
            else if( fn == 7 )
            {
                int k = 0;
                if( ld < end )
                {
                    k = *ld;
                    ld++;
                }
                if( ld < end )
                {
                    k += 256 * (*ld);
                    ld++;
                }
                xp += printf("%d", k);
            }
            else if( fn == 8 )
            {
                int k = 0;
                if( ld < end )
                {
                    k = *ld;
                    ld++;
                }
                xp += printf("%d", k);
            }
            else if( fn == 10 )
            {
                int k = 0;
                if( ld < end )
                {
                    k = *ld;
                    ld++;
                }
                if( cv_str && (k <= ' ' || k == '\'' || k >= 0x7F) )
                    xp += printf("$%02X", k);
                else
                    xp += printf("'%c", k);
            }
            else if( fn == 59 )
            {
                xp = put_tab(xp, tab_p3);

                // Print until end of line
                xp += m65_comment(ld, end);
                ld = end;
            }
            else if( fn == 65 )
            {
                if( cv_str && !in_quote )
                {
                    // Quote
                    int l = m65_conv_str(ld, end);
                    if( l > 0 )
                    {
                        xp += l;
                        ld += 2 + (ld[0] & 0x7F);
                    }
                    else
                    {
                        in_quote = 1;
                        xp += printf("\"");
                    }
                }
                else
                {
                    in_quote = 0;
                    xp += printf("\"");
                }
            }
            else if( fn > 10 && fn < 78 )
            {
                xp += printf("%s", funcs[fn - 10]);
            }
            else
            {
                xp += printf(" {err:%d} ", fn);
                fprintf(stderr, "%s: unknown token at line %d\n", fname, line);
                do_error = 1;
            }
        }
        if( cmd == 3 ) // ENDIF remove indent
        {
            tab1 -= 2;
            tab2 -= 2;
        }
    }
    if( at_eol )
        putchar(0x9B);
    else
        putchar('\n');
    free(data);
    return xp;
}

static void printfile(FILE *f)
{
    int c1, c2;
    c1 = getc(f);
    c2 = getc(f);
    if( c1 != 254 || c2 != 254 )
    {
        fprintf(stderr, "%s: not a MAC/65 file\n", fname);
        do_error = 1;
        return;
    }
    c1 = getc(f);
    c2 = getc(f);
    if( c1 == EOF || c2 == EOF )
    {
        fprintf(stderr, "%s: file too short\n", fname);
        do_error = 1;
        return;
    }
    tab1 = tab_p1;
    tab2 = tab_p2;
    while( m65line(f) )
        ;
}

int main(int argc, char **argv)
{
    int tabn = 0;
    int opt;
    while( (opt = getopt(argc, argv, "hacsnt:")) != -1 )
    {
        switch( opt )
        {
        case 'n':
            do_num = 0;
            break;
        case 'a':
            at_eol = 1;
            break;
        case 'c':
            cv_rem = 1;
            break;
        case 's':
            cv_str = 1;
            break;
        case 't':
        {
            const char *p = optarg;
            while( *p )
            {
                if( tabn > 2 )
                {
                    fprintf(stderr, "%s: more than 3 tab positions given: '%s'\n",
                            argv[0], p);
                    exit(EXIT_FAILURE);
                }
                const char *e = p;
                int t         = strtol(p, (char **)&e, 0);
                if( e == p || (*e != 0 && *e != ':') )
                {
                    fprintf(stderr, "%s: invalid argument to tab position: '%s'\n",
                            argv[0], e);
                    exit(EXIT_FAILURE);
                }
                else if( t < 0 || t > 256 )
                {
                    fprintf(stderr, "%s: tab position must be from 0 to 256, not '%d'\n",
                            argv[0], t);
                    exit(EXIT_FAILURE);
                }
                if( tabn == 0 )
                    tab_p1 = t;
                else if( tabn == 1 )
                    tab_p2 = t;
                else if( tabn == 2 )
                    tab_p3 = t;

                tabn++;
                if( *e )
                    e++;
                p = e;
            }
            break;
        }
        case 'h':
            fprintf(stderr,
                    "Usage: %s [options] [file] [... file]\n"
                    "Options:\n"
                    "\t-a        Use ATASCII line endings.\n"
                    "\t-c        Convert comments to ASCII.\n"
                    "\t-s        Convert strings with non-printable chars to hex.\n"
                    "\t-n        Don't print the line numbers.\n"
                    "\t-t num    Sets next TAB position to 'num'\n"
                    "\t-t a:b:c  Sets TAB positions 'a', 'b' and 'c'\n"
                    "\t-h        Show this help.\n",
                    argv[0]);
            exit(EXIT_SUCCESS);
        default:
            fprintf(stderr, "%s: try '-h' for help.\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if( optind >= argc )
    {
        fname = "(stdin)";
        printfile(stdin);
    }
    else
    {
        while( optind < argc )
        {
            fname        = argv[optind++];
            FILE *infile = fopen(fname, "rb");
            if( !infile )
            {
                fprintf(stderr, "%s: can't open file\n", fname);
                do_error = 1;
                continue;
            }
            printfile(infile);
            fclose(infile);
        }
    }

    if( do_error )
        return EXIT_FAILURE;
    return 0;
}
