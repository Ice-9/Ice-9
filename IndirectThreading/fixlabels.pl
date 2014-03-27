#!/usr/bin/env perl

use strict;
use warnings;
no warnings 'uninitialized';

use Getopt::Std;

my $do_vocabs = 1;

my $re_ident = qr![a-zA-Z0-9_\$]+!;
my $re_name = qr!$re_ident(?:::$re_ident)?!;
my $re_label = qr![a-zA-Z_][a-zA-Z0-9_]*!;
my $re_string = qr!"(?:\\.|.)*?"!;
my $re_char = qr!'(?:\\.|.)'!;
my $re_stacky = qr!\s+(?:[ICRE]:)?\(.*?\)!;         # if necessary, put a syntax into this to make it tight
my $re_comment = qr!\s+//.*!;

my (@code1, @code2, @dict);
my $codeTarget = \@code1;
my ($nextLabel, %labelOffset, %labelName);
my $word_type;
my %dict_link = ( root => "NULL", forth => "NULL" );         # all new entries should start with NULL
my $dict_ofs = 0;
my %CodeAddressWord;

our ($opt_s, $opt_b);
getopts('sb:');
my $nbytes = $opt_b ? $opt_b / 8 : 4;
# should have a separate option for the size of the string count value


my $dict_magic;
{ no warnings; $dict_magic = $nbytes == 4 ? '0x77777777' : '0x7777777777777777'; }


sub registerLabel($) {
    $labelName{$_[0]} = ++$nextLabel unless exists $labelName{$_[0]};
}

sub emitCode {
    push @$codeTarget, [$., $_[0]];
}

sub emitDict {
    push @dict, [$., $_[0]];
    ++$dict_ofs if $_[0];       # allow for blank lines
    ++$dict_ofs if $_[0] =~ /TARGET/;
}

my $oldlno = 0;
sub emitLine ($$) {
    my $lno = shift;
    local $_ = shift;
    s/\s*$//;

    if($lno != ++$oldlno && length) {
        print "#line $lno\n";
        $oldlno = $lno;
    }
    print "$_\n";
}

sub cleanChar ($) {
    return 32 if $_[0] eq " ";
    return "'\\''" if $_[0] eq "'";
    return "'\\\\'" if $_[0] eq "\\";
    return "'$_[0]'";
}


sub ordChar ($) {
    local $_ = shift;

    s/^'//;
    s/'$//;

    return ord $1 if /^(.)$/;

    if(m:^\\(.+)$:) {
        my $bit = $1;
        return 7        if $bit eq 'a';
        return 27       if $bit eq 'e';
        return 11       if $bit eq 'v';

        return ord "\b" if $bit eq 'b';
        return ord "\f" if $bit eq 'f';
        return ord "\n" if $bit eq 'n';
        return ord "\r" if $bit eq 'r';
        return ord "\t" if $bit eq 't';

        local $_ = $bit;
        return ord $1 & 0x1F if /^c(.)$/;
        return hex $1 & 0xFF if /^x([A-F0-9]+)$/i;
        return oct $1 & 0xFF if /^[0-7]+$/;
    }

    die "Crappy string given to ordChar: '$_'";
}   # ordChar


my $rc = 0;

sub validate ($) {
    if(! exists $CodeAddressWord{$_[0]}) {
        print STDERR "undefined word used at line $.: $_[0]\n";
        $rc = 1;
    }
}


sub fixIdent($) {
    local $_ = $_[0];
    s/::/\$\$/ if /^($re_ident)::($re_ident)$/o;
    return $_;
}


my ($vocab, $basename, $header, $name, $string, $stacky);
while(<>) {
    chomp;
    last if /^__END__$/;

    # allow multiline POD-style blocking, mostly for commenting out Work In Progress.
    # NOTE: my tags allow - in the words. POD doesn't...
    next if /^=\w+(-\w+)*\s*$/ .. /^=end\s*$/;

    s/\s*$//;
    s/^(\s*)//;
    my $indent = $1;

    if($word_type eq "SECONDARY") {
        # allow // comments by killing them off
        s:\s*//.*$::;
    }

    if(! $word_type) {
        # [ ANSI [ (YYYYe ...) ] ] spaces ( PRIMARY | SECONDARY ) [ spaces ( -noheader | -immediate ) ] NAME [ spaces "string" ] { spaces stack-spec } [ spaces comment ]
        # The optional (year) on the ANSI keyword allows distinction of the standard level of the word, if it's not in both 1994 and 2012.
        # The basic assumption currently is that it's in 1994 and 2012, but this code has not been completely 2012'd yet... original work was started against 1994.
        # The e is optional and can be used to show that it's in the extension word set of that standard. If using e, it's probably good to indicate both (1994 2012e)
        #
        if(/^(?:ANSI(?:\(\d{4}e?(?:\s+\d{4}e?)*\))?\s+)?(PRIMARY|SECONDARY)(?:\s+(-noheader|-immediate))?\s+($re_name)(?:\s+($re_string))?((?:$re_stacky)*)(?:$re_comment)?$/o) {
            emitDict "";        # nice spacing blank line

            $vocab = 'forth';
            ($word_type, $header, $name, $string, $stacky) = ($1, $2, $3, $4, $5);

            if($do_vocabs && $name =~ /^($re_ident)::(.*)/) {
                ($vocab, $basename) = ($1, $2);
            }
            else {
                $basename = $name;
            }

            $stacky =~ s/^\s+//;
            $stacky =~ s/\s+/ /g;
            $string =~ s/\\(.)/$1/g;        # remove the excess backslashery that was needed in the pattern

            if(length $string) {
                $string =~ s/^"//;
                $string =~ s/"$//;
            }
            else {
                $string = $basename;
            }

            $name = fixIdent $name;

            if($opt_s || $header ne -noheader) {
                my $i = $dict_ofs;

                # Prefix the name with an unfindable* space if it's a -noheader word but
                # required to put debugging headers.
                # *NOTE: could still be found by using the finders manually ....
                # OR: does the find routine deliberately ignore ones with leading blanks?
                # Anyway - it's only for debugging purposes!
                #
                $string = " $string" if $opt_s && $header eq -noheader;

                # construct the header string
                my $l = length($string);
                my $w = $l;
                $w .= "u | IMMEDIATE_BIT" if $header eq '-immediate';
                if(length $header) {
                    emitDict "Cell($w)        // $dict_ofs $word_type $header $name \"$string\"";
                }
                else {
                    emitDict "Cell($w)        // $dict_ofs $word_type $name \"$string\"";
                }

                # blank-pad the name string out to word-size
                $string .= ' ' x (($nbytes - $l % $nbytes) % $nbytes);

                while(length $string) {
                    if($nbytes == 2) {
                        emitDict sprintf "Cell(DICT_STRING(%s,%s))              // $dict_ofs",
                                          cleanChar substr($string, 0, 1),
                                          cleanChar substr($string, 1, 1);
                        $string = substr $string, 2;
                    }
                    else {
                        emitDict sprintf "Cell(DICT_STRING(%s,%s,%s,%s))        // $dict_ofs",
                                          cleanChar substr($string, 0, 1),
                                          cleanChar substr($string, 1, 1),
                                          cleanChar substr($string, 2, 1),
                                          cleanChar substr($string, 3, 1);
                        $string = substr $string, 4;
                    }
                }

                $dict_link{$vocab} //= "NULL";
                emitDict "Cell($dict_link{$vocab})          // $dict_ofs";
                $dict_link{$vocab} = "&Dictionary[$i]";
            }

            if($word_type eq "PRIMARY") {
                $CodeAddressWord{$name} = $dict_ofs;
                emitDict "GOTO_ADR(dwc_$name)           // $dict_ofs END -- $name";
                $stacky = "                 // $stacky" if length $stacky;
                emitCode "    dwc_$name: {$stacky";
            }

            if($word_type eq "SECONDARY") {
                $CodeAddressWord{$name} = $dict_ofs;
                $stacky = "   $stacky" if length $stacky;
                emitDict "GOTO_ADR(\$COLON)         // $dict_ofs = $name$stacky";
            }
            next;
        }

        if(/^DICTIONARY\s*$/) {
            # target of emitCode = 2nd array
            $codeTarget = \@code2;
            next;
        }
    }

    if($word_type && /^END(?:\s+--\s+($re_name))?$/) {
        my $ident = fixIdent $1;
        if(length($ident) && $ident ne $name) {
            print STDERR "ERROR: label incorrect terminator for $word_type $name: END -- $1\n";
            $rc = 1;
        }

        if($word_type eq "PRIMARY") {
            emitCode "        goto \$NEXT;";
            emitCode "    }";
        }
        if($word_type eq "SECONDARY") {
            emitDict "Cell(&Dictionary[${CodeAddressWord{'$$scolon'}}])             // $dict_ofs: \$\$scolon";
        }

        if($opt_s) {        # signatures in Dictionary to make it easier to dump words
            emitDict "Cell($dict_magic)";
            emitDict "Cell($dict_magic)";
            emitDict "Cell($dict_magic)";
        }

        $word_type = undef;
        # check all used labels are defined, and all defined labels are used
        foreach (keys %labelName) {
            if(! exists $labelOffset{$labelName{$_}}) {
                print STDERR "ERROR: label $_ not defined in $name\n";
                $rc = 1;
            }
        }

        %labelName = ();  # forget all labels
        next;
    }

    if($word_type eq "SECONDARY") {
        # these first items :CODE: and <LABEL>: must be on a line of their own
        if(/^:CODE:$/) {
            emitDict "Cell(&Dictionary[${CodeAddressWord{'$$literal'}}])            // $dict_ofs: \$\$literal";
            emitDict "Cell(GOTO_ADR(dwc_$name))                                     // literal value";
            emitDict "Cell(&Dictionary[${CodeAddressWord{'$ca_store'}}])            // $dict_ofs: \$ca_store";
            emitDict "Cell(&Dictionary[${CodeAddressWord{'$$scolon'}}])             // $dict_ofs: \$\$scolon";

            %labelName = ();  # forget all labels
            $word_type = 'PRIMARY';
            emitCode "    dwc_$name: {";
            next;
        }

        if(/^($re_label):$/) {
            my $lb = $1;
            registerLabel $lb;
            $labelOffset{$labelName{$lb}} = $dict_ofs;    # record offset on the label name
            next;
        }


        # the next items can be inline for forth-like compact code in the .b4cpp
        while(/\G\s*/gc) {
            if(/\GSCREAM\s+($re_string)/gc) {
                emitDict "Cell(&Dictionary[${CodeAddressWord{'$$literal'}}])        // $dict_ofs: scream";
                emitDict "Cell($1)";
                emitDict "Cell(&Dictionary[${CodeAddressWord{csz_type}}])           // $dict_ofs: Ctype";
                next;
            }

            if(/\Gdwc\((\S+)\)/gc) {
                my $ident = fixIdent $1;
                validate $ident;
                emitDict "Cell(&Dictionary[${CodeAddressWord{'$$literal'}}])        // $dict_ofs: \$\$literal";
                emitDict "Cell(GOTO_ADR(dwc_$ident))                                // literal value";
                next;
            }

            if(/\Gadr\((\S+)\)/gc) {
                my $ident = fixIdent $1;
                validate $ident;
                emitDict "Cell(&Dictionary[${CodeAddressWord{'$$literal'}}])        // $dict_ofs: \$\$literal";
                emitDict "Cell(CODE_ADDRESS($ident))                                // literal value";
                next;
            }

            if(/\Gliteral\((\S+)\)/gc) {
                my $ident = fixIdent $1;
                emitDict "Cell(&Dictionary[${CodeAddressWord{'$$literal'}}])        // $dict_ofs: \$\$literal";
                emitDict "Cell($ident)                                              // literal value";
                next;
            }

            if(/\G(jmpn?z?)\(($re_label)\)/gc) {
                registerLabel $2;
                emitDict "Cell(&Dictionary[${CodeAddressWord{\"\$\$$1\"}}]), TARGET L$labelName{$2} - $dict_ofs // $dict_ofs: \$\$$1";
                next;
            }

            if(/\G((?:_add_)?loop)\(($re_label)\)/gc) {
                registerLabel $2;
                emitDict "Cell(&Dictionary[${CodeAddressWord{\"\$\$$1\"}}]), TARGET L$labelName{$2} - $dict_ofs // $dict_ofs: \$\$$1";
                next;
            }

            if(/\G(0|1|2|-1)\b/gc) {
                my $word = $1;
                $word = '$_minus_1' if $word eq '-1';
                emitDict "Cell(&Dictionary[${CodeAddressWord{$word}}])              // $dict_ofs: $word";
                next;
            }

            # probably don't need this, now that the adr(), dwc() and the special treatment of 0 1 2 -1 is handled
            if(/\Gthread\(($re_name)\)/gc) {
                my $ident = fixIdent $1;
                validate $ident;
                emitDict "Cell(&Dictionary[${CodeAddressWord{$ident}}])             // $dict_ofs: $ident";
                next;
            }

            if(/\G([-+]?(?:0[xX][0-9a-fA-F]+|\d+)\b)/gc) {
                emitDict "Cell(&Dictionary[${CodeAddressWord{'$$literal'}}])        // $dict_ofs: \$\$literal";
                emitDict "Cell($1)                                                  // literal value";
                next;
            }

            if(/\G($re_name)/gc) {
                my $ident = fixIdent $1;
                validate $ident;
                emitDict "Cell(&Dictionary[${CodeAddressWord{$ident}}])             // $dict_ofs: $ident";
                next;
            }

            if(/\G($re_char)/gc) {
                emitDict "Cell(&Dictionary[${CodeAddressWord{'$$literal'}}])        // $dict_ofs: \$\$literal";
                emitDict ordChar($1) . "                                            // literal value $1";
                next;
            }

            warn "bad reference to a word - you probably want to use the munged name! $_\n" if /\G./;
            last;
        }
    }

    if(! $word_type && /^VOCABULARY\s+($re_ident)\s*;?\s*$/) {
        my $ident = $1;
        if(exists $dict_link{$ident}) {
            emitCode "    voc_$ident = CellPtr($dict_link{$ident});";
            $dict_link{$ident} = "NULL";
        }
        next;
    }

    if($word_type eq "PRIMARY") {
        s/^next;/goto \$NEXT;/;
        emitCode "$indent$_";
    }
    elsif($word_type ne "SECONDARY") {
        emitCode $_;
    }
}

foreach my $c (@code1) {
    (my($lno), local($_)) = @$c;

    if(/^INJECTIONS$/) {
        if(defined $opt_s) {
            print "# define        OPT_S\n";
            print "# define        DICT_MAGIC          $dict_magic\n";
        }

        next;
    }

    emitLine $lno, $_;
}


my $blanked = 1;    # lies!
foreach my $d (@dict) {
    (my $lno, $_) = @$d;
    my ($body, $comment) = m:^(.*?)\s*(//.*?)?\s*$:;

    if(! $body && ! $comment) {
        emitLine $lno, '' if ! $blanked;
        $blanked = 1;
        next;
    }

    $blanked = 0;
    if($body) {
        # do fixups
        $body =~ s/TARGET L(\d+) - (\d+)/$labelOffset{$1} - $2 - 1/e;
        $body =~ s:CODE_ADDRESS\(($re_name)\):&Dictionary[${CodeAddressWord{fixIdent $1}}]:g;
        $body = "        $body,";
    }

    my $spaces = 64 - length $body;
    $body .= ' ' x $spaces if $spaces < 64;

    emitLine $lno, "$body$comment";
}

foreach my $c (@code2) {
    (my ($lno), local($_)) = @$c;

    s:CODE_ADDRESS\(($re_name)\):&Dictionary[${CodeAddressWord{fixIdent $1}}]:g;
    s:FIRST_WORD\(($re_name)\):CellPtrPtr(&Dictionary[${CodeAddressWord{fixIdent $1}} + 1]):g;
    s:ASSIGN_DP:DP = &Dictionary[$dict_ofs]:;
    emitLine $lno, $_;
}

print STDERR "Dictionary size = $dict_ofs\n";

exit $rc;

