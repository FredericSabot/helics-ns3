/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
Copyright (c) 2017-2019, Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC.  See the top-level NOTICE for additional details. All rights reserved.

SPDX-License-Identifier: BSD-3-Clause
*/

#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/ipv4.h"
#include <memory>

#include "ns3/helics.h"
#include "ns3/helics-application.h"
#include "ns3/helics-filter-application.h"
#include "ns3/helics-static-sink-application.h"
#include "ns3/helics-static-source-application.h"
#include "ns3/helics-PMU-application.h"
#include "ns3/helics-PDC-application.h"
#include "ns3/helics-SPDC-application.h"
#include "ns3/helics-helper.h"

#include "helics/core/CoreTypes.hpp"
#include "helics/application_api/timeOperations.hpp"
#include "helics/application_api/typeOperations.hpp"

namespace ns3 {

HelicsHelper::HelicsHelper()
: broker("")
, name("ns3")
, core("zmq")
, timedelta(1.0)
, coreinit("")
{
    m_factory_filter.SetTypeId (HelicsFilterApplication::GetTypeId ());
    m_factory_sink.SetTypeId (HelicsStaticSinkApplication::GetTypeId ());
    m_factory_source.SetTypeId (HelicsStaticSourceApplication::GetTypeId ());
    m_factory_PMU.SetTypeId (HelicsPMUApplication::GetTypeId ());
    m_factory_PDC.SetTypeId (HelicsPDCApplication::GetTypeId ());
    m_factory_SPDC.SetTypeId (HelicsSPDCApplication::GetTypeId ());
}

void
HelicsHelper::SetupFederate(void) {
    helics::FederateInfo fi{};
    fi.broker = broker;
    fi.coreType = helics::coreTypeFromString(core);
    fi.setProperty(HELICS_PROPERTY_TIME_DELTA, helics::loadTimeFromString("1ns"));
    if (!coreinit.empty()) {
        fi.coreInitString = coreinit;
    }
    helics_federate = std::make_shared<helics::CombinationFederate>(name, fi);
}

// Some of the recognized args:
// broker - address of the broker to connect
// name - name of the federate
// corename - name of the core to create or find
// coretype - type of core to connect to
// offset - offset of time steps
// period - period of the federate
// timedelta - the time delta of the federate
// coreinit - the core initialization string
// inputdelay - delay on incoming communication to the federate
// outputdelay - delay on outgoing communication from the federate
// flags - named flag for the federate

// Some default values:
// timeDelta = timeEpsilon (minimum time advance allowed by federate)
// outputDelay, inputDelay, period, offset = timeZero
void
HelicsHelper::SetupFederate(int argc, char **argv)
{
  helics::FederateInfo fi (argc, argv);
  helics_federate = std::make_shared<helics::CombinationFederate> (name, fi);
}

void
HelicsHelper::SetupFederate(std::string &jsonString)
{
  helics::FederateInfo fi = helics::loadFederateInfo (jsonString);
  helics_federate = std::make_shared<helics::CombinationFederate> (name, fi);
}

void
HelicsHelper::SetupFederate(std::vector<std::string> &args)
{
  helics::FederateInfo fi (args);
  helics_federate = std::make_shared<helics::CombinationFederate> (name, fi);
}

void
HelicsHelper::SetupApplicationFederate(void)
{
  SetupFederate ();
  helics_endpoint = helics_federate->registerEndpoint ("fout");

  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::HelicsSimulatorImpl"));
}

void
HelicsHelper::SetupApplicationFederateWithConfig(std::string &configFileName)
{
	helics_federate = std::make_shared<helics::CombinationFederate> (configFileName);
	GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::HelicsSimulatorImpl"));
}

void
HelicsHelper::SetupCommandLine(CommandLine &cmd)
{
  cmd.AddValue ("broker", "address to connect the broker to", broker);
  cmd.AddValue ("name", "name of the ns3 federate", name);
  cmd.AddValue ("coretype", "type of core to connect to", core);
  cmd.AddValue ("timedelta", "the time delta of the federate", timedelta);
  cmd.AddValue ("coreinit", "the core initializion string", coreinit);
}

ApplicationContainer
HelicsHelper::InstallFilter (Ptr<Node> node, const std::string &name) const
{
    ApplicationContainer apps;
    Ptr<HelicsFilterApplication> app = m_factory_filter.Create<HelicsFilterApplication> ();
    if (!app) {
      NS_FATAL_ERROR ("Failed to create HelicsFilterApplication");
    }
    app->SetFilterName (name);
    Ptr<Ipv4> net = node->GetObject<Ipv4>();
    Ipv4InterfaceAddress interface_address = net->GetAddress(1,0);
    Ipv4Address address = interface_address.GetLocal();
    app->SetLocal(address, 1234);
    node->AddApplication (app);
    apps.Add (app);
    return apps;
}

ApplicationContainer HelicsHelper::InstallFilter (Ptr<Node> node, helics::Filter &fil, helics::Endpoint &ep) const
{
    ApplicationContainer apps;
    Ptr<HelicsFilterApplication> app = m_factory_filter.Create<HelicsFilterApplication> ();
    if (!app) {
      NS_FATAL_ERROR ("Failed to create HelicsFilterApplication");
    }
    app->SetupFilterApplication (fil, ep);
    Ptr<Ipv4> net = node->GetObject<Ipv4>();
    Ipv4InterfaceAddress interface_address = net->GetAddress(1,0);
    Ipv4Address address = interface_address.GetLocal();
    app->SetLocal(address, 1234);
    node->AddApplication (app);
    apps.Add (app);
    return apps;
}

ApplicationContainer
HelicsHelper::InstallEndpoint (Ptr<Node> node, helics::Endpoint &ep) const
{
  ApplicationContainer apps;
  Ptr<HelicsFilterApplication> app = m_factory_filter.Create<HelicsFilterApplication> ();
  if (!app) {
      NS_FATAL_ERROR ("Failed to create HelicsFilterApplication");
    }

  app->SetEndpoint (ep);
  Ptr<Ipv4> net = node->GetObject<Ipv4>();
  Ipv4InterfaceAddress interface_address = net->GetAddress(1,0);
  Ipv4Address address = interface_address.GetLocal();
  app->SetLocal(address, 1234);
  node->AddApplication (app);
  apps.Add (app);
  return apps;
}

ApplicationContainer
HelicsHelper::InstallStaticSink (Ptr<Node> node, const std::string &name, const std::string &destination, bool is_global, int port) const
{
    ApplicationContainer apps;
    Ptr<HelicsStaticSinkApplication> app = m_factory_sink.Create<HelicsStaticSinkApplication> ();
    if (!app) {
      NS_FATAL_ERROR ("Failed to create HelicsStaticSinkApplication");
    }
    app->SetEndpointName (name, is_global);
    app->SetDestination (destination);
    Ptr<Ipv4> net = node->GetObject<Ipv4>();
    Ipv4InterfaceAddress interface_address = net->GetAddress(1,0);
    Ipv4Address address = interface_address.GetLocal();
    app->SetLocal(address, port);
    node->AddApplication (app);
    apps.Add (app);
    return apps;
}

ApplicationContainer
HelicsHelper::InstallStaticSource (Ptr<Node> node, const std::string &name, const std::string &destination, bool is_global, int port) const
{
    ApplicationContainer apps;
    Ptr<HelicsStaticSourceApplication> app = m_factory_source.Create<HelicsStaticSourceApplication> ();
    if (!app) {
      NS_FATAL_ERROR ("Failed to create HelicsStaticSinkApplication");
    }
    app->SetEndpointName (name, is_global);
    app->SetDestination (destination);
    Ptr<Ipv4> net = node->GetObject<Ipv4>();
    Ipv4InterfaceAddress interface_address = net->GetAddress(1,0);
    Ipv4Address address = interface_address.GetLocal();
    app->SetLocal(address, port);
    node->AddApplication (app);
    apps.Add (app);
    return apps;
}

ApplicationContainer
HelicsHelper::InstallPMU (Ptr<Node> node, const std::string &name, const std::string &destination, double sampling_period_seconds, double stop_time_seconds, std::shared_ptr<helics::CombinationFederate> fed, std::vector<std::string> keys, int seed, bool pad, char pad_c, uint16_t pad_l, int port) const
{
    ApplicationContainer apps;
    Ptr<HelicsPMUApplication> app = m_factory_PMU.Create<HelicsPMUApplication> ();
    if (!app) {
      NS_FATAL_ERROR ("Failed to create HelicsPMUApplication");
    }
    app->SetName (name);
    app->SetDestination (destination);
    app->SetSeed (seed);
    Ptr<Ipv4> net = node->GetObject<Ipv4>();
    Ipv4InterfaceAddress interface_address = net->GetAddress(1,0);
    Ipv4Address address = interface_address.GetLocal();
    app->SetLocal(address, port);
    node->AddApplication (app);
    app->SetSubscriptions (fed, keys);
    app->SetSamplingPeriodSeconds (sampling_period_seconds);
    app->SetStopTimeSeconds (stop_time_seconds);
    if (pad)
      app->SetPadding (pad_c, pad_l);
    
    apps.Add (app);
    return apps;
}

ApplicationContainer
HelicsHelper::InstallPDC (Ptr<Node> node, const std::string &name, const std::string &destination, double timer, const unsigned int nb_aggregates, bool pad, char pad_c, uint16_t pad_l, int port) const
{
    ApplicationContainer apps;
    Ptr<HelicsPDCApplication> app = m_factory_PDC.Create<HelicsPDCApplication> ();
    if (!app) {
      NS_FATAL_ERROR ("Failed to create HelicsPDCApplication");
    }
    app->SetName (name);
    app->SetDestination (destination);
    Ptr<Ipv4> net = node->GetObject<Ipv4>();
    Ipv4InterfaceAddress interface_address = net->GetAddress(1,0);
    Ipv4Address address = interface_address.GetLocal();
    app->SetLocal(address, port);
    node->AddApplication (app);

    app->SetTimer (timer);
    app->SetNbAggregates (nb_aggregates);
    if (pad)
      app->SetPadding (pad_c, pad_l);
    apps.Add (app);
    return apps;
}

ApplicationContainer
HelicsHelper::InstallSPDC (Ptr<Node> node, const std::string &name, const std::string &destination, double timer, const unsigned int nb_aggregates, std::vector<std::string> PMU_names, std::string log_file, bool pad, char pad_c, uint16_t pad_l, int port) const
{
    ApplicationContainer apps;
    Ptr<HelicsSPDCApplication> app = m_factory_SPDC.Create<HelicsSPDCApplication> ();
    if (!app) {
      NS_FATAL_ERROR ("Failed to create HelicsSPDCApplication");
    }
    app->SetName (name);
    app->SetDestination (destination);
    Ptr<Ipv4> net = node->GetObject<Ipv4>();
    Ipv4InterfaceAddress interface_address = net->GetAddress(1,0);
    Ipv4Address address = interface_address.GetLocal();
    app->SetLocal(address, port);
    node->AddApplication (app);

    app->SetTimer (timer);
    app->SetNbAggregates (nb_aggregates);
    if (pad)
      app->SetPadding (pad_c, pad_l);
    
    app->setPMUs (PMU_names);
    app->setLogFile (log_file);
    apps.Add (app);
    return apps;
}
}

