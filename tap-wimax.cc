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

int 
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

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

  WimaxHelper::SchedulerType scheduler = WimaxHelper::SCHED_TYPE_SIMPLE;
 
  NodeContainer ssNodes;
  NodeContainer bsNodes;

  ssNodes.Create (2);
  bsNodes.Create (1);
 /****ADDED BY ANIRUDH: MOBILITY INFORMATION***********/
  Ptr<ConstantPositionMobilityModel> BSPosition;
  Ptr<ConstantPositionMobilityModel> SSPosition1;
  Ptr<ConstantPositionMobilityModel> SSPosition2;

  BSPosition = CreateObject<ConstantPositionMobilityModel> ();
  BSPosition->SetPosition  (Vector (1740, 0, 0));
  bsNodes.Get (0)->AggregateObject (BSPosition);

  SSPosition1 = CreateObject<ConstantPositionMobilityModel> (); // UDP server, receiver of packets. 
  SSPosition1->SetPosition (Vector (1745, 0, 0));
  ssNodes.Get (0)->AggregateObject (SSPosition1);

  SSPosition2 = CreateObject<ConstantPositionMobilityModel> ();
  SSPosition2->SetPosition (Vector (1750, 0, 0));
  ssNodes.Get (1)->AggregateObject (SSPosition2);		// object compisition technique 

  WimaxHelper wimax;

  NetDeviceContainer ssDevs, bsDevs;

  ssDevs = wimax.Install (ssNodes,
                          WimaxHelper::DEVICE_TYPE_SUBSCRIBER_STATION,
                          WimaxHelper::SIMPLE_PHY_TYPE_OFDM,
                          scheduler);
  bsDevs = wimax.Install (bsNodes, WimaxHelper::DEVICE_TYPE_BASE_STATION, WimaxHelper::SIMPLE_PHY_TYPE_OFDM, scheduler);

  wimax.EnableAscii ("bs-devices", bsDevs);
  wimax.EnableAscii ("ss-devices", ssDevs);

  Ptr<SubscriberStationNetDevice> ss[2];

  for (int i = 0; i < 2; i++)
    {
      ss[i] = ssDevs.Get (i)->GetObject<SubscriberStationNetDevice> ();
      ss[i]->SetModulationType (WimaxPhy::MODULATION_TYPE_QAM16_12);
    }

  Ptr<BaseStationNetDevice> bs;

  bs = bsDevs.Get (0)->GetObject<BaseStationNetDevice> ();
  //
  // Use the TapBridgeHelper to connect to the pre-configured tap devices for 
  // the left side.  We go with "UseLocal" mode since the wifi devices do not
  // support promiscuous mode (because of their natures0.  This is a special
  // case mode that allows us to extend a linux bridge into ns-3 IFF we will
  // only see traffic from one other device on that bridge.  That is the case
  // for this configuration.
  //
  TapBridgeHelper tapBridge;
  tapBridge.SetAttribute ("Mode", StringValue ("UseLocal"));
  tapBridge.SetAttribute ("DeviceName", StringValue ("tap-baseStation"));
  tapBridge.Install (bsNodes.Get (0), bs);

  //
  // Connect the right side tap to the right side wifi device on the right-side
  // ghost node.
  //
  tapBridge.SetAttribute ("DeviceName", StringValue ("tap-1"));
  tapBridge.Install (ssNodes.Get (0), ssDevs.Get (0));

  tapBridge.SetAttribute ("DeviceName", StringValue ("tap-2"));
  tapBridge.Install (ssNodes.Get (1), ssDevs.Get (1));

  Simulator::Stop (Seconds (600.));
  Simulator::Run ();
  Simulator::Destroy ();
}
