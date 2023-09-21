/* m65.c
 * Decodes a tokenized MAC/65 file from stdin to stdout.
 * (c) 2009-2023 Daniel Serpell.
 */

#include "tokens.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int at_eol = 0; // Use ATASCII EOL
int cv_rem = 0; // Convert comments to ASCII
int cv_str = 0; // Convert strings

unsigned char *getm65line(FILE *f)
{
    int c1, c2, l, i;
    unsigned char *line;
    if( feof(f) )
        return 0;
    if( (c1 = getc(f)) == EOF )
        return 0;
    if( (c2 = getc(f)) == EOF )
        return 0;
    if( (l = getc(f)) == EOF )
        return 0;
    if( l < 3 )
        return 0;
    line    = malloc(l);
    line[0] = c1;
    line[1] = c2;
    line[2] = l;
    for( i = 3; i < l; i++ )
    {
        int c = getc(f);
        if( c == EOF )
        {
            free(line);
            return 0;
        }
        line[i] = c;
    }
    return line;
}

int tab1, tab2;

int m65_comment(unsigned char *ld, unsigned char *end)
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

int m65_conv_str(unsigned char *ld, unsigned char *end)
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
            if( i - sub == 1 && ld[sub] != '\'' && ld[sub] != ' ')
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

int m65line(FILE *f)
{
    unsigned char *ld, *end;
    int in_quote = 0;
    int line;
    int xp = 0;
    ld     = getm65line(f);
    if( !ld )
        return 0;

    line = ld[0] + ld[1] * 256;
    end  = ld + ld[2];
    ld += 3;

    if( line < 100 )
        xp += printf("%02d ", line);
    else if( line < 10000 )
        xp += printf("%04d ", line);
    else
        xp += printf("%06d ", line);

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
            break;
        }

        if( cmd == 1 ) // IF indent
        {
            tab1 += 2;
            tab2 += 2;
        }

        // First TAB after label
        do
        {
            putchar(' ');
            xp++;
        }
        while( xp <= tab1 );

        // Print statement
        if( cmd >= 1 && cmd < 96 )
        {
            xp += printf("%s", toks[cmd]);
        }
        else
        {
            xp += printf(" {ERR:%d} ", cmd);
        }

        if( cmd == 7 )
        {
            // Macro: special case and put before tab
            for( int fn = 0x7F & *ld++; ld < end && fn > 0; ld++, fn--, xp++ )
                putchar(*ld);
        }

        // Second TAB after statement
        do
        {
            putchar(' ');
            xp++;
        }
        while( xp <= tab2 );

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
                do
                {
                    putchar(' ');
                    xp++;
                }
                while( xp <= 20 );

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
    return xp;
}

void printfile(FILE *f)
{
    int c1, c2;
    c1 = getc(f);
    c2 = getc(f);
    if( c1 != 254 || c2 != 254 )
    {
        printf("Not MAC/65 file\n");
        return;
    }
    c1 = getc(f);
    c2 = getc(f);
    if( c1 == EOF || c2 == EOF )
    {
        printf("Short file!\n");
        return;
    }
    tab1 = 8;
    tab2 = 12;
    while( m65line(f) )
        ;
}

int main(int argc, char **argv)
{
    int opt;
    while( (opt = getopt(argc, argv, "hacs")) != -1 )
    {
        switch( opt )
        {
        case 'a':
            at_eol = 1;
            break;
        case 'c':
            cv_rem = 1;
            break;
        case 's':
            cv_str = 1;
            break;
        case 'h':
            fprintf(stderr,
                    "Usage: %s [options] [file] [... file]\n"
                    "Options:\n"
                    "\t-a  Use ATASCII line endings.\n"
                    "\t-c  Convert comments to ASCII.\n"
                    "\t-s  Convert strings with non-printable chars to hex.\n"
                    "\t-h  Show this help.\n",
                    argv[0]);
            exit(EXIT_SUCCESS);
        default:
            fprintf(stderr, "%s: try '-h' for help.\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if( optind >= argc )
        printfile(stdin);
    else
    {
        while( optind < argc )
        {
            FILE *infile = fopen(argv[optind], "rb");
            if( !infile )
            {
                fprintf(stderr, "%s: can't open file\n", argv[optind]);
                exit(EXIT_FAILURE);
            }
            printfile(infile);
            fclose(infile);
            optind++;
        }
    }

    return 0;
}
