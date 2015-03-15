# Introduction #
Sundry Issues with the simulator


# Details #

  1. Meeting 11pm EST Thursday Feb 24:
> > Region coordinates
> > Define / Find out GPS or UTM coordinates of region boundaries.

  1. Interprocess communications:
> > http://docs.python.org/dev/library/multiprocessing.html

  1. Separate join agent and CSM code at system-level or application level?
> > Application level lets us prototype faster all within Python
> > System-level is closer to production code which allows multiple CSM applications    to use single join agent, etc.

  1. Broadcast issues
Is it because of TAP?

  1. 8.2.11a ranges calibrated correctly?
Currently about 25-30m. Seems to be way too low for freespace.

  1. Need to stress test ns-3
> > Achievable scale of simulation?

> State of CSM implementation in ns-3

  1. The high level idea is clear: The CSM code runs in a bunch of small containers, each of which is a lightweight VM. ( lxc)
jason: what’s the lightest weight lxc “distro” we can get? anything special that CSM code requires other stdlib?

  1. These VMs all have a unique IP address for themselves and access their wireless “simulated” net cards using a tap device.

  1. The simulator which is where ns-3 comes in runs in real time and simulates the wireless channel depending on current position of nodes in the simulation.


  1. How much load can the simulator handle while running real time? I ran into a bizarre problem documented by me here, which I am yet to figure out: http://groups.google.com/group/ns-3-users/browse_thread/thread/7e740e49e513089b
There seems to be a bug somewhere there.

  1. What is the default range of Wireless 802.11 a when two nodes are stationery and there is nothing else to interfere with their communication with each other?

Right now, I seem to be getting a range of only 25 to 30 m as observed by pings.
But, on closer inspection using ns-3 physical layer logging, it’s seen that packets are actually delivered from one node to another, but the second node does not respond.

  1. Why does it take some time for the VMs to “recognize” the simulator that is running the tap devices?

  1. How do you set limits on the RAM a light weight VM takes up for it’s contained processes ?

  1. Given that scaling may be an issue with real time systems in a different way ( ie missing deadlines and aborting), can be parallelize a many node system using mpi. ns-3 does provide features for this.

  1. Converting cab traces into ns-2 and subsequently feeding it into ns-3. We need to check on the fidelity of this conversion.