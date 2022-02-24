// /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright © 2022 Md. Zarif Ul Alam
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
 * Author: Md. Zarif Ul Alam
 *
 */

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "ns3/drop-tail-queue.h"
#include "es-red-queue-disc.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ESRedQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (ESRedQueueDisc);

TypeId
ESRedQueueDisc::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::ESRedQueueDisc")
      .SetParent<QueueDisc> ()
      .SetGroupName ("TrafficControl")
      .AddConstructor<ESRedQueueDisc> ()
      .AddAttribute ("OverwriteProbability", "Probability of overwriting zombie list",
                      DoubleValue (0.25),
                      MakeDoubleAccessor (&ESRedQueueDisc::p_overwrite),
                      MakeDoubleChecker<double> (0, 1))
      .AddAttribute ("MaximumDropProbability", "maximum probability of dropping a packet",
                      DoubleValue (0.15), MakeDoubleAccessor (&ESRedQueueDisc::p_max),
                      MakeDoubleChecker<double> (0, 1))
      .AddAttribute ("MaxSize",
                "The maximum number of packets accepted by this queue disc",
                QueueSizeValue (QueueSize ("25p")),
                MakeQueueSizeAccessor (&QueueDisc::SetMaxSize,
                                      &QueueDisc::GetMaxSize),
                MakeQueueSizeChecker ())
      .AddAttribute ("StabilizedRedMode", "Simple  or Full", IntegerValue (1),
                      MakeIntegerAccessor (&ESRedQueueDisc::stabilizedRedMode),
                      MakeIntegerChecker<int32_t> ());
  return tid;
}

ESRedQueueDisc::ESRedQueueDisc ()
    : QueueDisc (QueueDiscSizePolicy::SINGLE_INTERNAL_QUEUE)
{
  NS_LOG_FUNCTION (this);
  m_uv = CreateObject<UniformRandomVariable> ();
}

ESRedQueueDisc::~ESRedQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

void
ESRedQueueDisc::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_uv = 0;
  QueueDisc::DoDispose ();
}

int64_t
ESRedQueueDisc::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_uv->SetStream (stream);
  return 1;
}

void
ESRedQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Initializing ES RED params.");
  zombies = vector<EZombie> ();
  p_hitFreq = 0;
  alpha = p_overwrite / MAX_ZOMBIE_LIST_SIZE;
}

// calculate p_sred
double
ESRedQueueDisc::calculateProbabilityStabilizedRed()
{
    uint32_t bufferCapacity = GetInternalQueue (0)->GetMaxSize ().GetValue ();
    uint32_t q = GetInternalQueue (0)->GetNBytes ();

    double bufferCapacity_3 = (double) bufferCapacity / 3.0;
    double bufferCapacity_6 = (double) bufferCapacity / 6.0;

    // if 1/3 rd buffer capacity <= queue <= buffer capacity, then p_sred = p_max
    if(bufferCapacity_3 <= q && q <= bufferCapacity)
    {
        return p_max;
    }
    // if 1/6 th buffer capacity <= queue < 1/3 rd buffer capacity, then p_sred = p_max/4
    else if(bufferCapacity_6 <= q && q < bufferCapacity_3)
    {
        return p_max/4;
    }
    // else p_sred = 0
    else
    {
        return 0;
    }
}

// calculate p_zap simple
double
ESRedQueueDisc::calculateProbabilityZapSimple(double probabilityStabilizedRed)
{
    double multiply = 1.0/(256*p_hitFreq*p_hitFreq);

    if(multiply < 1)
    {
        return probabilityStabilizedRed * multiply;
    }
    else
    {
        return probabilityStabilizedRed;
    }
}

// calculate p_zap full
double
ESRedQueueDisc::calculateProbabilityZap(double probabilityStabilizedRed,int32_t hit)
{
    double multiply1 = 1.0/(256*p_hitFreq*p_hitFreq);
    double multiply2 = 1 + (hit/p_hitFreq);

    if(multiply1 < 1)
    {
        return probabilityStabilizedRed * multiply1 * multiply2;
    }
    else
    {
        return probabilityStabilizedRed * multiply2;
    }
}

bool
ESRedQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);

  uint32_t flowHash;
  if (GetNPacketFilters () == 0)
  {
    flowHash = item->Hash (0);
    // cout<<"flowHash: "<<flowHash<<endl;
  }
  else
  {
    int32_t ret = Classify (item);

    if (ret != PacketFilter::PF_NO_MATCH)
      {
        flowHash = static_cast<uint32_t> (ret);
      }
    else
      {
        // cout<<"Packet Filter No Match"<<endl;
        NS_LOG_ERROR ("No filter has been able to classify this packet, drop it.");
        DropBeforeEnqueue (item, "Packet filter no match");
        return false;
      }
  }

  int32_t flowID = flowHash;

  uint32_t nQueued = GetInternalQueue (0)->GetCurrentSize ().GetValue ();
  uint32_t capacity = GetInternalQueue (0)->GetMaxSize ().GetValue ();

  if (flowID == -1)
    {
      DropBeforeEnqueue (item, "InvalidFlowID");
    }

  int32_t curZombieSize = zombies.size ();
  // cout<<"curZombieSize: "<<curZombieSize<<endl;
  // cout<<"nQueued: "<<nQueued<<" , Capacity "<<capacity<< ", Percent : "<< nQueued*100.0/capacity<<endl;

  if (curZombieSize < MAX_ZOMBIE_LIST_SIZE) // still has space in zombie list
    {
      EZombie zombie;
      zombie.flowID = flowID;
      zombie.count = 0;
      zombie.time = Simulator::Now ();
      zombies.push_back (zombie);

      // if adding packet size doesn't exceed capacity of queue, then enqueue
        if (nQueued + item->GetSize () <= capacity)
        {
            NS_LOG_DEBUG ("Enqueueing " << item->GetPacket ()->GetUid () << " to queue " << (uint32_t) flowID);
            return GetInternalQueue (0)->Enqueue (item);
        }
        else
        {
            // if adding packet size exceeds capacity of queue, then drop packet
            NS_LOG_DEBUG ("Dropping " << item->GetPacket ()->GetUid () << " due to queue overflow");
            DropBeforeEnqueue (item, "QUEUE_OVERFLOW");
            return false;
        }
    }
  else // no more space in zombie list
    {
      // choose a random zombie
      uint32_t index = m_uv->GetInteger (0, curZombieSize - 1);
      int32_t hit = 0;

      if (zombies[index].flowID == flowID) // HIT
        {
          hit = 1;
          zombies[index].count++;
          zombies[index].time = Simulator::Now ();
        }
      else // not HIT
        {
          // get a random value and compare with p_overwrite
          double rand = m_uv->GetValue ();
          double p_overwrite_e = p_overwrite/(1+zombies[index].time.GetSeconds());
          if (rand < p_overwrite_e)
            {
              zombies[index].flowID = flowID;
              zombies[index].count = 0;
              zombies[index].time = Simulator::Now ();
            }
        }

      // update hit frequency
      p_hitFreq = (1 - alpha) * p_hitFreq + alpha * hit;    
    }

    // calculate p_sred
    double probabilityStabilizedRed = calculateProbabilityStabilizedRed();

    // calculate p_zap
    double probabilityZap = 0;
    if(stabilizedRedMode == 1)
    {
        probabilityZap = calculateProbabilityZapSimple(probabilityStabilizedRed);
    }
    else if(stabilizedRedMode == 2)
    {
        probabilityZap = calculateProbabilityZap(probabilityStabilizedRed,1);
    }

    // get a random value and compare with p_zap
    double rand = m_uv->GetValue ();
    if (rand < probabilityZap)
      {
        // cout<<"Drop"<<endl;
        DropBeforeEnqueue (item, "Zap");
        return false;
      }
    else
      {
        if(nQueued < capacity)
        {
            return GetInternalQueue (0)->Enqueue (item);
        }
        else
        {
            DropBeforeEnqueue (item, "Overflow");
            return false;
        }
      }
}


Ptr<QueueDiscItem>
ESRedQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  if (GetInternalQueue (0)->IsEmpty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }
  else
    {
      Ptr<QueueDiscItem> item = GetInternalQueue (0)->Dequeue ();

      NS_LOG_LOGIC ("Popped " << item);

      NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
      NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

      return item;
    }
}

Ptr<const QueueDiscItem>
ESRedQueueDisc::DoPeek (void)
{
  NS_LOG_FUNCTION (this);
  if (GetInternalQueue (0)->IsEmpty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<const QueueDiscItem> item = GetInternalQueue (0)->Peek ();

  NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
  NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

  return item;
}

bool
ESRedQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      cout<<"ESRedQueueDisc cannot have classes"<<endl;
      NS_LOG_ERROR ("ESRedQueueDisc cannot have classes");
      return false;
    }

  // if (GetNPacketFilters () == 0)
  //   {
  //     cout<<"ESRedQueueDisc need at least one packet filter"<<endl;
  //     NS_LOG_ERROR ("ESRedQueueDisc need at least one packet filter");
  //     return false;
  //   }

  if (GetNInternalQueues () == 0)
    {
      // add a DropTail queue
      AddInternalQueue (CreateObjectWithAttributes<DropTailQueue<QueueDiscItem>> (
          "MaxSize", QueueSizeValue (GetMaxSize ())));
    }

  if (GetNInternalQueues () != 1)
    {
      NS_LOG_ERROR ("ESRedQueueDisc needs 1 internal queue");
      return false;
    }
  return true;
}

} // namespace ns3
