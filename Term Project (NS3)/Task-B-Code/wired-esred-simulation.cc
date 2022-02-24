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
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <iomanip>
#include <map>

using namespace ns3;

std::string exp_name = "wired-ES-red";

uint32_t prev_b = 0;
Time prevTime = Seconds (0);

// Calculate throughput
static void
TraceThroughput (Ptr<FlowMonitor> monitor)
{
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  auto itr = stats.begin ();
  Time curTime = Now ();
  std::ofstream thr (exp_name+"/throughput_pertime.dat", std::ios::out | std::ios::app);
  thr <<  curTime << " " << (8 * (itr->second.txBytes - prev_b)) / (1e6 * (curTime.GetSeconds () - prevTime.GetSeconds ())) << std::endl;
  prevTime = curTime;
  prev_b = itr->second.txBytes;
  Simulator::Schedule (Seconds (0.1), &TraceThroughput, monitor);
}

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
  ofs_thp <<  curTime << " " << (tot_thr*1.0)/(1e6) << std::endl; // throughput (bit/s)
  ofs_delay <<  curTime << " " << tot_delay/tot_rx_packets << std::endl; // delay (s)
  ofs_drop <<  curTime << " " << (100.0* tot_drop)/(tot_rx_packets+tot_drop) << std::endl; // drop ratio (%)
  ofs_deliver <<  curTime << " " << (100.0 * tot_delivery)/(tot_rx_packets+tot_drop) << std::endl; // delivery ratio (%)
  Simulator::Schedule (Seconds (0.1), &TraceMetrics, monitor);
}

static void
TraceQueue (Ptr<QueueDisc> queue)
{
  uint32_t qSize = queue->GetCurrentSize ().GetValue ();
  uint32_t qMaxSize = queue->GetMaxSize ().GetValue (); 
  double bufferOccupancy = qSize * 100.0 / (double) qMaxSize; 
  std::ofstream q (exp_name+"/bufferOccupancy.dat", std::ios::out | std::ios::app);
  q << Simulator::Now ().GetSeconds () << " " << qSize << " " << bufferOccupancy << " "<<qMaxSize<<std::endl;
  Simulator::Schedule (Seconds (0.01), &TraceQueue, queue);
}

// trace queue drop rate
static void
TraceQueueDrop (Ptr<QueueDisc> queue)
{
  QueueDisc::Stats st = queue->GetStats ();
  double dropRatio = st.nTotalDroppedPackets * 100.0 / st.nTotalReceivedPackets; 
  std::ofstream q (exp_name+"/queueDropRate.dat", std::ios::out | std::ios::app);
  q << Simulator::Now ().GetSeconds () << " " << dropRatio <<std::endl;
  Simulator::Schedule (Seconds (0.01), &TraceQueueDrop, queue);
}

int main (int argc, char *argv[])
{
  uint32_t    nLeaf = 10;
  uint32_t    maxPackets = 100;
  bool        modeBytes  = true; // byte mode

  // queue disc limit * pktSize ~ 0.5 Mytes
  uint32_t    queueDiscLimitPackets = 977;
  uint32_t    pktSize = 512;

  std::string appDataRate = "650Mbps";
  uint16_t port = 5001;
  std::string bottleNeckLinkBw = "45Mbps";
  std::string bottleNeckLinkDelay = "1ms";

  exp_name += "-nleaf-" + std::to_string(nLeaf); // node
  exp_name += "-app-" + appDataRate; // data rate

  cout<<exp_name<<endl;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("nLeaf",     "Number of left and right side leaf nodes", nLeaf);
  cmd.AddValue ("maxPackets","Max Packets allowed in the device queue", maxPackets);
  cmd.AddValue ("queueDiscLimitPackets","Max Packets allowed in the queue disc", queueDiscLimitPackets);
  cmd.AddValue ("appPktSize", "Set OnOff App Packet Size", pktSize);
  cmd.AddValue ("appDataRate", "Set OnOff App DataRate", appDataRate);
  cmd.AddValue ("modeBytes", "Set Queue disc mode to Packets (false) or bytes (true)", modeBytes);

  cmd.Parse (argc,argv);

  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (pktSize));
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue (appDataRate));

  Config::SetDefault ("ns3::DropTailQueue<Packet>::MaxSize",
                      StringValue (std::to_string (maxPackets) + "p"));

  if (!modeBytes)
    {
      Config::SetDefault ("ns3::ESRedQueueDisc::MaxSize",
                          QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, queueDiscLimitPackets)));
    }
  else
    {
      Config::SetDefault ("ns3::ESRedQueueDisc::MaxSize",
                          QueueSizeValue (QueueSize (QueueSizeUnit::BYTES, queueDiscLimitPackets * pktSize)));
    }

  ////////////////////////////////////////////////////////////////////////////////////

  // Create the point-to-point link helpers
  PointToPointHelper bottleNeckLink;
  bottleNeckLink.SetDeviceAttribute  ("DataRate", StringValue (bottleNeckLinkBw));
  bottleNeckLink.SetChannelAttribute ("Delay", StringValue (bottleNeckLinkDelay));

  PointToPointHelper pointToPointLeaf;
  pointToPointLeaf.SetDeviceAttribute    ("DataRate", StringValue ("10Mbps"));
  pointToPointLeaf.SetChannelAttribute   ("Delay", StringValue ("1ms"));


  PointToPointDumbbellHelper d (nLeaf, pointToPointLeaf,
                                nLeaf, pointToPointLeaf,
                                bottleNeckLink);

  ////////////////////////////////////////////////////////////////////////////////////

  // Install Stack
  InternetStackHelper stack;
  for (uint32_t i = 0; i < d.LeftCount (); ++i)
    {
      stack.Install (d.GetLeft (i));
    }
  for (uint32_t i = 0; i < d.RightCount (); ++i)
    {
      stack.Install (d.GetRight (i));
    }

  stack.Install (d.GetLeft ());
  stack.Install (d.GetRight ());
  TrafficControlHelper tchBottleneck;
  QueueDiscContainer queueDiscs;
  tchBottleneck.SetRootQueueDisc ("ns3::ESRedQueueDisc");
  tchBottleneck.Install (d.GetLeft ()->GetDevice (0));
  queueDiscs = tchBottleneck.Install (d.GetRight ()->GetDevice (0));

  // Assign IP Addresses
  d.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"),
                         Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),
                         Ipv4AddressHelper ("10.3.1.0", "255.255.255.0"));

  // Install on/off app on all right side nodes
  OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address ());
  // clientHelper.SetAttribute ("OnTime", StringValue ("ns3::UniformRandomVariable[Min=0.|Max=1.]"));
  // clientHelper.SetAttribute ("OffTime", StringValue ("ns3::UniformRandomVariable[Min=0.|Max=1.]"));
  clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApps;
  for (uint32_t i = 0; i < d.LeftCount (); ++i)
    {
      sinkApps.Add (packetSinkHelper.Install (d.GetLeft (i)));
    }
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (6.0));

  ApplicationContainer clientApps;
  for (uint32_t i = 0; i < d.RightCount (); ++i)
    {
      // Create an on/off app sending packets to the left side
      AddressValue remoteAddress (InetSocketAddress (d.GetLeftIpv4Address (i), port));
      clientHelper.SetAttribute ("Remote", remoteAddress);
      clientApps.Add (clientHelper.Install (d.GetRight (i)));
    }
  clientApps.Start (Seconds (1.0)); // Start 1 second after sink
  clientApps.Stop (Seconds (5.0)); // Stop before the sink

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

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

  // Flow monitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();
  Simulator::Schedule (Seconds (0.1), &TraceMetrics, flowMonitor);
  Simulator::Schedule (Seconds (0.1), &TraceQueue, queueDiscs.Get (0));
  Simulator::Schedule (Seconds (0.1), &TraceQueueDrop, queueDiscs.Get (0));

  Simulator::Stop (Seconds (6.5)); // force stop,
  std::cout << "Running the simulation" << std::endl;
  Simulator::Run ();

  QueueDisc::Stats st = queueDiscs.Get (0)->GetStats ();

  // if (st.GetNDroppedPackets (RedQueueDisc::UNFORCED_DROP) == 0)
  //   {
  //     std::cout << "There should be some unforced drops" << std::endl;
  //     exit (1);
  //   }

  // if (st.GetNDroppedPackets (QueueDisc::INTERNAL_QUEUE_DROP) != 0)
  //   {
  //     std::cout << "There should be zero drops due to queue full" << std::endl;
  //     exit (1);
  //   }

  std::cout << "*** Stats from the bottleneck queue disc ***" << std::endl;
  std::cout << st << std::endl;
  std::cout << "Destroying the simulation" << std::endl;

  Simulator::Destroy ();
  return 0;
}
