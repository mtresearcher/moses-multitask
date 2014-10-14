#! /usr/bin/perl -w

#******************************************************************************
# Prashant Mathur @ FBK-irst. October 2014
#******************************************************************************
# perl build-trigger-model.pl --corpus TED --srcln en --trgln de --omodel TED --working-dir=/home/prashant/work/ --scripts-dir=/home/prashant/work/github/moses-current/mosesdecoder-1/scripts/training --treeTagger=/home/prashant/Software/TreeTagger/cmd --source english --target german --factors 2

use strict;
use warnings;
binmode STDIN,  ':utf8';
binmode STDOUT, ':utf8';
use Data::Dumper;
use Getopt::Long "GetOptions";

sub main {
        my $__usage = "
USAGE
-----
perl build-trigger-model.pl --corpus=<string> --srcln=<string> --trgln=<string> --omodel=<string> --working-dir=<path> --scripts-dir=<path> [--treeTagger=<path> --source=<string> --target=<string> --factors=<integer>]
-----

If you specify tree tagger, you would have to specify the english language names.
";
my $__corpus = "";
my $__srcln  = "";
my $__trgln  = "";
my $__omodel = "";
my $__workingdir = "";
my $__scriptsDIR = "";
my $__treeTagger = "";
my $__src = "";
my $__trg = "";
my $__factors = 0;
my $__help;
GetOptions(
                'corpus=s'  => \$__corpus,
                'srcln=s'   => \$__srcln,
                'trgln=s'   => \$__trgln,
                'omodel=s'  => \$__omodel,
		'working-dir=s'=> \$__workingdir,
		'scripts-dir=s' => \$__scriptsDIR,
		'treeTagger=s' => \$__treeTagger,
		'source=s' => \$__src,
		'target=s' => \$__trg,
                'factors=i' => \$__factors,
                'help'      => \$__help
        );

if ($__help) { die "$__usage\n\n"; }

if($__workingdir ne ""){
my $pid=$$;

print STDERR "PREPROCESSING FILES..\n";
system("perl -pe 's/^/----------\n/g' $__corpus.$__srcln > $pid.$__srcln");
system("perl -pe 's/^/----------\n/g' $__corpus.$__trgln > $pid.$__trgln");

print STDERR "RUNNING TREE TAGGER..\n";
if(-x "$__treeTagger/tree-tagger-$__src"){
system("$__treeTagger/tree-tagger-$__src < $pid.$__srcln > $pid.$__srcln.tagged");
}
if(-x "$__treeTagger/tree-tagger-$__trg"){
system("$__treeTagger/tree-tagger-$__trg < $pid.$__trgln > $pid.$__trgln.tagged");
}

print STDERR "BUILDING VOCABULARY..\n";

system("perl $__scriptsDIR/treeTagger2vocab.pl --corpus=$pid.$__srcln.tagged --prefix=$pid.$__srcln");
system("perl $__scriptsDIR/treeTagger2vocab.pl --corpus=$pid.$__trgln.tagged --prefix=$pid.$__trgln");

print STDERR "BUILDING MODEL..\n";
system("perl $__scriptsDIR/build-factored-trigger-model.pl --corpus $pid -srcln $__srcln --trgln $__trgln --omodel $__omodel.trigger-model -factors $__factors");

system("rm -rf $pid*");
}
else{
die "Please specify working dir\n $__usage\n";
}
}

&main();
