/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
Copyright (c) 2017-2019, Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See the top-level NOTICE for additional details. All rights reserved.

SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef HELICS_HELPER_H
#define HELICS_HELPER_H

#include "ns3/application-container.h"
#include "ns3/core-module.h"
#include "ns3/node-container.h"

#include "ns3/helics.h"

namespace ns3 {

class HelicsHelper {
public:
  HelicsHelper ();

  void SetupFederate (void);
  void SetupFederate (int argc, char **argv);
  void SetupFederate (std::vector<std::string> &args);
  void SetupFederate (std::string &jsonString);
  void SetupApplicationFederate (void);
  void SetupApplicationFederateWithConfig (std::string &configFileName);
  void SetupCommandLine (CommandLine &cmd);

  ApplicationContainer InstallFilter (Ptr<Node> node, const std::string &name) const;
  ApplicationContainer InstallFilter (Ptr<Node> node, helics::Filter &fil, helics::Endpoint &ep) const;
  ApplicationContainer InstallEndpoint (Ptr<Node> node, helics::Endpoint &ep) const;

  ApplicationContainer InstallStaticSink (Ptr<Node> node, const std::string &name, const std::string &destination, bool is_global=false, int port=1234) const;
  ApplicationContainer InstallGlobalStaticSink (Ptr<Node> node, const std::string &name, const std::string &destination, int port=1234) const { return InstallStaticSink (node, name, destination, true, port); }

  ApplicationContainer InstallStaticSource (Ptr<Node> node, const std::string &name, const std::string &destination, bool is_global=false, int port=1234) const;
  ApplicationContainer InstallGlobalStaticSource (Ptr<Node> node, const std::string &name, const std::string &destination, int port=1234) const { return InstallStaticSource (node, name, destination, true, port); }

  ApplicationContainer InstallPMU (Ptr<Node> node, const std::string &name, const std::string &destination, double sampling_period_seconds, double stop_time_seconds, std::shared_ptr<helics::CombinationFederate> fed, std::vector<std::string> keys, int seed, bool pad, char pad_c = '*', uint16_t pad_l=500, int port=1234) const;
  ApplicationContainer InstallPDC (Ptr<Node> node, const std::string &name, const std::string &destination, double timer, const unsigned int nb_aggregates, bool pad, char pad_c, uint16_t pad_l, int port=123) const;
  ApplicationContainer InstallSPDC (Ptr<Node> node, const std::string &name, const std::string &destination, double timer, const unsigned int nb_aggregates, std::vector<std::string> PMU_names, std::string log_file, bool pad, char pad_c, uint16_t pad_l, int port=123) const;

private:
  std::string broker;
  std::string name;
  std::string type;
  std::string core;
  double timedelta;
  std::string coreinit;
  ObjectFactory m_factory_filter;
  ObjectFactory m_factory_sink;
  ObjectFactory m_factory_source;
  ObjectFactory m_factory_PMU;
  ObjectFactory m_factory_PDC;
  ObjectFactory m_factory_SPDC;
};

}

#endif /* HELICS_HELPER_H */

