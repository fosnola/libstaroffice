#!/usr/bin/perl

$html = 1;

sub GenRaw {

    $STOFFVL = 'Calc3.1 Calc4 Calc5 Draw3.1 Draw4 Draw5 Pres5 Text3.1 Text4 Text5';

    @stoffVersionList = split(/\s+/, $STOFFVL);

    foreach $stoffVersion ( @stoffVersionList )
    {
	# remove all diff files, since they are possible outdated now
	$diffs = $stoffVersion . '/*.diff '.$stoffVersion . '/*/*.diff';
	`rm -f $diffs`;
    
	$regrInput = $stoffVersion . '/regression.in';
        $FL = `cat $regrInput`;

	@fileList = split(/\n/, $FL);
        foreach $_ ( @fileList )
	{
	    if (!/^([GPST]):(.*)/) {
		print "Skip ".$_.": unknown type\n";
		next;
	    }

	    $file=$2;
	    $fileType=$1;
	    
	    $filePath = $stoffVersion  . '/' . $file;
	    `sd2raw $filePath >$filePath.raw 2>$filePath.raw`;

	    `sdw2html $filePath >$filePath.html` if $fileType eq "T";
	    `sdc2csv -o $filePath.csv $filePath` if $fileType eq "S";
	    `sd2svg  -o $filePath.svg $filePath` if ($fileType eq "G" || $fileType eq "P");

	    `sd2odf $filePath > $filePath.writerperfect.tmp`;
	    `xmllint --format $filePath.writerperfect.tmp > $filePath.writerperfect`;
	    `rm $filePath.writerperfect.tmp`;
	}
    }
}

# Main function
&GenRaw;

1;
