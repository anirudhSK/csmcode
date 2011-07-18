/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

//
// This is an illustration of how one could use virtualization techniques to
// allow running applications on virtual machines talking over simulated
// networks.
//
// The actual steps required to configure the virtual machines can be rather
// involved, so we don't go into that here.  Please have a look at one of
// our HOWTOs on the nsnam wiki for more details about how to get the 
// system confgured.  For an example, have a look at "HOWTO Use Linux 
// Containers to set up virtual networks" which uses this code as an 
// example.
//
// The configuration you are after is explained in great detail in the 
// HOWTO, but looks like the following:
//
//  +----------+                           +----------+
//  | virtual  |                           | virtual  |
//  |  Linux   |                           |  Linux   |
//  |   Host   |                           |   Host   |
//  |          |                           |          |
//  |   eth0   |                           |   eth0   |
//  +----------+                           +----------+
//       |                                      |
//  +----------+                           +----------+
//  |  Linux   |                           |  Linux   |
//  |  Bridge  |                           |  Bridge  |
//  +----------+                           +----------+
//       |                                      |
//  +------------+                       +-------------+
//  | "tap-left" |                       | "tap-right" |
//  +------------+                       +-------------+
//       |           n0            n1           |
//       |       +--------+    +--------+       |
//       +-------|  tap   |    |  tap   |-------+
//               | bridge |    | bridge |
//               +--------+    +--------+
//               |  wifi  |    |  wifi  |
//               +--------+    +--------+
//                   |             |     
//                 ((*))         ((*))
//
//                       Wifi LAN
//
//                        ((*))
//                          |
//                     +--------+
//                     |  wifi  |
//                     +--------+
//                     | access |
//                     |  point |
//                     +--------+
//
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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TapWifiVirtualMachineExample");

//static void
//CourseChange (std::ostream *os, std::string foo, Ptr<const MobilityModel> mobility)
//{
//  Vector pos = mobility->GetPosition (); // Get position
//  Vector vel = mobility->GetVelocity (); // Get velocity
//
//  // Prints position and velocities
//  *os << Simulator::Now () << " POS: x=" << pos.x << ", y=" << pos.y
//      << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
//      << ", z=" << vel.z << std::endl;
//}
int 
main (int argc, char *argv[])
{

  /*************The below is for inputting ns-2 traces ******************/
  CommandLine cmd;
  cmd.Parse (argc, argv);
  std::string logFile("/home/anirudh/Desktop/ns-3-dev/examples/tap/mobility.log");

  std::ofstream os;
  os.open(logFile.c_str());
  std::string traceFile("/home/anirudh/Desktop/ns-3-dev/examples/tap/synthetic.trace");
  //
  // We are interacting with the outside, real, world.  This means we have to 
  // interact in real-time and therefore means we have to use the real-time
  // simulator and take the time to calculate checksums.
  //
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  //
  // Create two ghost nodes.  The first will represent the virtual machine host
  // on the left side of the network; and the second will represent the VM on 
  // the right side.
  //
  NodeContainer nodes;
  nodes.Create (6);

  //
  // We're going to use 802.11 A so set up a wifi helper to reflect that.
  //
  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate54Mbps"));

  //
  // No reason for pesky access points, so we'll use an ad-hoc network.
  //
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifiMac.SetType ("ns3::AdhocWifiMac");

  //
  // Configure the physcial layer.
  //
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());

  //
  // Install the wireless devices onto our ghost nodes.
  //
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodes);

//  CsmaHelper csma;
//  NetDeviceContainer devices = csma.Install (nodes);

  //
  // We need location information since we are talking about wifi, so add a
  // constant position to the ghost nodes.
  //


  /*****************Constant position mobility models************************/
   MobilityHelper mobility;
//  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
//  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
//  positionAlloc->Add (Vector (20.0, 0.0, 0.0));
//  positionAlloc->Add (Vector (0.0, 20.0, 0.0));
//  positionAlloc->Add (Vector (20.0, 20.0, 0.0));
//  positionAlloc->Add (Vector (10.0, 10.0, 0.0));
//  positionAlloc->Add (Vector (20.0, 10.0, 0.0));
//  mobility.SetPositionAllocator (positionAlloc);
//  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
//  mobility.Install (nodes);

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
    "MinX", DoubleValue (0.0),
    "MinY", DoubleValue (0.0),
    "DeltaX", DoubleValue (2),
    "DeltaY", DoubleValue (2),
    "GridWidth", UintegerValue (1),
    "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
    "Bounds", RectangleValue (Rectangle (0, 40, 0, 40)));
//
// mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

//  Ns2MobilityHelper mobility(traceFile);

 mobility.Install (nodes); 

//  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",MakeBoundCallback (&CourseChange, &os));


  mobility.EnableAsciiAll(std::cout);


  //
  // Use the TapBridgeHelper to connect to the pre-configured tap devices for 
  // the left side.  We go with "UseLocal" mode since the wifi devices do not
  // support promiscuous mode (because of their natures0.  This is a special
  // case mode that allows us to extend a linux bridge into ns-3 IFF we will
  // only see traffic from one other device on that bridge.  That is the case
  // for this configuration.
  //
  TapBridgeHelper tapBridge;
  tapBridge.SetAttribute ("Mode", StringValue ("UseLocal")); // TODO Difference between useBridge and useLocal 
  tapBridge.SetAttribute ("DeviceName", StringValue ("tap1"));
  tapBridge.Install (nodes.Get (0), devices.Get (0));

  tapBridge.SetAttribute ("DeviceName", StringValue ("tap2"));
  tapBridge.Install (nodes.Get (1), devices.Get (1));

  tapBridge.SetAttribute ("DeviceName", StringValue ("tap3"));
  tapBridge.Install (nodes.Get (2), devices.Get (2));

  tapBridge.SetAttribute ("DeviceName", StringValue ("tap4"));
  tapBridge.Install (nodes.Get (3), devices.Get (3));

  tapBridge.SetAttribute ("DeviceName", StringValue ("tap5"));
  tapBridge.Install (nodes.Get (4), devices.Get (4));

  tapBridge.SetAttribute ("DeviceName", StringValue ("tap6"));
  tapBridge.Install (nodes.Get (5), devices.Get (5));

  // Run the simulation for ten minutes to give the user time to play around
  //
  Simulator::Stop (Seconds (600.));
  Simulator::Run ();
  Simulator::Destroy ();
}
