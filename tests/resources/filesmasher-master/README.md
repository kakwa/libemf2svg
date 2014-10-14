File Smasher
============

Description
-----------

This program can be used to randomly sprinkle random bytes onto a
file.  It supports large 64-bit file sizes for files over 2GiB.

Usage
-----

`filesmasher <filename> <amount> [range]`

The program will open the existing file for writing to do its thing,
so be sure to have a backup.

`<amount>` specifies the number of bytes to sprinkle into the file.

`[range]` specifies where in the file the program should touch.  The
format is simple: comma-separated list of start-end or single
positional entries.  For example: `15-63,100-135,75,77,80,1024-4095`

Ranges start at byte position 0.  Ranges can be given in any order,
and no more attention is given to any one range or value.  Ranges
cannot overlap, but they can be adjacent.  A range must be start-end
value order, and may not extend past the end of the file.
