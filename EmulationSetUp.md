# Introduction #

Setting up the ns-3 emulation needs some tasks to be finished. Here they are.


# Details #
List of action items

  1. Fixing the interaction between mobility logging and real time simulator.
  1. Increasing the range of the wireless interface using a lower bit rate.
  1. Calibrating wifi against the real world to make sure that the power numbers and the range are consistent ie 16.0206 dbm actually gives only 25 metres or so. Real world data seems better.
  1. Setting up logging frameworks and trace processing frameworks for automatic collection of statistics.
  1. Instrumenting nodes and / or logging events to figure out battery consumption.
  1. Integrating WiMax or LTE to simulate cloud accesses.
  1. Emulating or simulating a cloud server somewhere out there.
  1. scripts to automate all of the above.
  1. Limits of real time scalability.
  1. mpi implementations to speed up the simulation.
  1. A tiny app to test this all out while waiting for the CSM port.