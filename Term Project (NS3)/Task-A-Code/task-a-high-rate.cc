/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 NITK Surathkal
 *
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
 *
 * Author: Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-module.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <iomanip>
#include <map>

using namespace ns3;


std::string exp_name = "wifi";
// calculate metrics
static void
TraceMetrics (Ptr<FlowMonitor> monitor)
{
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  std::ofstream ofs_thp (exp_name+"/throughput.dat", std::ios::out | std::ios::app);
  std::ofstream ofs_delay (exp_name+"/delay.dat", std::ios::out | std::ios::app);
  std::ofstream ofs_drop (exp_name+"/drop.dat", std::ios::out | std::ios::app);
  std::ofstream ofs_deliver (exp_name+"/delivery.dat", std::ios::out | std::ios::app);
  Time curTime = Now ();
  
  // threshold
  double tot_thr = 0;
  // delay
  double tot_delay = 0;
  double tot_rx_packets = 0;
  // drop and delivery
  double tot_tx_packets = 0;
  double tot_drop = 0;
  double tot_delivery = 0;
  // total sent
  double tot_sent = 0;
  int num_flows = 0;
  for(auto itr:stats)
  {
    // threshold
    tot_thr += (8 * itr.second.rxBytes ) / (1.0 * curTime.GetSeconds () );
    // delay
    tot_delay += itr.second.delaySum.GetSeconds ();
    tot_rx_packets += itr.second.rxPackets;
    // drop and delivery
    tot_tx_packets += itr.second.txPackets;
    tot_drop += itr.second.lostPackets;
    tot_delivery += itr.second.rxPackets;
    tot_sent += itr.second.txPackets;
    num_flows++;
  }
  ofs_thp <<  curTime << " " << tot_thr << std::endl; // throughput (bit/s)
  ofs_delay <<  curTime << " " << tot_delay/tot_rx_packets << std::endl; // delay (s)
  ofs_drop <<  curTime << " " << (100.0* tot_drop)/(tot_rx_packets+tot_drop) << std::endl; // drop ratio (%)
  ofs_deliver <<  curTime << " " << (100.0 * tot_delivery)/(tot_rx_packets+tot_drop) << std::endl; // delivery ratio (%)
  Simulator::Schedule (Seconds (0.1), &TraceMetrics, monitor);
}


NS_LOG_COMPONENT_DEFINE ("MyRedAredWifiExample");

int main (int argc, char *argv[])
{
  // uint32_t    nLeaf = 10;
  uint32_t    maxPackets = 100;
  bool        modeBytes  = true;

  double      minTh = 5;
  double      maxTh = 15;
  uint32_t    pktSize = 500;
  uint32_t    queueDiscLimitPackets = 300;

  std::string queueDiscType = "RED";
  uint16_t port = 5001;
  std::string bottleNeckLinkBw = "5Mbps";
  std::string bottleNeckLinkDelay = "50ms";


  // VARIABLE PARAMETERS
  uint32_t nWifi = 10; // 3 , 6, 10 , 13 , 18 
  std::string appDataRate = "10Mbps"; // 2 5 10 15 20
  int32_t num_flow = 100; // 10 , 20 , 40 , 70 , 100  -----> x 2
  int32_t speed = 10; // 10 , 20 , 30 , 40 , 50

  exp_name += "-nwifi-" + std::to_string(nWifi); // node
  exp_name += "-nflow-" + std::to_string(num_flow); // flow
  exp_name += "-app-" + appDataRate; // data rate
  exp_name += "-speed-" + std::to_string(speed); // range

  cout<<exp_name<<endl;


  CommandLine cmd (__FILE__);
  // cmd.AddValue ("nLeaf",     "Number of left and right side leaf nodes", nLeaf);
  cmd.AddValue ("maxPackets","Max Packets allowed in the device queue", maxPackets);
  cmd.AddValue ("queueDiscLimitPackets","Max Packets allowed in the queue disc", queueDiscLimitPackets);
  cmd.AddValue ("queueDiscType", "Set Queue disc type to RED or ARED", queueDiscType);
  cmd.AddValue ("appPktSize", "Set OnOff App Packet Size", pktSize);
  cmd.AddValue ("appDataRate", "Set OnOff App DataRate", appDataRate);
  cmd.AddValue ("modeBytes", "Set Queue disc mode to Packets (false) or bytes (true)", modeBytes);

  cmd.AddValue ("redMinTh", "RED queue minimum threshold", minTh);
  cmd.AddValue ("redMaxTh", "RED queue maximum threshold", maxTh);
  cmd.Parse (argc,argv);

  if ((queueDiscType != "RED") && (queueDiscType != "ARED"))
    {
      std::cout << "Invalid queue disc type: Use --queueDiscType=RED or --queueDiscType=ARED" << std::endl;
      exit (1);
    }

  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (pktSize));
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue (appDataRate));

  Config::SetDefault ("ns3::DropTailQueue<Packet>::MaxSize",
                      StringValue (std::to_string (maxPackets) + "p"));

  if (!modeBytes)
    {
      Config::SetDefault ("ns3::RedQueueDisc::MaxSize",
                          QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscLimitPackets)));
    }
  else
    {
      Config::SetDefault ("ns3::RedQueueDisc::MaxSize",
                          QueueSizeValue (QueueSize (QueueSizeUnit::BYTES, queueDiscLimitPackets * pktSize)));
      minTh *= pktSize;
      maxTh *= pktSize;
    }

  Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (minTh));
  Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (maxTh));
  Config::SetDefault ("ns3::RedQueueDisc::LinkBandwidth", StringValue (bottleNeckLinkBw));
  Config::SetDefault ("ns3::RedQueueDisc::LinkDelay", StringValue (bottleNeckLinkDelay));
  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (pktSize));

  if (queueDiscType == "ARED")
    {
      // Turn on ARED
      Config::SetDefault ("ns3::RedQueueDisc::ARED", BooleanValue (true));
      Config::SetDefault ("ns3::RedQueueDisc::LInterm", DoubleValue (10.0));
    }

  /**
   * @brief 2 point to point nodes, which later will work as AP nodes for wifi
   * 
   */

  NodeContainer p2pBottleNeckNodes;
  p2pBottleNeckNodes.Create (2);

  // Create the point-to-point link helpers
  PointToPointHelper bottleNeckLink;
  bottleNeckLink.SetDeviceAttribute  ("DataRate", StringValue (bottleNeckLinkBw));
  bottleNeckLink.SetChannelAttribute ("Delay", StringValue (bottleNeckLinkDelay));

  NetDeviceContainer p2pBottleNeckDevices;
  p2pBottleNeckDevices = bottleNeckLink.Install (p2pBottleNeckNodes);

  /**
   * @brief wifi left + right
   * 
   */

  NodeContainer wifiStaNodesLeft,wifiStaNodesRight;
  wifiStaNodesLeft.Create (nWifi);
  wifiStaNodesRight.Create (nWifi);
  NodeContainer wifiApNodeLeft = p2pBottleNeckNodes.Get (0);
  NodeContainer wifiApNodeRight = p2pBottleNeckNodes.Get (1);

  // PHY helper
  // constructs the wifi devices and the interconnection channel between these wifi nodes.
  YansWifiChannelHelper channelLeft = YansWifiChannelHelper::Default ();
  YansWifiChannelHelper channelRight = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phyLeft,phyRight;
  phyLeft.SetChannel (channelLeft.Create ()); //  all the PHY layer objects created by the YansWifiPhyHelper share the same underlying channel
  phyRight.SetChannel (channelRight.Create ()); 

  // MAC layer
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager"); // tells the helper the type of rate control algorithm to use, here AARF algorithm

  WifiMacHelper macLeft,macRight;
  Ssid ssidLeft = Ssid ("ns-3-ssid-left"); //  creates an 802.11 service set identifier (SSID) object
  Ssid ssidRight = Ssid ("ns-3-ssid-right"); 
  
  // configure Wi-Fi for all of our STA nodes
  macLeft.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssidLeft),
               "ActiveProbing", BooleanValue (false)); 
               
  macRight.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssidRight),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevicesLeft, staDevicesRight;
  staDevicesLeft = wifi.Install (phyLeft, macLeft, wifiStaNodesLeft);
  staDevicesRight = wifi.Install (phyRight, macRight, wifiStaNodesRight);

  //  configure the AP (access point) node
  macLeft.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssidLeft));
  macRight.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssidRight));

  NetDeviceContainer apDevicesLeft, apDevicesRight;
  apDevicesLeft = wifi.Install (phyLeft, macLeft, wifiApNodeLeft); // single AP which shares the same set of PHY-level Attributes (and channel) as the station
  apDevicesRight = wifi.Install (phyRight, macRight, wifiApNodeRight); 


  /**
   * 
   * Mobility Model
   * 
   * We want the STA nodes to be mobile, wandering around inside a bounding box, 
   * and we want to make the AP node stationary
   * 
   **/

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (0.5),
                                 "DeltaY", DoubleValue (1.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  // // tell STA nodes how to move
  // mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
  //                            "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));  
  
  // // tell STA nodes how to move
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant="+std::to_string(speed)+"]"));
  
  // install on STA nodes
  mobility.Install (wifiStaNodesLeft);
  mobility.Install (wifiStaNodesRight);

  // tell AP node to stay still
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  // install on AP node
  mobility.Install (wifiApNodeLeft);
  mobility.Install (wifiApNodeRight);

  // Install Stack
  InternetStackHelper stack;
  stack.Install (wifiApNodeLeft);
  stack.Install (wifiApNodeRight);
  stack.Install (wifiStaNodesLeft);
  stack.Install (wifiStaNodesRight);

  // install queue
  TrafficControlHelper tchBottleneck;
  QueueDiscContainer queueDiscLeft, queueDiscRight;
  tchBottleneck.SetRootQueueDisc ("ns3::RedQueueDisc");
  queueDiscRight = tchBottleneck.Install (wifiApNodeRight.Get(0)->GetDevice(0));

  // Assign IP Addresses
  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pBottleNeckDevices);

  Ipv4InterfaceContainer staNodeInterfacesLeft, staNodeInterfacesRight;
  Ipv4InterfaceContainer apNodeInterfaceLeft, apNodeInterfaceRight;

  address.SetBase ("10.1.2.0", "255.255.255.0");
  staNodeInterfacesLeft = address.Assign (staDevicesLeft);
  apNodeInterfaceLeft = address.Assign (apDevicesLeft);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  staNodeInterfacesRight = address.Assign (staDevicesRight);
  apNodeInterfaceRight = address.Assign (apDevicesRight);


  // Install on/off app on all right side nodes
  OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address ());
  // clientHelper.SetAttribute ("OnTime", StringValue ("ns3::UniformRandomVariable[Min=0.|Max=1.]"));
  // clientHelper.SetAttribute ("OffTime", StringValue ("ns3::UniformRandomVariable[Min=0.|Max=1.]"));
  clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
 
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);

  ApplicationContainer sinkApps;
  for (uint32_t i = 0; i < nWifi; ++i)
    {
      // create sink app on left side node
      sinkApps.Add (packetSinkHelper.Install (wifiStaNodesLeft.Get(i)));
    }
  sinkApps.Start (Seconds (1.0));
  sinkApps.Stop (Seconds (6.5));

  // sinkApps.Start (Seconds (0.0));
  // sinkApps.Stop (Seconds (30.0));

  ApplicationContainer clientApps;
  int cur_flow_count = 0;
  for (uint32_t i = 0; i < nWifi; ++i)
    {
      // Create an on/off app on right side node which sends packets to the left side
      AddressValue remoteAddress (InetSocketAddress (staNodeInterfacesLeft.GetAddress(i), port));
      
      for(uint32_t j = 0; j < nWifi; ++j)
      {
        clientHelper.SetAttribute ("Remote", remoteAddress);
        clientApps.Add (clientHelper.Install (wifiStaNodesRight.Get(j)));

        cur_flow_count++;
        if(cur_flow_count >= num_flow)
          break;
      }

      if(cur_flow_count >= num_flow)
          break;

      // clientHelper.SetAttribute ("Remote", remoteAddress);
      // clientApps.Add (clientHelper.Install (wifiStaNodesRight.Get(i)));
    }
  clientApps.Start (Seconds (2.0)); // Start 1 second after sink
  clientApps.Stop (Seconds (6.0)); // Stop before the sink  
  // clientApps.Start (Seconds (1.0)); // Start 1 second after sink
  // clientApps.Stop (Seconds (17.0)); // Stop before the sink

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (6.5)); // force stop,

  std::string dirToSave = "mkdir -p " + exp_name;
  std::string dirToDel = "rm -rf " + exp_name;
  if (system (dirToDel.c_str ()) == -1)
  {
      exit (1);
  }
  if (system (dirToSave.c_str ()) == -1)
  {
      exit (1);
  }

  // AsciiTraceHelper ascii;
  // bottleNeckLink.EnableAsciiAll (ascii.CreateFileStream (exp_name+"/red-wifi.tr"));
  // bottleNeckLink.EnablePcapAll (exp_name+"/red-wifi");

  // Flow monitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();
  Simulator::Schedule (Seconds (2.2), &TraceMetrics, flowMonitor);

  std::cout << "Running the simulation :( " <<exp_name<< std::endl;
  Simulator::Run ();

  flowMonitor->SerializeToXmlFile(exp_name+"/flow.xml", true, true);

  QueueDisc::Stats stRight = queueDiscRight.Get (0)->GetStats ();

  if (stRight.GetNDroppedPackets (RedQueueDisc::UNFORCED_DROP) == 0)
    {
      std::cout << stRight << std::endl;
      std::cout << "There should be some unforced drops (RIGHT)" << std::endl;
      exit (1);
    }

  if (stRight.GetNDroppedPackets (QueueDisc::INTERNAL_QUEUE_DROP) != 0)
    {
      std::cout << "There should be zero drops due to queue ful; (RIGHT)" << std::endl;
      exit (1);
    }

  std::cout << "*** Stats from the bottleneck queue disc (RIGHT) ***" << std::endl;
  std::cout << stRight << std::endl;
  
  std::cout << "Destroying the simulation" << std::endl;

  Simulator::Destroy ();
  return 0;
}