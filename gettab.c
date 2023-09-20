#include <stdio.h>

int main()
{
    int i, j;
    for( i = 0; i < 0x103E; i++ )
        getchar();
    i = 0;
    j = 0;
    printf("char *toks[]={\n\"");
    while( 1 )
    {
        int c  = getchar();
        char k = c & 0x7F;
        if( c == EOF )
            break;
        if( k == '"' )
        {
            j++;
            putchar('\\');
        }
        if( k > 31 )
        {
            j++;
            printf("%c", k);
        }
        else if( k > 0 )
        {
            j += 4;
            printf("\\x%02x", k);
        }
        if( c & 0x80 )
        {
            putchar('"');
            putchar(',');
            while( j++ < 10 )
                putchar(' ');
            j = 0;
            printf("/* %2d */\n", i);
            i++;
            if( i > 95 )
                break;
            putchar('"');
        }
    }
    printf("};\nchar *funcs[]={\n\"");
    i = 10;
    j = 0;
    while( 1 )
    {
        int c  = getchar();
        char k = c & 0x7F;
        if( c == EOF )
            break;
        if( k == '"' || k == '\\' )
        {
            j++;
            putchar('\\');
        }
        if( k > 31 )
        {
            j++;
            printf("%c", k);
        }
        else if( k > 0 )
        {
            j += 4;
            printf("\\x%02x", k);
        }
        if( c & 0x80 )
        {
            putchar('"');
            putchar(',');
            while( j++ < 10 )
                putchar(' ');
            j = 0;
            printf("/* %2d */\n", i);
            i++;
            if( i > 77 )
                break;
            putchar('"');
        }
    }
    printf("};\n");
    return 0;
}
