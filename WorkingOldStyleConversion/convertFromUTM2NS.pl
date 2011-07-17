use Text::ParseWords;
use Math::Trig; 
open(FILE,$ARGV[0]);
#maintain a hash about the last state for every node ID
#maintain a hash for every nodeID to determine its local relative node ID 
$xStart=$ARGV[1];
$yStart=$ARGV[2];
$xEnd=$ARGV[3];
$yEnd=$ARGV[4];
$counter=0;
$PI=3.14159265;
## TODO: Still have to write some code for correcting the other coordinate when the first coordinate is truncated at the boundary. 

while(<FILE>) {

	$line=$_;
	chomp($line);				# read the next line and discard any white spaces at the end, chomp discards the spaces and newlines at the end 
	@currentState=shellwords($line);	# split the line on spaces
	$nodeId=$currentState[1];		# second column is the node ID

	$x=$currentState[3];			# UTM x coordinate
	$y=$currentState[4];			# UTM y coordinate 

	if($x > $xStart && $x < $xEnd && $y > $yStart && $y < $yEnd) { # within bounding box, we are good 

		if($last{$nodeId}) {		# hash that stores the last state as a string entry hashed on every node ID 
						# the if checks if the entry exists at all 
						# If there was a last state, emit the setdest now, else emit a set instead of a setdest 
						# the if checks that this node was seen before in this particular bounded terrain	
			if($last{$nodeId} eq "exited") {
						# its re-entering the terrain after exiting the terrain once before. 					
				$time=$currentState[0];
				$xLocation=$currentState[3];
				$yLocation=$currentState[4];
				$xLocation=$xLocation-$xStart;
				$yLocation=$yLocation-$yStart;
				print "\$ns_ at $time \$node_($localNodeId{$nodeId}) startNode \n";	# start communication for this node 
				print "\$ns_ at $time \$node_($localNodeId{$nodeId}) set X_ $xLocation \n";
				print "\$ns_ at $time \$node_($localNodeId{$nodeId}) set Y_ $yLocation \n";
				$last{$nodeId}=$line;
			}

			else 			 { # The Node did not "Exit" and its last is non null and so
	                                           # it was already in the bounded box , so emit a setdest 
				@lastState=shellwords($last{$nodeId});
				$time=$lastState[0];
				$lastX=$lastState[3];
				$lastY=$lastState[4];
				$velocity=$lastState[5];
				$xDest=$currentState[3];
				$yDest=$currentState[4];
				if($lastX==$xDest && $lastY==$yDest)  { # no movement at all don't emit trace
					print "# Duplicated trace entry"; # do nothing 
					$xDest=$xDest-$xStart;# relative to offsets 
					$yDest=$yDest-$yStart;
					print "# \$ns_ at $time  \$node_($localNodeId{$nodeId}) setdest $xDest $yDest $velocity \n";	# comment this out into the trace file so that ns3 does not act on
				}
				elsif($velocity != 0 ) {
					$xDest=$xDest-$xStart;# relative to offsets 
					$yDest=$yDest-$yStart;
					print "\$ns_ at $time  \$node_($localNodeId{$nodeId}) setdest $xDest $yDest $velocity \n";
				}			# don't bother printing, it's anyway stationery at this location. May save ns some trace locations. 
				$last{$nodeId}=$line;	# update lastSeen State
			}
		}	
	
		else {
			# output set instead of set dest if you are seeing it for the first time since the last{$nodeId} entry is null , else it won't get here
			# So, assign a new localnodeID to the node
		
	
			$time=$currentState[0];
			$xLocation=$currentState[3];
			$yLocation=$currentState[4];

			$xLocation=$xLocation-$xStart;	# subtract offset from x location. 
			$yLocation=$yLocation-$yStart;  # subtract offset from y location too

			$localNodeId{$nodeId}=$counter;	
			$counter++;
			print "\$ns_ at $time \$node_($localNodeId{$nodeId}) startNode \n";		# start communication for this node 
			print "\$ns_ at $time \$node_($localNodeId{$nodeId}) set X_ $xLocation \n";
			print "\$ns_ at $time \$node_($localNodeId{$nodeId}) set Y_ $yLocation \n";
			$last{$nodeId}=$line;
	
		}
	}


	else {				##################################CURRENT STATE OF THE NODE IS OUT OF THE BOUNDING BOX, ELSE IT WOULD NOT BE HERE ################################################################################
					# Did the node just go out ? Then truncate its path to the boundary.
					# Is this the first sighting of the node and it is outside the region to start out with itself, in which case don't bother, wait till it gets in ?
					# Did the node go out and is it coming back inside now ? In this case, again use set instead of setdest.

					# The logic: If the coordinate is within the bound box: 
					# 		check if the node has been seen for the first time now, then use set else use setdest, assign a new relative ID to this node.
					# If the coodinate is out of the bound box, then if the node has not been seen before, don't bother.
					# else if the node has been seen before, truncate to boundary set last state =0.
					# now the next time this node is seen in the trace a set will be used instead of a setdest, but the old ID will be used.
					# check if NS allows two sets.   

					#	if($last{$nodeId} eq "exited") {	# if the node exited previously 
					#				# path needs to be truncated here.
					#		# don't do anything, its already outside
					#		
					#	}

		if($last{$nodeId} ne "exited" && $last{$nodeId} )    	{
					# there is a last state entry that is not "exited"
					#truncate the path, only if it did not exit in its last state, no point truncating paths for nodes that are already outside or for nodes that were not evne part of the simulation earlier ie last is{$nodeId} is null  
			@lastState=shellwords($last{$nodeId});
			$time=$lastState[0];
			$velocity=$lastState[5];
			$xDest=$currentState[3];
			$yDest=$currentState[4];
			$xOrig=$lastState[3];
			$yOrig=$lastState[4];
		
			if($xDest==$xOrig && $yDest==$yOrig) {
				# it has not moved, so nothing
				print"# no change in position when node exited terrain of interest \n";

			}
		
			elsif($yDest==$yOrig ) {		# zero slope
				# moves horizontally, so only truncate based on x coordinate
				if($xDest >= $xEnd ) {
					$xDest=$xEnd-0.01;
				}
				elsif($xDest <= $xStart) {
					$xDest=$xStart+0.01;
				}
				# print trace here
				$xDest=$xDest-$xStart;
				$yDest=$yDest-$yStart;
				if($velocity != 0) {
					print "\$ns_ at $time  \$node_($localNodeId{$nodeId}) setdest $xDest $yDest $velocity \n";
				}			# don't bother printing, it's anyway stationery at this location. May save ns some trace locations. 


			}
			elsif($xDest==$xOrig) {		# infinite slope
				if($yDest >= $yEnd ) {
					$yDest=$yEnd-0.01;
				}
				elsif($yDest <= $yStart) {
					$yDest=$yStart+0.01;
				}
				# print trace here
				$xDest=$xDest-$xStart;
				$yDest=$yDest-$yStart;
				if($velocity != 0) {
					print "\$ns_ at $time  \$node_($localNodeId{$nodeId}) setdest $xDest $yDest $velocity \n";
				}			# don't bother printing, it's anyway stationery at this location. May save ns some trace locations. 

			}

			else {	# normal slopes 	
				# do the truncation here
				$slopeToDestination=($yDest-$yOrig)/($xDest-$xOrig);
				$angleToDestination=atan($slopeToDestination);

				# correct this angle to fall between 0 and 2*PI.
				if($yDest >= $yOrig && $xDest >= $xOrig) {
					# first quadrant
					$angleToDestination=$angleToDestination;	# no change required. 
				}
				elsif($yDest >= $yOrig && $xDest <= $xOrig) {
					# second quadrant
					$angleToDestination=$angleToDestination+$PI;	# second quadrant as given below 	
				}
				elsif($yDest <= $yOrig && $xDest <= $xOrig) {
					# third quadrant
					$angleToDestination=$angleToDestination+$PI;	# third quadrant
				} 

				else  {
					# fourth quadrant
					$angleToDestination=$angleToDestination+2*$PI;
					
				}
				# end of angleToDestination Correction

				$topRightAngle=atan(($yEnd-$yOrig)/($xEnd-$xOrig));
				$topLeftAngle=atan(($yEnd-$yOrig)/($xStart-$xOrig));
				$topLeftAngle=$topLeftAngle+$PI;			# It Lies in the second quadrant and topLeftAngle is negative by principal value 

				$bottomRightAngle=atan(($yStart-$yOrig)/($xEnd-$xOrig));
				$bottomRightAngle=$bottomRightAngle+$PI;		# It lies in the third quadrant and bottomRightAngle is positive by principal value 

				$bottomLeftAngle=atan(($yStart-$yOrig)/($xStart-$xOrig));
				$bottomLeftAngle=$bottomLeftAngle+2*$PI;			# It lies in the fourth quadrant. 


				##############IN ALL TRUNCATIONS BELOW: The new x coordinate $xDest has to satisfy: 	$yDest-$yOrig=$slopeToDestination*($xDest-$xOrig);  #######################
				
				if($angleToDestination >= $topRightAngle && $angleToDestination <= $topLeftAngle) {
					$yDest=$yEnd-0.01;				# exits through the max y coordinate at some x 
					if($slopeToDestination != 0) {
						$xDest=(($yDest-$yOrig)/$slopeToDestination)+$xOrig;
					}
					$yDest=$yDest-$yStart;				# coordinates relative to our offsets.
					$xDest=$xDest-$xStart;	

				}

				elsif($angleToDestination > $topLeftAngle && $angleToDestination <= $bottomRightAngle) {
					$xDest=$xStart +0.01;				# Exits through min x coordinate
					$yDest=$yOrig+$slopeToDestination*($xDest-$xOrig);

					$yDest=$yDest-$yStart;				# coordinates relative to our offsets.
					$xDest=$xDest-$xStart;	


				}

				elsif($angleToDestination > $bottomRightAngle && $angleToDestination <= $bottomLeftAngle  ) {
					$yDest=$yStart+0.01;				# exits through min y coordinate
					if($slopeToDestination != 0) {
						$xDest=(($yDest-$yOrig)/$slopeToDestination)+$xOrig;
					}
					$yDest=$yDest-$yStart;				# coordinates relative to our offsets.
					$xDest=$xDest-$xStart;	


				}

				else			 {
					$xDest=$xEnd-0.01;				# exits through max x
					$yDest=$yOrig+$slopeToDestination*($xDest-$xOrig);

					$yDest=$yDest-$yStart;				# coordinates relative to our offsets.
					$xDest=$xDest-$xStart;	


				}



				if($velocity != 0) {
					print "\$ns_ at $time  \$node_($localNodeId{$nodeId}) setdest $xDest $yDest $velocity \n";
				}			# don't bother printing, it's anyway stationery at this location. May save ns some trace locations. 

				print "\$ns_ at $time \$node_($localNodeId{$nodeId}) stopNode \n";	# no more communication for this node 
				$last{$nodeId}="exited";
			}
		}
					# else the last state does not exist, do nothing here :) 
	}
}
