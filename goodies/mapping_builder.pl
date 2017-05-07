#!/usr/bin/perl

use Font::TTF::Font;
use strict;

my $f = Font::TTF::Font->open($ARGV[0]);
my $c = $f->{'cmap'}->read->find_ms;
my $p = $f->{'post'}->read;
my @rev = $f->{'cmap'}->reverse;

my $num = $f->{'maxp'}->read->{'numGlyphs'};
my $name = $f->{'name'}->read->find_name(4);

my $MAX_GLYPH = 1500;

if ($MAX_GLYPH < $num) {
    print "ERROR, max number of glyphes exceeded";
    exit(1);
}

print "{\"$name\", $num, {\n";
for (my $i = 0; $i < $MAX_GLYPH; $i++)
{
    my ($pname) = $p->{'VAL'}[$i];
    my ($uid) = $rev[$i];
    if ($uid) {
        printf ("0x%04X,", $uid);
    } else {
        printf ("0x%04X,", 0);
    }
    if ($i % 10 == 9){
        printf("\n");
    } else {
        printf(" ");
    }

}
print "},},\n"
