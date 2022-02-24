#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/propagation-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv6-flow-classifier.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/point-to-point-module.h"
#include <ns3/lr-wpan-error-model.h>
using namespace ns3;
int tracedNode=1;

/////////////////////////////////////////////

std::string exp_name = "lrwpan";

std::string dir;

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
  ofs_drop <<  curTime << " " << (100.0* tot_drop)/tot_tx_packets << std::endl; // drop ratio (%)
  ofs_deliver <<  curTime << " " << (100.0 * tot_delivery)/tot_tx_packets << std::endl; // delivery ratio (%)
  Simulator::Schedule (Seconds (0.1), &TraceMetrics, monitor);
}

////////////////////////////////////////////

bool tracing = true;

uint32_t nWsnNodes;
int32_t num_flow;

uint16_t sinkPort=9;
double start_time = 0;
double duration = 100.0;

double stop_time;
bool sack = true;
uint16_t nSourceNodes;
Ptr<LrWpanErrorModel>  lrWpanError;
int range_val;

void initialize(int argc, char** argv) {

  uint32_t    maxPackets = 100;
  bool        modeBytes  = true;
  double      minTh = 5;
  double      maxTh = 15;
  uint32_t    queueDiscLimitPackets = 300;
  uint32_t    pktSize = 50;

  // std::string bottleNeckLinkBw = "5Mbps";
  // std::string bottleNeckLinkDelay = "50ms";

  // VARIABLE PARAMETERS
  nSourceNodes = 5; // 3 , 6, 10 , 13 , 18 
  std::string appDataRate = "6000bps"; // 3000/50 -> 60, 6000 -> 120, 10000/50 -> 200, 13000/50 -> 260, 18000/50 -> 360
  num_flow = 100; // 10 , 20 , 30 , 40 , 50
  range_val = 10; // 10 , 20 , 30 , 40 , 50

  exp_name += "-npan-" + std::to_string(nSourceNodes); // node
  exp_name += "-nflow-" + std::to_string(num_flow); // flow
  exp_name += "-app-" + appDataRate; // data rate
  exp_name += "-range-" + std::to_string(range_val); // range

  cout<<exp_name<<endl;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("tracing", "turn on log components", tracing);
  cmd.AddValue ("nSourceNodes", "turn on log components", nSourceNodes);
  cmd.AddValue ("tracedNode", "turn on log components", tracedNode);
  cmd.Parse (argc, argv);

  if( tracing ) {
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
  }

  nWsnNodes = nSourceNodes + 1;
  tracedNode = std::max(1, tracedNode);
  
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (pktSize));
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue (appDataRate));

  //////////////////////////////////
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
  // Config::SetDefault ("ns3::RedQueueDisc::LinkBandwidth", StringValue (bottleNeckLinkBw));
  // Config::SetDefault ("ns3::RedQueueDisc::LinkDelay", StringValue (bottleNeckLinkDelay));
  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (pktSize));
  /////////////////////////////////

  stop_time = start_time + duration;

  lrWpanError = CreateObject<LrWpanErrorModel> ();


  std::cout << "------------------------------------------------------\n"; 
  std::cout << "Source Count: " << nSourceNodes << "\n"; 
  std::cout << "------------------------------------------------------\n"; 
}

int main (int argc, char** argv) {
  initialize(argc, argv);

  Packet::EnablePrinting ();

  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  NodeContainer wsnNodesLeft;
  wsnNodesLeft.Add(p2pNodes.Get(0));
  wsnNodesLeft.Create (nSourceNodes);

  NodeContainer wsnNodesRight;
  wsnNodesRight.Add (p2pNodes.Get (1));
  wsnNodesRight.Create (nSourceNodes);

  // p2p net device
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("50000bps"));
  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);
  
  // lrwpan net device
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                               "MinX", DoubleValue (0.0),
                               "MinY", DoubleValue (0.0),
                               "DeltaX", DoubleValue (2),
                               "DeltaY", DoubleValue (0),
                               "GridWidth", UintegerValue (10),
                               "LayoutType", StringValue ("RowFirst"));
    // mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
    //                            "MinX", DoubleValue (0.0),
    //                            "MinY", DoubleValue (0.0),
    //                            "DeltaX", DoubleValue (80),
    //                            "DeltaY", DoubleValue (80),
    //                            "GridWidth", UintegerValue (5),
    //                            "LayoutType", StringValue ("RowFirst"));                             
  // mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
  //                                "MinX", DoubleValue (0.0),
  //                                "MinY", DoubleValue (0.0),
  //                                "DeltaX", DoubleValue (3),
  //                                "DeltaY", DoubleValue (5),
  //                                "GridWidth", UintegerValue (3),
  //                                "LayoutType", StringValue ("RowFirst"));
  mobility.Install (wsnNodesLeft);
  mobility.Install (wsnNodesRight);

  // creating a channel with range propagation loss model  
  Config::SetDefault ("ns3::RangePropagationLossModel::MaxRange", DoubleValue (range_val*1.0));

  Ptr<SingleModelSpectrumChannel> channelLeft = CreateObject<SingleModelSpectrumChannel> ();
  Ptr<RangePropagationLossModel> propModelLeft = CreateObject<RangePropagationLossModel> ();
  Ptr<ConstantSpeedPropagationDelayModel> delayModelLeft = CreateObject<ConstantSpeedPropagationDelayModel> ();
  channelLeft->AddPropagationLossModel (propModelLeft);
  channelLeft->SetPropagationDelayModel (delayModelLeft);


  Ptr<SingleModelSpectrumChannel> channelRight = CreateObject<SingleModelSpectrumChannel> ();
  Ptr<RangePropagationLossModel> propModelRight = CreateObject<RangePropagationLossModel> ();
  Ptr<ConstantSpeedPropagationDelayModel> delayModelRight = CreateObject<ConstantSpeedPropagationDelayModel> ();
  channelRight->AddPropagationLossModel (propModelRight);
  channelRight->SetPropagationDelayModel (delayModelRight);

  LrWpanHelper lrWpanHelperLeft;
  lrWpanHelperLeft.SetChannel(channelLeft);
  NetDeviceContainer lrwpanDevicesLeft = lrWpanHelperLeft.Install (wsnNodesLeft);
  lrWpanHelperLeft.AssociateToPan (lrwpanDevicesLeft, 0);


  LrWpanHelper lrWpanHelperRight;
  lrWpanHelperRight.SetChannel(channelRight);
  NetDeviceContainer lrwpanDevicesRight = lrWpanHelperRight.Install (wsnNodesRight);
  lrWpanHelperRight.AssociateToPan (lrwpanDevicesRight, 0);


  SixLowPanHelper sixLowPanHelperLeft, sixLowPanHelperRight;
  NetDeviceContainer sixLowPanDevicesLeft = sixLowPanHelperLeft.Install (lrwpanDevicesLeft);
  NetDeviceContainer sixLowPanDevicesRight = sixLowPanHelperRight.Install (lrwpanDevicesRight);

  // IP STACK
  InternetStackHelper internetv6;
  internetv6.Install (wsnNodesLeft);
  internetv6.Install (wsnNodesRight);

  Ipv6AddressHelper ipv6;
  ipv6.SetBase (Ipv6Address ("2001:aaaa::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer p2pDeviceInterfaces;
  p2pDeviceInterfaces = ipv6.Assign (p2pDevices);
  p2pDeviceInterfaces.SetForwarding (1, true);
  p2pDeviceInterfaces.SetDefaultRouteInAllNodes (1);
  p2pDeviceInterfaces.SetForwarding (0, true);
  p2pDeviceInterfaces.SetDefaultRouteInAllNodes (0);

  ipv6.SetBase (Ipv6Address ("2001:bbbb::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer wsnDeviceInterfacesLeft;
  wsnDeviceInterfacesLeft = ipv6.Assign (sixLowPanDevicesLeft);
  wsnDeviceInterfacesLeft.SetForwarding (0, true);
  wsnDeviceInterfacesLeft.SetDefaultRouteInAllNodes (0);

  ipv6.SetBase (Ipv6Address ("2001:cccc::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer wsnDeviceInterfacesRight;
  wsnDeviceInterfacesRight = ipv6.Assign (sixLowPanDevicesRight);
  wsnDeviceInterfacesRight.SetForwarding (0, true);
  wsnDeviceInterfacesRight.SetDefaultRouteInAllNodes (0);

  for (uint32_t i = 0; i < sixLowPanDevicesLeft.GetN (); i++) {
    Ptr<NetDevice> dev = sixLowPanDevicesLeft.Get (i);
    dev->SetAttribute ("UseMeshUnder", BooleanValue (true));
    dev->SetAttribute ("MeshUnderRadius", UintegerValue (10));
  }
  for (uint32_t i = 0; i < sixLowPanDevicesRight.GetN (); i++) {
    Ptr<NetDevice> dev = sixLowPanDevicesRight.Get (i);
    dev->SetAttribute ("UseMeshUnder", BooleanValue (true));
    dev->SetAttribute ("MeshUnderRadius", UintegerValue (10));
  }

  // install queue
  // TrafficControlHelper tchBottleneck;
  // QueueDiscContainer queueDiscRight;
  // tchBottleneck.SetRootQueueDisc ("ns3::RedQueueDisc");
  // queueDiscRight = tchBottleneck.Install (wsnNodesRight.Get (0)->GetDevice(0));


  // Install on/off app on all right side nodes
  OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address ());
  clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
 
  Address sinkLocalAddress (Inet6SocketAddress (Ipv6Address::GetAny (), sinkPort));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);

  ApplicationContainer sinkApps;
  for(int i=1;i<=nSourceNodes;i++) {
    sinkApps.Add (packetSinkHelper.Install (wsnNodesLeft.Get (i)));
  }
  sinkApps.Start (Seconds (1.0));
  sinkApps.Stop (Seconds (90.0));


  ApplicationContainer clientApps;
  int cur_flow_count = 0;
  for (uint32_t i = 1; i <= nSourceNodes; ++i)
    {
      // Create an on/off app on left side node which sends packets to the right side
      AddressValue remoteAddress (Inet6SocketAddress (wsnDeviceInterfacesLeft.GetAddress(i,1), sinkPort));

      // clientHelper.SetAttribute ("Remote", remoteAddress);
      // clientApps.Add (clientHelper.Install (wsnNodesRight.Get(i)));
      for(uint32_t j = 1; j <= nSourceNodes; ++j)
      {
        clientHelper.SetAttribute ("Remote", remoteAddress);
        clientApps.Add (clientHelper.Install (wsnNodesRight.Get(j)));
        cur_flow_count++;

        if(cur_flow_count >= num_flow)
          break;
      }

      if(cur_flow_count >= num_flow)
          break;
    }
  clientApps.Start (Seconds (2.0)); // Start 1 second after sink
  clientApps.Stop (Seconds (80.0)); // Stop before the sink  

  // Flow monitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();

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


  if (tracing) {
    // AsciiTraceHelper ascii;
    // lrWpanHelper.EnableAsciiAll (ascii.CreateFileStream (exp_name + "/red-lrwpan-pan.tr"));
    // lrWpanHelper.EnablePcapAll (exp_name, false);

    // csmaHelper.EnableAsciiAll (ascii.CreateFileStream (exp_name + "/red-lrwpan-csma.tr"));
    // csmaHelper.EnablePcapAll (exp_name, false);

    // Simulator::Schedule (Seconds (0.1), &TraceThroughput, flowMonitor);
    // Simulator::Schedule (Seconds (0.1), &TraceDelay, flowMonitor);
    // Simulator::Schedule (Seconds (0.1), &TraceDrop, flowMonitor);
    // Simulator::Schedule (Seconds (0.1), &TraceDelivery, flowMonitor);
    // Simulator::Schedule (Seconds (0.1), &TraceSent, flowMonitor);
    Simulator::Schedule (Seconds (0.1), &TraceMetrics, flowMonitor);

  }

  Simulator::Stop (Seconds (100));
  Simulator::Run ();

  flowHelper.SerializeToXmlFile (exp_name + "/flow.xml", true, true);

  // QueueDisc::Stats stRight = queueDiscRight.Get (0)->GetStats ();
  // std::cout << "*** Stats from the bottleneck queue disc (Right) ***" << std::endl;
  // std::cout << stRight << std::endl;
  
  std::cout << "Destroying the simulation" << std::endl;

  Simulator::Destroy ();

  return 0;
}

