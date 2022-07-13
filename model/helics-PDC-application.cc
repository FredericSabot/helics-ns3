/*
Copyright (c) 2017-2019, Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See the top-level NOTICE for additional details. All rights reserved.

SPDX-License-Identifier: BSD-3-Clause
*/

#include <iostream>
#include <fstream>
#include <vector>

#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"

#include "ns3/helics.h"
#include "ns3/helics-application.h"
#include "ns3/helics-PDC-application.h"
#include "helics/helics.hpp"
#include "ns3/simulator.h"

#include <algorithm>
#include <sstream>
#include <string>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HelicsPDCApplication");

NS_OBJECT_ENSURE_REGISTERED (HelicsPDCApplication);

TypeId
HelicsPDCApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HelicsPDCApplication")
    .SetParent<HelicsPMUApplication> ()
    .AddConstructor<HelicsPDCApplication> ()
    .AddAttribute ("Timer",
                   "The maximum waiting time between the first packet received with a given time stamp, and the sending of the associated aggregated frame",
                   DoubleValue (),
                   MakeDoubleAccessor (&HelicsPDCApplication::timer),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NbPMUs",
                   "The number of PMUs that send data to the aggregator",
                   UintegerValue (),
                   MakeUintegerAccessor (&HelicsPDCApplication::nb_aggregates),
                   MakeUintegerChecker<unsigned int> ())

  ;
  return tid;
}

HelicsPDCApplication::HelicsPDCApplication (void)
{
  NS_LOG_FUNCTION (this);
}

HelicsPDCApplication::~HelicsPDCApplication (void)
{
  NS_LOG_FUNCTION (this);
}

void
HelicsPDCApplication::SetTimer (const double timer_)
{
  NS_LOG_FUNCTION (this << timer_);

  timer = timer_;
}

double
HelicsPDCApplication::GetTimer (void) const
{
  NS_LOG_FUNCTION (this);

  return timer;
}

void
HelicsPDCApplication::SetNbAggregates (const unsigned int nbAggregates)
{
  NS_LOG_FUNCTION (this << nbAggregates);

  nb_aggregates = nbAggregates;
}

void
HelicsPDCApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  HelicsPMUApplication::DoDispose ();
}

void 
HelicsPDCApplication::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  HelicsPMUApplication::StartApplication();
}

void 
HelicsPDCApplication::StopApplication (void)
{
  NS_LOG_FUNCTION (this);

  HelicsPMUApplication::StopApplication();
}

void
HelicsPDCApplication::SetSubscriptions (std::shared_ptr<helics::CombinationFederate> fed, std::vector<std::string> keys)
{
  NS_FATAL_ERROR (this << ": PDCs should only aggregate measures, not create them");
}

void
HelicsPDCApplication::StartSampling (helics::Time time)
{
  // NS_FATAL_ERROR (this << ": PDCs should only aggregate measures, not create them");
}

void
HelicsPDCApplication::GenerateMeasurement (helics::Time time)
{
  NS_FATAL_ERROR (this << ": PDCs should only aggregate measures, not create them");
}

void
HelicsPDCApplication::SetSeed (int seed)
{
  NS_FATAL_ERROR (this << ": PDCs do not use a seed");
}

void
HelicsPDCApplication::DoRead (std::unique_ptr<helics::Message> message)
{
  NS_LOG_FUNCTION (this << message->to_string());

  if (padding_b)
    message = HelicsPMUApplication::RemovePadding (std::move (message), padding_char);
  std::string data = std::string(message->to_string());
  std::string timeStamp;

  std::stringstream s(data);
  if (!std::getline(s, timeStamp, ';'))
  {
    NS_FATAL_ERROR ("Malformed data");
  }
  std::string payload;
  std::getline(s, payload);

  auto aggr_message = std::make_unique<helics::Message> ();
  std::string aggr_data = timeStamp;

  if (payloads.find(timeStamp) == payloads.end())  // First received packet with this timeStamp
  {    
    std::vector<std::string> payload_v = {payload};
    payloads[timeStamp] = payload_v;

    for (auto &it : payloads[timeStamp])  // Aggregated data = timeStamp followed by all the received payloads separated by \n
    {
      aggr_data += "\n" + it;
    }
    aggr_message->data = aggr_data;
    if (padding_b)
      aggr_message = HelicsPMUApplication::AddPadding (std::move (aggr_message), padding_char, padded_size);

    first_arrival[timeStamp] = Simulator::Now ().GetSeconds ();
    // Schedules a send event for when the timer for timeStamp runs out. This event will be replaced if a new PMU packet arrives before the timer
    send_events[timeStamp] = Send (m_destination, first_arrival[timeStamp] + timer, std::move (aggr_message));
  }
  else if (send_events[timeStamp].IsExpired ())
  {
    // Message arriving after frame has been send, and is thus disregarded
  }
  else
  {
    send_events[timeStamp].Cancel ();
    payloads[timeStamp].push_back(payload);

    for (auto &it : payloads[timeStamp])  // Aggregated data = timeStamp followed by all the received payloads separated by \n
    {
      aggr_data += "\n" + it;
    }
    aggr_message->data = aggr_data;
    if (padding_b)
      aggr_message = HelicsPMUApplication::AddPadding (std::move (aggr_message), padding_char, padded_size);

    if (payloads[timeStamp].size () < nb_aggregates)
    {
      send_events[timeStamp] = Send (m_destination, first_arrival[timeStamp] + timer, std::move (aggr_message));  // Schedules a send event for when the timer for timeStamp runs out
    }
    else
    {
      send_events[timeStamp] = Send (m_destination, Simulator::Now  ().GetSeconds (), std::move (aggr_message));  // Send the message now since the frame is complete
    }
  }
}

} // Namespace ns3
