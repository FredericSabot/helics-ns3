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
#include "ns3/helics-SPDC-application.h"
#include "ns3/simulator.h"
#include "helics/helics.hpp"

#include <algorithm>
#include <sstream>
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include "ns3/names.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HelicsSPDCApplication");

NS_OBJECT_ENSURE_REGISTERED (HelicsSPDCApplication);

TypeId
HelicsSPDCApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HelicsSPDCApplication")
    .SetParent<HelicsPDCApplication> ()
    .AddConstructor<HelicsSPDCApplication> ()

  ;
  return tid;
}

HelicsSPDCApplication::HelicsSPDCApplication (void)
{
  NS_LOG_FUNCTION (this);
}

HelicsSPDCApplication::~HelicsSPDCApplication (void)
{
  NS_LOG_FUNCTION (this);
}

void 
HelicsSPDCApplication::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  HelicsPDCApplication::StartApplication();
}

void
HelicsSPDCApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  PrintLogs ();
  HelicsPDCApplication::DoDispose ();
}

void
HelicsSPDCApplication::setPMUs (std::vector<std::string> pmu_names)
{
  PMU_names = pmu_names;
}

void
HelicsSPDCApplication::setLogFile (std::string log)
{
  log_file = log;
}

void
HelicsSPDCApplication::DoRead (std::unique_ptr<helics::Message> message)
{
  NS_LOG_FUNCTION (this << message->to_string ());

  if (padding_b)
    message = HelicsApplication::RemovePadding (std::move (message), padding_char);

  std::string data = std::string (message->to_string ());
  std::string timeStamp;

  std::stringstream s(data);
  std::getline(s, timeStamp);

  std::string payload;
  std::getline(s, payload);
  std::string line;
  std::vector<std::string> PMUs;
  while (std::getline(s, line)) {
    payload += "\n" + line;
    
    std::stringstream ss(line);
    std::string PMU_name;
    if (!std::getline(ss, PMU_name, ';'))
    {
      NS_FATAL_ERROR ("Malformed data");
    }
    PMUs.push_back (PMU_name);
  }

  if (payloads.find(timeStamp) == payloads.end())  // First received packet with this timeStamp
  {    
    std::vector<std::string> payload_v = {payload};
    payloads[timeStamp] = payload_v;

    for (auto PMU_name : PMUs)
    {
      delays[timeStamp][PMU_name] = Simulator::Now ().GetSeconds () - std::stof (timeStamp);
    }

    first_arrival[timeStamp] = Simulator::Now ().GetSeconds ();
    send_events[timeStamp] = Simulator::Schedule(Seconds (first_arrival[timeStamp] + timer), &HelicsSPDCApplication::ReadPayloads, this);
  }
  else if (send_events[timeStamp].IsExpired ())
  {
    // Message arriving after maximum wait time, and is thus disregarded
  }
  else
  {
    send_events[timeStamp].Cancel ();

    payloads[timeStamp].push_back (payload);

    for (auto PMU_name : PMUs)
    {
      delays[timeStamp][PMU_name] = Simulator::Now ().GetSeconds () - std::stof (timeStamp);
    }

    if (payloads[timeStamp].size () < nb_aggregates)
    {
      send_events[timeStamp] = Simulator::Schedule(Seconds (first_arrival[timeStamp] + timer), &HelicsSPDCApplication::ReadPayloads, this);
    }
    else
    {
      send_events[timeStamp] = Simulator::ScheduleNow(&HelicsSPDCApplication::ReadPayloads, this);
    }
  }
}

void
HelicsSPDCApplication::ReadPayloads ()
{
  NS_LOG_FUNCTION (this);

  // TODO: transform payloads into maps/vectors that are easier to work with, and feed them to TakeControlActions (args)

  TakeControlActions ();
}

void
HelicsSPDCApplication::TakeControlActions ()
{
  NS_LOG_FUNCTION (this);

  // The base SPDC does not take control actions
}

void
HelicsSPDCApplication::PrintLogs ()
{
  NS_LOG_FUNCTION (this);

  std::string separator = ";";
  std::ofstream stream(log_file.c_str(), std::ofstream::out);

  // Header
  stream << "Time stamp";
  for (auto PMU_name : PMU_names)
  {
    stream << separator << PMU_name;
  }
  stream << std::endl;

  for (auto [timeStamp, PMU_delays] : delays)
  {
    stream << timeStamp;

    for (auto PMU_name : PMU_names)
    {
      if (PMU_delays.find (PMU_name) == PMU_delays.end ())
      {
        stream << separator << "0";  // Lost
      }
      else
      {
        stream << separator << PMU_delays[PMU_name];
      }
    }
    stream << std::endl;
  }
}
} // Namespace ns3
