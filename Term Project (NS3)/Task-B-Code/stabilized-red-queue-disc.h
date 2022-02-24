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

#ifndef STABILIZED_RED_QUEUE_DISC_H
#define STABILIZED_RED_QUEUE_DISC_H

#include <iostream>
#include <vector>

#include "ns3/queue-disc.h"
#include "ns3/nstime.h"
#include "ns3/boolean.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"

using namespace std;

struct Zombie
{
  int32_t flowID;         
  int32_t count;
  // Time time;

  // Zombie(int32_t flowID, int32_t count)
  // {
  //   this->flowID = flowID;
  //   this->count = count;
  // }

  // set time
  // void SetTime(Time time)
  // {
  //   this->time = time;
  // }
};

#define MAX_ZOMBIE_LIST_SIZE 1000

namespace ns3 {

class TraceContainer;

/**
 * \ingroup traffic-control
 *
 * \brief A Stabilized RED packet queue disc
 */
class StabilizedRedQueueDisc : public QueueDisc
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief StabilizedRedQueueDisc Constructor
   *
   * Create a Stabilized RED queue disc
   */
  StabilizedRedQueueDisc ();

  /**
   * \brief Destructor
   *
   * Destructor
   */ 
  virtual ~StabilizedRedQueueDisc ();

  /** 
   * \brief Drop types
   */
  enum
  {
    DTYPE_NONE,        //!< Ok, no drop
    DTYPE_FORCED,      //!< A "forced" drop
    DTYPE_UNFORCED,    //!< An "unforced" (random) drop
  };


 /**
  * Assign a fixed random variable stream number to the random variables
  * used by this model.  Return the number of streams (possibly zero) that
  * have been assigned.
  *
  * \param stream first stream index to use
  * \return the number of stream indices assigned by this model
  */
  int64_t AssignStreams (int64_t stream);

protected:
  /**
   * \brief Dispose of the object
   */
  virtual void DoDispose (void);

private:
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void);
  virtual bool CheckConfig (void);

  /**
   * \brief Initialize the queue parameters.
   *
   * Note: if the link bandwidth changes in the course of the
   * simulation, the bandwidth-dependent RED parameters do not change.
   * This should be fixed, but it would require some extra parameters,
   * and didn't seem worth the trouble...
   */
  virtual void InitializeParams (void);
  /**
   * \brief Compute the average queue size
   * \param nQueued number of queued packets
   * \param m simulated number of packets arrival during idle period
   * \param qAvg average queue size
   * \param qW queue weight given to cur q size sample
   * \returns new average queue size
   */
  double Estimator (uint32_t nQueued, uint32_t m, double qAvg, double qW);

  
  double calculateProbabilityStabilizedRed();
  double calculateProbabilityZapSimple(double probabilityStabilizedRed);
  double calculateProbabilityZap(double probabilityStabilizedRed,int32_t hit);
  
  vector<Zombie> zombies;
  double p_hitFreq;
  double p_overwrite; // 0.25 in paper
  double p_max; // 0.15 in paper
  double alpha; // p_overwrite/ZOMBIE_LIST_SIZE in paper

  int32_t stabilizedRedMode; // 1 for simple, 2 for full

  Ptr<UniformRandomVariable> m_uv;
};

}; // namespace ns3

#endif // RED_QUEUE_DISC_H
