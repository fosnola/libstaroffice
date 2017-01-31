#!/usr/bin/perl

my $html = 0;		# set to 1 to output a nicely formatted HTML page

my %toDo=("csv"=>0, "epub"=>0, "html"=>0, "odf"=>0, "raw"=>1, "svg"=>0, "valgrind"=>0 );

my $pass_colour = "11dd11";
my $fail_colour = "dd1111";
my $warn_colour = "e59800";
my $skip_colour = "9999dd";

sub DisplayCell
{
  my ($bgColor, $text) = @_;

  print "<td style='background-color: #$bgColor;'>$text</td>\n";
}

sub DiffTest
{
  my ($command, $command2, $file, $extension, $condition) = @_;
  my $result = "passed";
  my $comment = "";

  if (!$condition) {
    DisplayCell($skip_colour, "skipped") if ($html);
    return "skipped";
  }

  my $errPath = $file . ".$extension.err";
  my $rawPath = $file . ".$extension";
  my $newRawPath = $file . ".$extension.new";
  my $diffPath = $file . ".$extension.diff";

  # generate a new raw output to compare with
  `$command $file 1> $newRawPath`;
  if ($command2) {
    `mv $newRawPath $newRawPath.tmp`;
    `$command2 $newRawPath.tmp 1> $newRawPath`;
    `rm $newRawPath.tmp`;
  }

  # HACK: check if there is a raw file with _some_ contents. If not, we've had a segfault
  my $err = "";
  my $diff = "";
  $newRaw=`cat $newRawPath`;
  if ($newRaw eq "") {
    $err = "Segmentation fault";
    `echo $err > $errPath`;
  }
  
  if ($err ne "") {
    $result = "fail";
  } else {
    # remove the generated (empty) error file
    `rm -f $errPath`;
      
    # diff the stored raw data with the newly generated raw data
    `diff -u $rawPath $newRawPath 1>$diffPath 2>$diffPath`;
    $diff=`cat $diffPath | grep -v "No differences encountered"`;
      
    if ($diff ne "") {
      $result = "changed";
    } else {
      `rm -f $diffPath`;
    }
  }
	    
  # remove the generated raw file
  `rm -f $newRawPath`;

  # DISPLAYING RESULTS
  if ($html) {
    my $bgColor;
    if ($diff eq "" && $err eq "") {
      $bgColor = $pass_colour;
    } elsif ($err ne "") {
      $bgColor = $fail_colour;
    } else {
      $bgColor = $warn_colour;
    }

    if ($err ne "" || $diff ne "") {
      $comment = " <a href='" . ($err ne "" ? $errPath : $diffPath) . "'>" . ($err ne "" ? "error" : "diff") . "<a>";
    }

    DisplayCell($bgColor, $result . $comment);
  } else {
    if ($err ne "" || $diff ne "") {
      $comment = ($err ne "" ? "(error: " : "(diff: ") . ($err ne "" ? $errPath : $diffPath) . ")";
    }
    print "! $file diff (using $command): $result $comment\n";
  }

  return $result;
}

sub CgTest
{
  my ($command, $file) = @_;

  my $callgraph = `sd2raw -c $file`;
  chomp($callgraph);

  return $callgraph;
}

sub CheckVg
{
  my ($file,$msg) = @_;
  open VG, "$vgPath";
  my $result = "passed";
  my $valgrind = 0;
  my $vgOutput;
  while (<VG>) {
    if (/^\=\=/) {
      $vgOutput .= $_;
      if (/definitely lost: [1-9]/ ||
	  /ERROR SUMMARY: [1-9]/ ||
	  /Invalid read of/) {
	$valgrind = 1;
      }
    }
  }
  close VG;

  `rm -f $vgPath`;
  if ($valgrind) {
    open VG, ">$vgPath";
    print VG $vgOutput;
    close VG;
    $result="fail";
    $valgrind = 1;
  }

  if ($html) {
    ($valgrind eq 0 ? DisplayCell($pass_colour, "passed") : DisplayCell($fail_colour, "failed <a href='$vgPath'>log<a>"));
  } else {
    print $msg. " " . ($valgrind eq "0" ? "passed" : "failed (see '$vgPath')") . "\n";
  }

  return $result;
}

sub RegTest
{
  my $i;
  # basic raw/html endOfLine
  my $endLine=($html?"<br>\n":"\n");

  # list of directories containing test files
  my @listDirectory = ( "Calc3.1", "Calc4", "Calc5", "Draw3.1", "Draw4", "Draw5", "Pres5", "Text3.1", "Text4", "Text5" );

  # list of commands to test
  # type => [ "name", "extension", "array of accepted types", "command", "filter" ]
  my @commands=("raw", "csv", "epub", "html", "svg", "odf");
  my %listCommand = (
    "csv" => [ "CSV", "csv", ["S"], "sdc2csv", 0 ],
    "epub" => [ "Epub", "epub", ["G", "P", "S", "T"], "sd2epub", 0],
    "html" => [ "Html", "html", ["T"], "sdw2html", 0],
    "odf" => ["WriterPerfect", "writerperfect", ["G", "P", "S", "T"], "sd2odf", "xmllint --format"],
    "raw"=>["Raw", "raw", ["G", "P", "S", "T"], "sd2raw", 0],
    "svg" => ["SVG", "svg", ["G", "P"], "sd2svg", 0]
      );
  
  # valgrind command
  my $vgVersionOutput = `valgrind --version`;
  my $vgCommand=( ($vgVersionOutput =~ /\-2.1/ || $vgVersionOutput =~ /\-2.2/) ? "valgrind --tool=memcheck" : "valgrind" );
  # init failures
  my $callGraphFailures = 0;
  my %diffFailures, %valgrindFailures;
  foreach $i ( @commands ) {
    $diffFailures{$i}=0;
    $valgrindFailures{$i}=0;
  }
  foreach my $directory ( @listDirectory ) {
    if ($html) {
      print "<b>Regression testing the " . $directory . " parser</b><br>\n";
      print "<table>\n";
      print "<tr>\n";
      print "<td style='background-color: rgb(204, 204, 255);'><b>File</b></td>\n";
      print "<td style='background-color: rgb(204, 204, 255);'><b>Call Graph Test</b></td>\n";
      foreach $i ( @commands ) {
	print "<td style='background-color: rgb(204, 204, 255);'><b>$listCommand{$i}[0] Diff Test</b></td>\n" if ($toDo{$i});
      }
      if (!$toDo{"valgrind"}) {
	print "<td style='background-color: rgb(204, 204, 255);'><b>Valgrind Tests</b></td>\n";
      }
      else {
	foreach $i ( @commands ) {
	  print "<td style='background-color: rgb(204, 204, 255);'><b>$listCommand{$i}[0]<br> Valgrind Test</b></td>\n" if ($toDo{$i});
	}
      }
      print "</tr>\n";
    } else {
      print "Regression testing the " . $directory . " parser\n";
    }

    my $regrInput = $directory . '/regression.in';

    my @fileList = split(/\n/, `cat $regrInput`);
    foreach $_ ( @fileList ) {
      if (!/^([GPST]):(.*)/) {
        print "Skip ".$_.": unknown type\n";
	next;
      }

      $file=$2;
      $fileType=$1;
      my $filePath = $directory  . '/' . $file;
      if ($html) {
        print "<tr>\n";
        print "<td><a href='" . $filePath . "'>" . $file . "</a></td>\n";
      }

      # //////////////////////////
      # CALL GRAPH REGRESSION TEST
      # //////////////////////////

      my $cgResult = CgTest("sd2raw --callgraph", $filePath);

      $callGraphFailures++ if ($cgResult ne "0");
      if ($html) {
	($cgResult eq "0" ? DisplayCell($pass_colour, "passed") : 
	 DisplayCell($fail_colour, "failed"));
      } else {
	print "! $file call graph: " . ($cgResult eq "0" ? "passed" : "failed") . "\n";
      }

      # /////////////////////
      # DIFF REGRESSION TESTS
      # /////////////////////

      foreach $i ( @commands ) {
	next if (!$toDo{$i});
	$diffFailures{$i}++ if (DiffTest($listCommand{$i}[3], $listCommand{$i}[4], $filePath, $listCommand{$i}[1], ($fileType ~~ $listCommand{$i}[2]) ) eq "fail");
      }

      # ////////////////////////
      # VALGRIND REGRESSION TEST
      # ////////////////////////
      if (!$toDo{"valgrind"}) {
	if (!$html) {
	  print "! $file valgrind: skipped all tests\n";
	}
	else {
	  DisplayCell($skip_colour, "skipped");
	  print "</tr>\n"
	}
	next;
      }

      foreach $i ( @commands ) {
	next if (!$toDo{$i});
	if ($fileType ~~ $listCommand{$i}[2]) {
	  $vgPath = $directory  . '/' . $file . '.' . $listCommand{$i}[1] . 'vg';
	  `$vgCommand --leak-check=yes $listCommand{$i}[3] $filePath 1> $vgPath 2> $vgPath`;
	  $valgrindFailures{$i}++ if (CheckVg($vgPath, "! $file $listCommand{$i}[0] valgrind:") eq "fail");
	}
	elsif ($html) {
	  DisplayCell($skip_colour, "skipped");
	}
      }

      print "</tr>\n" if ($html);
    }

    print "</table><br>\n" if ($html);
  }

  #
  # Summary
  #
  if ($html) {
    print "<b>Summary</b>".$endLine;
  } else {
    print "\nSummary".$endLine;
  }
  ## callgraph errors
  print "Regression test found " . $callGraphFailures . " call graph failure(s)".$endLine;
  # diff errors
  foreach $i ( @commands ) {
    if ($toDo{$i}) {
      print "Regression test found $diffFailures{$i} $listCommand{$i}[0] diff failure(s)".$endLine;
    } else {
      print "$listCommand{$i}[0] test skipped".$endLine;
    }
  }
  ## valgrind errors
  if (!$toDo{"valgrind"}) {
    print "Valgrind test skipped".$endLine;
  }
  else {
    foreach $i ( @commands ) {
      if ($toDo{$i}) {
	print "Regression test found $valgrindFailures{$i} $listCommand{$i}[0] valgrind failure(s)".$endLine;
      } else {
	print "$listCommand{$i}[0] valgrind test skipped".$endLine;
      }
    }
  }
}

my $confused = 0;
while (scalar(@ARGV) > 0) {
  my $argument = shift @ARGV;
  if ($argument =~ /--output-html/) {
    $html = 1;
    next;
  }
  $find=0;
  foreach $key (keys %toDo) {
    if ($argument =~ /--$key/) {
      $find=1;
      $toDo{$key} = 1;
      break;
    }
  }
  $confused = 1 if (!$find);
}
if ($confused) {
  print "Usage: regression.pl [--output-html] [--valgrind] [--csv] [--epub] [--html] [--odf] [--svg]\n";
  exit;  
}

# Main function
if ($html)
{
  print "<html>\n<head>\n</head>\n<body>\n";
  print "<h2>libstoff Regression Test Suite</h2>\n";
}

&RegTest;

if ($html)
{
  print "</body>\n</html>\n";
}

