/*
Copyright (c) 2017-2019, Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See the top-level NOTICE for additional details. All rights reserved.

SPDX-License-Identifier: BSD-3-Clause
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

#include "ns3/log.h"
#include "ns3/string.h"

#include "ns3/helics.h"
#include "ns3/helics-application.h"
#include "ns3/helics-PMU-application.h"
#include "helics/helics.hpp"
#include "ns3/simulator.h"
#include <stdlib.h>     /* srand, rand */

#include <algorithm>
#include <string>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HelicsPMUApplication");

NS_OBJECT_ENSURE_REGISTERED (HelicsPMUApplication);

TypeId
HelicsPMUApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HelicsPMUApplication")
    .SetParent<HelicsStaticSinkApplication> ()
    .AddConstructor<HelicsPMUApplication> ()
  ;
  return tid;
}

HelicsPMUApplication::HelicsPMUApplication ()
{
  NS_LOG_FUNCTION (this);
}

HelicsPMUApplication::~HelicsPMUApplication ()
{
  NS_LOG_FUNCTION (this);
}

void
HelicsPMUApplication::SetEndpointName (const std::string &name, bool is_global)
{
  NS_FATAL_ERROR (this << " should not define endpoint");
}

void
HelicsPMUApplication::SetPadding (const char pad, uint16_t len)
{
  NS_LOG_FUNCTION (this << pad);

  padding_char = pad;
  padded_size = len;
  padding_b = true;
}

void
HelicsPMUApplication::SetSubscriptions (std::shared_ptr<helics::CombinationFederate> fed, std::vector<std::string> keys)
{
  for (auto key : keys) {
    subscriptions.push_back (fed->getSubscription (key));
  }
}

void
HelicsPMUApplication::SetSamplingPeriodSeconds (double period)
{
  sampling_period = period;
}

void
HelicsPMUApplication::SetStopTimeSeconds (double stop_time_seconds)
{
  stop_time = stop_time_seconds;
}

void
HelicsPMUApplication::StartSampling (helics::Time time)
{
  if (!turn_on)
    GenerateMeasurement (time);
  else
    NS_LOG_WARN (this << "Already started");
}

void
HelicsPMUApplication::GenerateMeasurement (helics::Time time)
{
  NS_LOG_INFO (this << "  time: " << time);

  std::stringstream ss;
  ss << std::fixed;  // Avoid automatic scientific notation for small/large number
  ss << std::setprecision (5);
  ss << time;
  std::string timeStamp = ss.str ();
  timeStamp = timeStamp.substr (0, timeStamp.size () - 1);  // Remove the last char of timeStamp (which is second ('s'))

  std::string separator = ";";
  std::string data = timeStamp + separator + this->GetName();

  for (auto sub : subscriptions) {
    // std::cout << "updated: " << sub.isUpdated () << std::endl;
    if (!sub.isUpdated ()) {
      std::cout << "time: " << time << ", " << sub.getDisplayName () << ": " << sub.getDouble () << std::endl;
    }
    std::string value = std::to_string (sub.getDouble ());
    data += separator + value;
  }

  auto message = std::make_unique<helics::Message> ();
  message->data = data;
  if (padding_b)
    message = HelicsApplication::AddPadding (std::move (message), padding_char, padded_size);

  std::normal_distribution<> n{1e-6/2, 1e-6/6};  // PMUs have a synchronisation accuracy of 1µs (for the measurement, don't know about timing of packets)
  double random_delay;
  do
  {
    random_delay = n (m_eng);
  } while (random_delay <= 0);  // Truncated normal distribution (removes values below µ - 3 sigma, cannot schedule events in the past)
  random_delay = 0;

  // double ordering_delay = 0;
  Send(m_destination, time + random_delay, std::move (message));

  int64_t delay_ns = Time::FromDouble (time, Time::S).GetNanoSeconds() - Simulator::Now ().GetNanoSeconds ();  // Delay to schedule at current helics_time
  delay_ns += Time::FromDouble (sampling_period, Time::S).GetNanoSeconds ();  // Additional delay to schedule at next sampling period
  if (time + sampling_period < stop_time)
    Simulator::Schedule (NanoSeconds (delay_ns), &ns3::HelicsPMUApplication::GenerateMeasurement, this, time + sampling_period);
}

void
HelicsPMUApplication::SetSeed (int seed)
{
  std::default_random_engine eng (seed);
  m_eng = eng;
}

void
HelicsPMUApplication::SetDestination (const std::string &destination)
{
  NS_LOG_FUNCTION (this << destination);
  m_destination = destination;
}

std::string
HelicsPMUApplication::GetDestination (void) const
{
  return m_destination;
}

void
HelicsPMUApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  HelicsApplication::DoDispose ();
}

void 
HelicsPMUApplication::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  HelicsApplication::StartApplication();

  if (m_destination.empty()) {
    NS_FATAL_ERROR ("HelicsPMUApplication is missing destination");
  }
  StartSampling (m_startTime.GetSeconds ());
}

void 
HelicsPMUApplication::StopApplication (void)
{
  NS_LOG_FUNCTION (this);

  HelicsApplication::StopApplication();
}

void
HelicsPMUApplication::DoFilter (std::unique_ptr<helics::Message> message)
{
  NS_LOG_FUNCTION (this << message->to_string());

  NS_FATAL_ERROR ("HelicsPMUApplication should not filter messages");
}

void
HelicsPMUApplication::DoEndpoint (helics::Endpoint id, helics::Time time, std::unique_ptr<helics::Message> message)
{
  NS_LOG_FUNCTION (this << id.getName() << time << message->to_string());

  NS_FATAL_ERROR ("HelicsPMUApplication should not receive endpoint events");
}

void
HelicsPMUApplication::DoRead (std::unique_ptr<helics::Message> message)
{
  NS_LOG_FUNCTION (this << message->to_string());

  NS_FATAL_ERROR ("HelicsPMUApplication should not read from socket");
}

} // Namespace ns3
