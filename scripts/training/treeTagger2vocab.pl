#! /usr/bin/perl -w

#******************************************************************************
# Prashant Mathur @ FBK-irst. July 2014
#******************************************************************************
# after each tagged sentence there should be a
# ---   NN  ---
# line

use strict;
use warnings;
#use open ':utf8';
binmode STDIN,  ':utf8';
binmode STDOUT, ':utf8';
use Data::Dumper;
use Getopt::Long "GetOptions";

my $__usage = "
USAGE
-----
perl treeTagger2vocab.pl --corpus=name --prefix=name

[OUTPUT]
prefix_surface.txt
prefix_tags.txt
prefix_lemma.txt
-----
";

my $__debug;
my $__corpus = "";
my $__help;
my $__prefix = "prefix";
my (%WordsVocab, %TagsVocab, %LemmasVocab, %RevTagsVocab, %RevLemmaVocab)=();
GetOptions(
	'debug'    => \$__debug,
	'corpus=s' => \$__corpus,
	'prefix=s' => \$__prefix,
	'help'     => \$__help
);

if ($__help) { die "$__usage\n\n"; }

open __CORPUS, "$__corpus"
  or die "OOPS! Couldn't open the source corpus file\n";
open __SURFACE, ">${__prefix}.factor0.txt"
  or die "OOPS! Cannot write to file ${__prefix}.factor0.txt\n";
open __TAGS, ">${__prefix}.factor1.txt"
  or die "OOPS! Cannot write to file ${__prefix}.factor1.txt\n";
open __LEMMA, ">${__prefix}.factor2.txt"
  or die "OOPS! Cannot write to file ${__prefix}.factor2.txt\n";
my $counter=0;
my %tuple=();
while (<__CORPUS>) {
	chop();
	if ( $_ =~ m/^----------/ ) {
        if($counter!=0){
		    print __SURFACE "\n";
		    print __TAGS "\n";
		    print __LEMMA "\n";
        }
		next;
	}
	my ( $words, $tags, $lemma ) = split(/\t/);
	my ($id1, $id2, $id3)=0;
	if ( defined $words && defined $tags && defined $lemma ) {
		if(defined $tuple{$words}{$tags}{$lemma}){
			my @temp = @{$tuple{$words}{$tags}{$lemma}};
			$id1 = $temp[0];
			$id2 = $temp[1];
			$id3 = $temp[2];
		}
		else{
			$id1 = getWordsVocabID($words);
			$id2 = getTagsVocabID($tags);
			$id3 = getLemmasVocabID($lemma);
			push(@{$tuple{$words}{$tags}{$lemma}}, $id1);
			push(@{$tuple{$words}{$tags}{$lemma}}, $id2);
			push(@{$tuple{$words}{$tags}{$lemma}}, $id3);
		}
		print __SURFACE $id1 . " ";
		print __TAGS $id2 . " ";
		print __LEMMA $id3 . " ";
	}
    $counter++;
}
close(__SURFACE);
close(__TAGS);
close(__LEMMA);

DumpVocab($__prefix);

sub getWordsVocabID{
	my $a = shift;
	$WordsVocab{scalar(keys %WordsVocab)} = $a;
	return scalar(keys %WordsVocab)-1;
}

sub getTagsVocabID{
	my $a = shift;
	if(defined $RevTagsVocab{$a}){
		return $RevTagsVocab{$a};
	}
	$TagsVocab{scalar(keys %TagsVocab)} = $a;
	$RevTagsVocab{$a}=scalar(keys %TagsVocab);
	return scalar(keys %TagsVocab)-1;
}

sub getLemmasVocabID{
	my $a = shift;
	if(defined $RevLemmaVocab{$a}){
		return $RevLemmaVocab{$a};
	}
	$LemmasVocab{scalar(keys %LemmasVocab)} = $a;
	$RevLemmaVocab{$a}=scalar(keys %TagsVocab);
	return scalar(keys %LemmasVocab)-1;
}

sub DumpVocab{
	my $prefix = shift;
	open WordsVocabFile, ">${prefix}.factor0.voc" or die;
	foreach my $k(sort keys %WordsVocab){
		print WordsVocabFile $k."\t".$WordsVocab{$k}."\n"; 
	}
	close(WordsVocabFile);
	
	open TagsVocabFile, ">${prefix}.factor1.voc" or die;
	foreach my $k(sort keys %TagsVocab){
		print TagsVocabFile $k."\t".$TagsVocab{$k}."\n"; 
	}
	close(TagsVocabFile);
	
	open LemmasVocabFile, ">${prefix}.factor2.voc" or die;
	foreach my $k(sort keys %LemmasVocab){
		print LemmasVocabFile $k."\t".$LemmasVocab{$k}."\n"; 
	}
	close(LemmasVocabFile);
}
