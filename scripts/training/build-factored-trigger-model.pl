#! /usr/bin/perl -w

#******************************************************************************
# Prashant Mathur @ FBK-irst. July 2014
#******************************************************************************
# two contiguous source words trigger one target word
# build-factored-trigger-model.pl --corpus <corpus> --srcln en --trgln fr --omodel ISTM.<corpus>
# supports 2 factors at max, specially handles the pos-tags and lemma information from TreeTagger
# use this before on the lemma file :
# perl -pe 'if(/<unknown>/) {s/^(.*)\t(.*)\t(.*)/$1\t$2\t$1/g}' -i tree-tagger-output

use strict;
use warnings;
binmode STDIN,  ':utf8';
binmode STDOUT, ':utf8';
use Data::Dumper;
use Getopt::Long "GetOptions";
my (
	%__factor1_map_src, %__factor2_map_src,
	%__factor1_map_trg, %__factor2_map_trg
  )
  = ();

my (
	%__F0SRCVOC, %__F0TRGVOC, %__F1SRCVOC,
	%__F1TRGVOC, %__F2SRCVOC, %__F2TRGVOC
  )
  = ();

sub main {
	my $__usage = "
USAGE
-----
perl build-factored-trigger-model.pl --corpus=<string> --srcln=<string> --trgln=<string> --omodel=<string> --factors=<INT> 
-----

file structure (with factors): 
<corpus>.<srcln>
<corpus>.<trgln>
<corpus>.<srcln>.factor1.voc
<corpus>.<srcln>.factor1.txt
<corpus>.<trgln>.factor1.voc
<corpus>.<trgln>.factor1.txt
<corpus>.<srcln>.factor2.voc
<corpus>.<srcln>.factor2.txt
<corpus>.<trgln>.factor2.voc
<corpus>.<trgln>.factor2.txt

";

	my $__debug;
	my $__corpus = "";
	my $__srcln  = "";
	my $__trgln  = "";
	my $__omodel = "";
	my $__help;
	my %__model_factor0        = ();    #store the model here
	my %__model_factor1        = ();
	my %__model_factor2        = ();
	my %__joint_counts_factor0 = ();
	my %__joint_counts_factor1 = ();
	my %__joint_counts_factor2 = ();
	my ( $__pairwise_counts, $__pairwise_counts_factor1,
		$__pairwise_counts_factor2 )
	  = 0;
	my (
		$__src_counts_factor0, $__trg_counts_factor0, $__src_counts_factor1,
		$__trg_counts_factor1, $__src_counts_factor2, $__trg_counts_factor2
	  )
	  = 0;
	my $__counter = 0;
	my $__factors = 0;
	my (
		%__factor0_freq_src, %__factor0_freq_trg, %__factor1_freq_src,
		%__factor2_freq_src, %__factor1_freq_trg, %__factor2_freq_trg
	  )
	  = ();

	GetOptions(
		'debug'     => \$__debug,
		'corpus=s'  => \$__corpus,
		'srcln=s'   => \$__srcln,
		'trgln=s'   => \$__trgln,
		'omodel=s'  => \$__omodel,
		'factors=i' => \$__factors,
		'help'      => \$__help
	);

	if ($__help) { die "$__usage\n\n"; }

	print STDERR "
******************************
Reading Vocabulary";
	local (
		*F0_SRC_VOC, *F1_SRC_VOC, *F2_SRC_VOC,
		*F0_TRG_VOC, *F1_TRG_VOC, *F2_TRG_VOC
	);
	open F0_SRC_VOC, "$__corpus.$__srcln.factor0.voc"
	  or die "Cannot open the source factor 0 vocab file\n";
	open F0_TRG_VOC, "$__corpus.$__trgln.factor0.voc"
	  or die "Cannot open the target factor 0 vocab file\n";

	if ( $__factors > 0 ) {
		open F1_SRC_VOC, "$__corpus.$__srcln.factor1.voc"
		  or die "Cannot open the source factor 1 vocab file\n";
		open F1_TRG_VOC, "$__corpus.$__trgln.factor1.voc"
		  or die "Cannot open the target factor 1 vocab file\n";
		if ( $__factors > 1 ) {
			open F2_SRC_VOC, "$__corpus.$__srcln.factor2.voc"
			  or die "Cannot open the source factor 2 vocab file\n";
			open F2_TRG_VOC, "$__corpus.$__trgln.factor2.voc"
			  or die "Cannot open the target factor 2 vocab file\n";
		}
	}
	print STDERR "\nReading Vocabulary...\n";
	while (<F0_SRC_VOC>) {
		chop();
		my ( $key, $value ) = split(/\t/);
		$__F0SRCVOC{$key} = $value;
	}
	print STDERR "Source Vocab Size (Factor 0) : ", scalar( keys %__F0SRCVOC ),
	  "\n";
	while (<F0_TRG_VOC>) {
		chop();
		my ( $key, $value ) = split(/\t/);
		$__F0TRGVOC{$key} = $value;
	}
	print STDERR "Target Vocab Size (Factor 0) : ", scalar( keys %__F0TRGVOC ),
	  "\n";
	if ( $__factors > 0 ) {
		while (<F1_SRC_VOC>) {
			chop();
			my ( $key, $value ) = split(/\t/);
			$__F1SRCVOC{$key} = $value;
		}
		while (<F1_TRG_VOC>) {
			chop();
			my ( $key, $value ) = split(/\t/);
			$__F1TRGVOC{$key} = $value;
		}
		if ( $__factors > 1 ) {
			while (<F2_SRC_VOC>) {
				chop();
				my ( $key, $value ) = split(/\t/);
				$__F2SRCVOC{$key} = $value;
			}
			while (<F2_TRG_VOC>) {
				chop();
				my ( $key, $value ) = split(/\t/);
				$__F2TRGVOC{$key} = $value;
			}
		}
	}

	print STDERR "
******************************";

	local ( *F0_SRC, *F0_TRG, *F1_SRC, *F2_SRC, *F1_TRG, *F2_TRG );

	open F0_SRC, "$__corpus.$__srcln.factor0.txt"
	  or die "Cannot open the source corpus file\n";
	open F0_TRG, "$__corpus.$__trgln.factor0.txt"
	  or die "Cannot open the target corpus file\n";
	if ( $__factors > 0 ) {
		open F1_SRC, "$__corpus.$__srcln.factor1.txt"
		  or die "Cannot open the source first factor file\n";
		open F1_TRG, "$__corpus.$__trgln.factor1.txt"
		  or die "Cannot open the target first factor file\n";
		if ( $__factors > 1 ) {
			open F2_SRC, "$__corpus.$__srcln.factor2.txt"
			  or die "Cannot open the source second factor file\n";
			open F2_TRG, "$__corpus.$__trgln.factor2.txt"
			  or die "Cannot open the target second factor file\n";
		}
	}

	print STDERR "
******************************
CORPUS = $__corpus
Collecting counts";

	my ( $__fh1line_src, $__fh2line_src ) = "";
	my ( $__fh1line_trg, $__fh2line_trg ) = "";
	while ( my $__src = <F0_SRC> ) {
		my $__trg = <F0_TRG>;
		chop($__src);
		chop($__trg);
		if ( $__factors > 0 ) {
			$__fh1line_src = <F1_SRC>;
			$__fh1line_trg = <F1_TRG>;
			chop($__fh1line_src);
			chop($__fh1line_trg);
			if ( $__factors > 1 ) {
				$__fh2line_src = <F2_SRC>;
				$__fh2line_trg = <F2_TRG>;
				chop($__fh2line_src);
				chop($__fh2line_trg);
			}
		}

		$__counter++;
		if ( $__counter % 1000 == 0 ) { print STDERR "."; }

		my @__TOKENS_SRC = split( /\s+/, $__src );
		my @__TOKENS_TRG = split( /\s+/, $__trg );
		my ( @__FACTORS1_SRC, @__FACTORS2_SRC ) = ();
		my ( @__FACTORS1_TRG, @__FACTORS2_TRG ) = ();
		if ( $__factors > 0 ) {
			@__FACTORS1_SRC = split( /\s+/, $__fh1line_src );
			@__FACTORS1_TRG = split( /\s+/, $__fh1line_trg );
			if ( $__factors > 1 ) {
				@__FACTORS2_SRC = split( /\s+/, $__fh2line_src );
				@__FACTORS2_TRG = split( /\s+/, $__fh2line_trg );
			}
			for ( my $i = 0 ; $i < scalar(@__TOKENS_SRC) ; $i++ ) {
				$__factor1_map_src{ $__TOKENS_SRC[$i] } = $__FACTORS1_SRC[$i];
				$__factor2_map_src{ $__TOKENS_SRC[$i] } = $__FACTORS2_SRC[$i];
			}
			for ( my $i = 0 ; $i < scalar(@__TOKENS_TRG) ; $i++ ) {
				$__factor1_map_trg{ $__TOKENS_TRG[$i] } = $__FACTORS1_TRG[$i];
				$__factor2_map_trg{ $__TOKENS_TRG[$i] } = $__FACTORS2_TRG[$i];
			}
		}

		for ( my $i = 0 ; $i < @__TOKENS_SRC ; $i++ ) {
			$__factor0_freq_src{ $__TOKENS_SRC[$i] }++;
			$__pairwise_counts += scalar(@__TOKENS_TRG);
			$__src_counts_factor0++;
		}
		for ( my $i = 0 ; $i < @__TOKENS_TRG ; $i++ ) {
			$__factor0_freq_trg{ $__TOKENS_TRG[$i] }++;
			$__pairwise_counts += scalar(@__TOKENS_SRC);
			$__trg_counts_factor0++;
		}
		for ( my $i = 0 ; $i < @__TOKENS_SRC ; $i++ ) {
			for ( my $j = 0 ; $j < @__TOKENS_TRG ; $j++ ) {
				$__joint_counts_factor0{ $__TOKENS_SRC[$i] }
				  { $__TOKENS_TRG[$j] }++;    # joint counts
			}
		}

		if ( $__factors > 0 ) {
			for ( my $i = 0 ; $i < @__FACTORS1_SRC ; $i++ ) {
				$__factor1_freq_src{ $__FACTORS1_SRC[$i] }++;
				$__pairwise_counts_factor1 += scalar(@__FACTORS1_TRG);
				$__src_counts_factor1++;
				if ( $__factors > 1 ) {
					$__factor2_freq_src{ $__FACTORS2_SRC[$i] }++;
					$__pairwise_counts_factor2 += scalar(@__FACTORS2_TRG);
					$__src_counts_factor2++;
				}
			}
			for ( my $i = 0 ; $i < @__FACTORS1_TRG ; $i++ ) {
				$__factor1_freq_trg{ $__FACTORS1_TRG[$i] }++;
				$__pairwise_counts_factor1 += scalar(@__FACTORS1_SRC);
				$__trg_counts_factor1++;
				if ( $__factors > 1 ) {
					$__factor2_freq_trg{ $__FACTORS2_TRG[$i] }++;
					$__pairwise_counts_factor2 += scalar(@__FACTORS2_SRC);
					$__trg_counts_factor2++;
				}
			}

			for ( my $i = 0 ; $i < @__FACTORS1_SRC ; $i++ ) {
				for ( my $j = 0 ; $j < @__FACTORS1_TRG ; $j++ ) {
					if (   exists $__factor1_freq_src{ $__FACTORS1_SRC[$i] }
						&& exists $__factor1_freq_trg{ $__FACTORS1_TRG[$j] } )
					{
						$__joint_counts_factor1{ $__FACTORS1_SRC[$i] }
						  { $__FACTORS1_TRG[$j] }++;
					}
					if ( $__factors > 1 ) {
						if (   exists $__factor2_freq_src{ $__FACTORS2_SRC[$i] }
							&& exists $__factor2_freq_trg{ $__FACTORS2_TRG[$j] }
						  )
						{
							$__joint_counts_factor2{ $__FACTORS2_SRC[$i] }
							  { $__FACTORS2_TRG[$j] }++;
						}
					}
				}
			}
		}
	}

	close(F0_SRC);
	close(F0_TRG);
	close(F1_SRC);
	close(F1_TRG);
	close(F2_SRC);
	close(F2_TRG);

	&calculatePMI(
		$__pairwise_counts,    \%__joint_counts_factor0,
		\%__factor0_freq_src,  \%__factor0_freq_trg,
		$__src_counts_factor0, $__trg_counts_factor0,
		\%__model_factor0,     0
	);

	if ( $__factors > 0 ) {
		&calculatePMI(
			$__pairwise_counts_factor1, \%__joint_counts_factor1,
			\%__factor1_freq_src,       \%__factor1_freq_trg,
			$__src_counts_factor1,      $__trg_counts_factor1,
			\%__model_factor1,          1
		);
		if ( $__factors > 1 ) {
			&calculatePMI(
				$__pairwise_counts_factor2, \%__joint_counts_factor2,
				\%__factor2_freq_src,       \%__factor2_freq_trg,
				$__src_counts_factor2,      $__trg_counts_factor2,
				\%__model_factor2,          2
			);
		}
	}
	print STDERR "Size of models\nF0 : "
	  . scalar( keys %__model_factor0 )
	  . "\tF1 : "
	  . scalar( keys %__model_factor1 )
	  . "\tF2 : "
	  . scalar( keys %__model_factor2 ) . "\n";
	&DumpModel( \%__model_factor0, \%__model_factor1, \%__model_factor2,
		$__omodel );

	print STDERR "
******************************
Compressing .. 
";
	system("gzip $__omodel");
	print STDERR "
******************************
";
}

sub calculatePMI {

	my ( %__joint_prob, %__prob_x, %__prob_y ) = ();
	my ( $__pairwise_counts, $__joint_counts, $__srcfreq, $__trgfreq,
		$__src_counts, $__trg_counts, $__model, $__factorID )
	  = @_;
	print STDERR "
******************************
Calculating PMI values for factor $__factorID\n";

	foreach my $__token1 ( keys %{$__srcfreq} ) {
		$__prob_x{$__token1} =
		  ( $__srcfreq->{$__token1} * 1.0 ) / ( $__src_counts * 1.0 );
	}
	foreach my $__token2 ( keys %{$__trgfreq} ) {
		$__prob_y{$__token2} =
		  ( $__trgfreq->{$__token2} * 1.0 ) / ( $__trg_counts * 1.0 );
	}
	foreach my $__token1 ( keys %{$__joint_counts} ) {
		foreach my $__token2 ( keys %{ $__joint_counts->{$__token1} } ) {
			$__joint_prob{$__token1}{$__token2} =
			  ( $__joint_counts->{$__token1}->{$__token2} * 1.0 ) /
			  ( $__pairwise_counts * 1.0 );
			$__model->{$__token1}->{$__token2} =
			  log( $__joint_prob{$__token1}{$__token2} ) -
			  ( log( $__prob_x{$__token1} ) + log( $__prob_y{$__token2} ) );
		}
	}
	%__joint_prob = ();
	%__prob_x     = ();
	%__prob_y     = ();
	print STDERR "
******************************
";
}

sub DumpModel {

	print STDERR "
******************************
Dumping Model 
";
	my ( $__model, $__model_factor1, $__model_factor2, $output ) = @_;
	open MODEL, ">", $output
	  or die "Cannot open the model in write mode, Do you have permissions?\n";
	foreach my $__token1 ( sort keys %{$__model} ) {
		if ( defined $__model->{$__token1} ) {
			foreach my $__token2 ( sort keys %{ $__model->{$__token1} } ) {
                my $value1 = sprintf("%.3f", $__model->{$__token1}->{$__token2});
                my $value2 = sprintf("%.3f", $__model_factor1->{ $__factor1_map_src{$__token1} }->{ $__factor1_map_trg{$__token2} });
                my $value3 = sprintf("%.3f", $__model_factor2->{ $__factor2_map_src{$__token1} }->{ $__factor2_map_trg{$__token2} });
				print MODEL $__F0SRCVOC{$__token1}, "|||",
				  $__F0TRGVOC{$__token2}, "|||$value1|||$value2|||$value3\n";
			}
		}
	}
	print STDERR "
******************************
";
}

&main();
