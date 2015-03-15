# Introduction #

This is a list of prospective bugs in ns-3 that I must fix.


# Details #

Here is a list of bugs that must be fixed in ns=3

1. The curious bug in the real time scheduler interacting with static allocation that I showed to Tushar. This is in a separate folder.

2. The problem of insufficient diagnostics in the ns-3 logging framework. If you don't give the Log Component name correctly, it just keeps going without giving any errors.

3. The issue of insufficient diagnostics with the tracing framework also. It is very similar to the problems with the logging framework, no different in fact.

4. The other curious case of wimax-multicast.cc where changing the position seems to have no effect on range. In other words, there seems to be that line down below saying mobility.Install which overrides any mobility information on top rendering it useless. I broke my head to figure this out.

5. ns-2 Mobility helper should learn to ignore commented lines just as ns2 does when it sources commented tcl lines. This seems to be coded for but not tested. Seriously screw OSS. How do I lay my hands on every bug possible ?

6. ns-2 Mobility Helper does not take in relative file paths as input. This is one major PITA.