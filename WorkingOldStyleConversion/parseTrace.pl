use Text::ParseWords;
use DateTime;
open(FILE,$ARGV[0]);				# convert the given date time into PERL's date time 
$startTime=$ARGV[1];				# start time of all the traces
$endTime = $ARGV[2];				# end time of all the traces

# extract start time from command line arguments
# In the command line argument, the date and time are separated by a -
@dateTimeComponents=quotewords('-',0,$startTime);
$date=@dateTimeComponents[0];
$time=@dateTimeComponents[1];
@dateComponents=quotewords('/',0,$date);
@timeComponents=quotewords(':',0,$time);
$year=$dateComponents[2];
$month=$dateComponents[1];
$day=$dateComponents[0];
#print "$day \t $month \t $year \n";
$hour=$timeComponents[0];
$minute=$timeComponents[1];
$second=$timeComponents[2];

$startDateTime = DateTime->new( year => $year,
				month => $month,
				day => $day,
				hour => $hour,
				minute => $minute, 
				second => $second ,
	                        time_zone => 'America/Chicago',
                           );
## end of startTime extraction 

# extract end time from command line arguments
@dateTimeComponents=quotewords('-',0,$endTime);
$date=@dateTimeComponents[0];
$time=@dateTimeComponents[1];
@dateComponents=quotewords('/',0,$date);
@timeComponents=quotewords(':',0,$time);
$year=$dateComponents[2];
$month=$dateComponents[1];
$day=$dateComponents[0];
#print "$day \t $month \t $year \n";
$hour=$timeComponents[0];
$minute=$timeComponents[1];
$second=$timeComponents[2];

$endDateTime = DateTime->new( year => $year,
				month => $month,
				day => $day,
				hour => $hour,
				minute => $minute, 
				second => $second ,
	                        time_zone => 'America/Chicago',
                           );

## end of endTime extraction

#print @dateComponents;
# convert into the DateTime data type using its constructor.

$counter=0;
#@fileInMemory=<FILE>;
while(<FILE>) {
	$line=$_;
	chomp($line);
	@fields=quotewords(',',0,$line);

	# invert fields 4 and 5 we want lat long not the other way and remove the commas
	# so that geotrans can read from this. 
	# read the timeStamp 

	$timeStamp=$fields[0];
	@dateTime=shellwords($timeStamp);	# date and time are separated by a space in trace file 
	$date=$dateTime[0];
	$time=$dateTime[1];
	
	# Convert into PERL date Time data type

	@dateComponents=quotewords('/',0,$date);
	@timeComponents=quotewords(':',0,$time);
	$year=$dateComponents[2];
	$month=$dateComponents[1];
	$day=$dateComponents[0];
	$hour=$timeComponents[0];
	$minute=$timeComponents[1];
	$second=$timeComponents[2];

	# convert into the DateTime data type using its constructor.
	$currentDateTime = DateTime->new( year => $year,
		month => $month,
		day => $day,
		hour => $hour,
		minute => $minute, 
		second => $second ,
                time_zone => 'America/Chicago',
           );

	
	# check if the Date lies within the boundaries we are searching for
	$cmp1=DateTime->compare($currentDateTime,$startDateTime);
	$cmp2=DateTime->compare($currentDateTime,$endDateTime);

	if($cmp1 > 0 && $cmp2  < 0 ) {
		# within the boundaries, so we can output it, chooses time values only within a certain boundary 
		# do the subtraction here	
		$duration = $currentDateTime->subtract_datetime($startDateTime);
#		print "Start Date is: $startDateTime \n";
#		print "Current Date is : $currentDateTime \n";
 		#	print "Minutes $mins \n";
		$timeElapsed=$duration->seconds;
		$timeElapsed=3600*$duration->hours+ 60*$duration->minutes+$duration->seconds;
		print "$timeElapsed\t";	# try and subtract from an offset time value 
		# check the number plate
		$numPlate=$fields[1];
		if($cabIDs{$numPlate}) {
			print "$cabIDs{$numPlate}\t";
		}

		else {
			$counter++;
			$cabIDs{$numPlate}=$counter;
			print "$cabIDs{$numPlate}\t";
		}

		# lat-long					# Important to invert the order for GeoConvert to work
		print"$fields[4]\t";
		print"$fields[3]\t";	

		# speed
		$speed=$fields[5]/3.6;				# convert speed from kmph to metres per second
		print "$speed\t";
		print "\n";	# end of line		
	}	
}
