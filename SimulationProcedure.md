# Introduction #

Same as above


# Details #

1. First, define a region of interest in Singapore , by defining the left bottom and top right corners. Also, define a time duration of interest.



2. Convert the corners given in GPS coordinates into UTM coordinates using GeoConvert.


3. UTM is planar so use these coordinates along with duration of interest to filter and reduce the area of interest.


4. Feed the UTMed corners and time duration into a script that converts from our GPS cab traces into an ns-2 style trace which tells every node where it is at all points in time.


5. This script ( the way I have written) also comments into the ns-2 style trace file when a node goes out of our Area of Inteerest. Nodes on reading this, can drop out of the CSM. All this code has to be implemented in movementTracking.py


6. Run a perl script to determine the node count in the file which determines the maximum node ID. DONE


6b. Divide the ns-2 trace file into so many separate trace files based on the maximum node ID.


7. Create as many VMs as the maximum nodeID and start them up along with the simulation.

A rough calculation: In the CBD, in a 70 km2 area over three hours 12000 taxis are visible. So over an hour, over 1 km2 it comes out to be roughly 57 taxis. I should check this independently. In this sense, I think it will scale.