Lists MAC/65 tokenized files
----------------------------

This is a simple tool to list (convert to text) a binary tokenized MAC/65 file.

The MAC/65 assembler is a popular macro assembler for the Atari 8-bit
computers.

Usage
-----

Command line:
```
  m65 [options] [file] [... file]
```

Options:

 - `-a`          Use ATASCII line endings, generating an Atari compatible listing.

 - `-c`          Convert comments from ATASCII to ASCII, converting
                 inverse-video characters and removing control characters.

 - `-s`          Convert strings with non-printable chars to hexadecimal, resulting
                 in an assembly listing with only printable characters giving
                 the same output.

 - `-n`          Don't print the line numbers.

 - `-t` _num_    Sets next TAB position to 'num', the tabs define the position of
                 the mnemonics (tab 1), the arguments (tab 2) and the comments
                 (tab 3)

 - `-t` _n1:n2:n3_  Sets TAB positions to the values _n1_, _n2_ and _n3_
                    respectively.

 - `-h`          Show a simple usage help.

With the `-a` option the output is *exactly* as the original MAC/65 assembler
output, including all the spacing.

To convert sources to a format suitable for other assemblers, it is recommended to use
the `-csnt 8:16:32` options, this will output a nice formatted source.

