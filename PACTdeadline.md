PACT deadline:

1. pinging one host from another does not work in my simulation.

The ARP request is heard and the ARP reply is sent out, but the ARP reply is not heard. Try and fix or get around this problem.

ARP requests on the other hand are heard by all those concerned on the subnet. ie the ARP request which is a broadcast seems to work.

I tried this with a netcat instead of a ping, no difference still.

CONCLUSION: It looks like broadcast packets are heard correctly and forwarded upwards in the stack. Unicast packets are not received correctly, presumably because they are just dropped on the ground even by the intended recipient.

IDEALLY: We would not want to send unicast packets all over the place as broadcast like we are doing now and then drop it at the application.  For now, the situation seems to be that only broadcast packets are received correctly but other packets are not.

WORKAROUND: Send all packets in broadcast using libnet and filter them at the application layer. Inefficient but sort of works.

2. Issues with thread and timer interaction in python. Does python handle thread scheduling correctly ?

My libnet thread ( I have to use this, seems to be hogging all the time on the processor and the timer thread is missing packets).


3. Removing the IGMP packets.

4. Getting the energy logging to work correctly.

5. Trace Conversion from GPS trace into ns3.: Add the set’s and remove duplicate entires. DONE

6. Synchronizing node stops and node starts.

7. Is one WiMax station sufficient to cover the entire area ? Gives upto 750 meters Base Station to Mobile Range but we need more I guess.

8. Are our cab traces dense enough ?  Not sure.

9. Setting up a static route to the server inside the cloud seems extremely difficult. The automatic route setup works only for P2P and CSMA networks and not for these "hybrid" kind of networks.

10. I am trying to port the whole 3-G wimax and wifi setup into the vm framework. This seems to be throwing random sigsev at times.

11. Calculate the density of nodes from the cab traces.
Is 12000 per 70 km2 per three hours sensible? Verify this from the raw traces.

12. How do you synchronize and send the start signal to all the processes in the beginning ?

13. The Wimax emulation runs without any VMs on top for a long enough time.

14. Wimax emulation seems to be a PITA for now.