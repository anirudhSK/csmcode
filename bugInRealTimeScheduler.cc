/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>
#include <fstream>

#include "ns3/simulator-module.h"
#include "ns3/node-module.h"
#include "ns3/core-module.h"
#include "ns3/wifi-module.h"
#include "ns3/helper-module.h"
#include "ns3/rectangle.h"
#include "ns3/mobility-module.h"
#include "ns3/ns2-mobility-helper.h"
#include <stdlib.h>
#include "ns3/simulator.h"
#include "ns3/nstime.h"

using namespace ns3;

#define MAX_NODES 1
//Node node[MAX_NODES];					// array representing all the nodes in the system


static void InitializeNode(int nodeId) {
std::cout<<"In Initialize Node on node ID  "<<nodeId<<std::endl;
}




int main (int argc, char *argv[])
{
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  int i=0;
  Simulator::Schedule (Seconds (5.0), &InitializeNode, i);
  Simulator::Stop (Seconds (50));
  Simulator::Run();
  Simulator::Destroy ();
}
