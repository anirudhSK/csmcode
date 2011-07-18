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

// This is for ns-2 trace integration.  


// global variables relevant to the simulation to set up the nodes

#define MAX_NODES 60

WifiHelper wifi;
NqosWifiMacHelper wifiMac;
YansWifiChannelHelper wifiChannel;
YansWifiPhyHelper wifiPhy;

int mobilityMode =-1; 
int terrainDimension=0;
int numberOfNodes=0;

//Node node[MAX_NODES];					// array representing all the nodes in the system
//MobilityHelper mobilityArray[MAX_NODES];		// mobility array 
//ns3::Ptr<ns3::Node> smartNodePointer[MAX_NODES];	// array of smart pointers
//Ptr<ListPositionAllocator> positionAlloc[MAX_NODES];	// array of position allocators 
//NetDeviceContainer devices[MAX_NODES];			// array of network devices 
//// Ns2MobilityHelper ns2mobility[MAX_NODES];		// array of ns2 mobility helpers TODO definitely tackle this problem but a while later.
//TapBridgeHelper tapBridge[MAX_NODES];			// aarray of tap bridge helpers 

//std::string traceFile("/home/anirudh/Desktop/ns-3-dev/examples/tap/synthetic.trace");

void InitializeNetworkComponents() {
  
  // We're going to use 802.11p data channel ,so set up a wifi helper to reflect that.
  wifi = WifiHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211p_SCH); // 802.11p radios 
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate3MbpsBW10MHz")); // want to see if the range improves now 

  // No reason for pesky access points, so we'll use an ad-hoc network.
  wifiMac = NqosWifiMacHelper::Default ();
  wifiMac.SetType ("ns3::AdhocWifiMac");

  // Configure the physcial layer.
  wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());

  wifiPhy.Set ("TxPowerStart", DoubleValue(33));
  wifiPhy.Set ("TxPowerEnd", DoubleValue(33)); // changing the transmit power to refelct higher allowed transmit powers for 802.11p 

}

class NetworkedMobileNode {
	public:
		Node node;				// ns-3's representation of the node that this class represents
		MobilityHelper mobility;		// ns-3's representation of the node's mobility
		ns3::Ptr<ns3::Node> smartNodePointer;	// ns-3's smart pointer representation for the node 
		NetDeviceContainer device;		// ns-3's representation of the device running on this node
		TapBridgeHelper tapBridge;		// ns-3's representation of the tap bridge that this node connects to. 
		int nodeId;				// node's ID  
		int mobilityMode;			// mobility type
	
		NetworkedMobileNode(int nodeId,int mobilityMode) { // constructor
			     this->nodeId=nodeId;
	                     this->mobilityMode=mobilityMode;
		}
		
		void InitializeNetworkedMobileNode() {
			std::cout<<"In Initialize Node on node ID  "<<nodeId<<std::endl;
			smartNodePointer= ns3::Ptr<ns3::Node>::Ptr(&node);
			device = wifi.Install (wifiPhy, wifiMac, smartNodePointer); // connect node with phy and mac to create device
			if(mobilityMode==1) { // constant position mobility fixed nodes
			   // std::cout<<"Terrain side is  " << terrainDimension<<std::endl;  
		   	     Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
   
			     float randomxPosition=((float)rand()/RAND_MAX)*(terrainDimension) ; // All nodes are randomly distributed in a square of side terrainDimension
			     float randomyPosition=((float)rand()/RAND_MAX)*(terrainDimension) ; // All nodes are randomly distributed in a square of side terrainDimension
			     positionAlloc->Add (Vector (randomxPosition,randomyPosition, 0.0));
			 
 			     //std::cout<<"Node : "<<i<<": Node position is x : "<< randomxPosition<<" y : "<<randomyPosition<<endl;
			     mobility.SetPositionAllocator (positionAlloc);
			     mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
   		        }
			else if(mobilityMode==2)  {// ie random mobility model
			     std::cout<<"Terrain dimension is "<<terrainDimension<<std::endl;
			     mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
			      "MinX", DoubleValue (0.0),
			      "MinY", DoubleValue (0.0),
			      "DeltaX", DoubleValue (1),
			      "DeltaY", DoubleValue (1),
			      "GridWidth", UintegerValue (1),
			      "LayoutType", StringValue ("RowFirst"));
   			      mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel","Bounds", RectangleValue (Rectangle (0, terrainDimension, 0, terrainDimension)));
  		        }
			// TODO: Add the option to feed in traces from ns-2 
			mobility.Install(smartNodePointer);
			char tapString[10];
			sprintf(tapString,"tap%d",nodeId);
			//tapBridge.SetAttribute ("Mode", StringValue ("UseLocal"));
			tapBridge.SetAttribute ("DeviceName", StringValue (tapString));
			tapBridge.Install (smartNodePointer, device.Get (0));   // devices is a container but it contains only one object in this case
		}
};

static void InitializeNode(NetworkedMobileNode* mobileNode){
	mobileNode->InitializeNetworkedMobileNode();
}




int main (int argc, char *argv[])
{
//  LogComponentEnable("YansWifiPhy",LOG_ALL);
//  LogComponentEnable("TapBridge",LOG_ALL);
//  LogComponentEnable("RealtimeSimulatorImpl",LOG_ALL);
  srand(time(NULL));
  CommandLine cmd;
  cmd.Parse (argc, argv);
  int simulationTime=0;
  if(argc < 5) {
	std::cout<<"Usage: Enter number of Nodes followed by the size of terrain followed by mobility mode "<<std::endl;
	std::cout<<"1. Constant position, 2. Mobile based on random walk, 3. Trace driven"<<std::endl;
	std::cout<<"Followed by simulation time "<<std::endl;
	exit(0);
  }
  else {
 	numberOfNodes=atoi(argv[1]);
	terrainDimension=atoi(argv[2]);
	mobilityMode=atoi(argv[3]);
	simulationTime=atoi(argv[4]);
  }
  std::string logFile("/home/anirudh/Desktop/ns-3-dev/examples/tap/mobility.log");
  std::ofstream os;
  os.open(logFile.c_str());
  // We are interacting with the outside, real, world.  This means we have to 
  // interact in real-time and therefore means we have to use the real-time
  // simulator and take the time to calculate checksums.
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  // Install the wireless devices onto our ghost nodes.
  // TODO: Fix the UseLocal for br
  InitializeNetworkComponents();
  
  int i=0;
  for(i=0;i<numberOfNodes;i++) {
//       std::cout<<"Now inserting node"<<i<<std::endl;
       NetworkedMobileNode* freshMobileNode=new NetworkedMobileNode(i,mobilityMode);	// create a new networked mobile node object 
       Simulator::Schedule(Seconds(0.1*(i+1)),&InitializeNode,freshMobileNode);
              
  }

  std::cout<<"Simulating for "<<simulationTime<<" seconds "<<std::endl;
  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run();
  Simulator::Destroy ();
}
