/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/aodv-module.h"

#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/nix-vector-helper.h"

#include "ns3/helics-helper.h"
#include "ns3/helics-SPDC-application.h"

#include "ns3/ipv4.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/visualizer-module.h"

#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>

#include <cstdlib>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("HelicsExample");

int 
main (int argc, char *argv[])
{
	CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);

  bool verbose = false;

	std::string dataRate_p2p = "800Mbps";
  double sampling_period_PMU = 0.02;
  bool padding_b = true;
  char padding_c = '*';
  uint16_t padding_l = 500;  // Assumed that the aggregated packets also have a size of 500 bytes (like the PMU ones)
  uint16_t padding_SPDC = 500;

  double start_time = 1;
  double stop_time = 5;
  double cleanup_time = 5;

  HelicsHelper helicsHelper;

  if (verbose)
  {
    /*LogComponentEnable ("HelicsExample", LOG_LEVEL_INFO);
    LogComponentEnable ("HelicsSimulatorImpl", LOG_LEVEL_INFO);
    LogComponentEnable ("HelicsStaticSinkApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("HelicsStaticSourceApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("HelicsApplication", LOG_LEVEL_INFO);*/

    // LogComponentEnable ("HelicsPDCApplication", LOG_LEVEL_INFO);
    // LogComponentEnable ("HelicsSPDCApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("HelicsPMUApplication", LOG_LEVEL_INFO);

    /*LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);*/
  }
  // LogComponentEnable ("HelicsSPDCApplication", LOG_LEVEL_INFO);

  NS_LOG_INFO ("Calling helicsHelper.SetupApplicationFederate");
  std::string path = std::filesystem::current_path();
  std::string json = path + "/contrib/helics/examples/IEEE39.json";
  helicsHelper.SetupApplicationFederateWithConfig(json);

  // read topology file
  std::string inputFile = path + "/contrib/helics/examples/ieee39.csv";
  std::string log_file = path + "/contrib/helics/examples/ieee39-log-delays.csv";
  std::ifstream topologyStream(inputFile.c_str(), std::ifstream::in);
  std::string line, word;
  std::vector<std::vector<std::string>> topology;

  std::getline(topologyStream, line);
  if (line != "End 1; End 2; Length" && line != "End 1; End 2; Length;") {
    std::cout << line << std::endl;
    throw std::invalid_argument("Invalid csv header or missing file");
  }

  while (std::getline(topologyStream, line)) {
    std::vector<std::string> row;
    std::stringstream s(line);
    while (std::getline(s, word, ';')) {
      row.push_back(word);
    }
    topology.push_back(row);
    line.clear();
  }
  topologyStream.close();

  NodeContainer nodes;
  nodes.Create (39);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue (dataRate_p2p));
  NetDeviceContainer p2pDevices;

  Config::SetDefault ("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue (true));  // Only useful when global routing is used
  Ipv4GlobalRoutingHelper globalRouting;
  InternetStackHelper stack;
  stack.SetRoutingHelper (globalRouting);
  stack.Install (nodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.255.252");
  NetDeviceContainer failed_link;

  for (auto &it : topology) {
    int node_1 = std::stoi(it[0]) - 1;
    int node_2 = std::stoi(it[1]) - 1;
    double length = std::stof(it[2]);
    double delay = length * 5000;  // 5000ns per km (i.e. assuming 1.5 refractive index)
    std::string delay_s = std::to_string(delay) + "ns";

    // delay_s = "5ms";  // Replace delay by value from GECO test case

    p2p.SetChannelAttribute ("Delay", StringValue (delay_s));
    NodeContainer n_links = NodeContainer (nodes.Get (node_1), nodes.Get (node_2));
    NetDeviceContainer n_devices = p2p.Install (n_links);
    ipv4.Assign (n_devices);
    if (node_1 == (16-1) && node_2 == (17-1))
    {
      failed_link = n_devices;
    }
    ipv4.NewNetwork ();
  }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();  // Is bypassed if another routing algo is defined

  double failure_time = 1;
  Ptr<Node> n = nodes.Get (16-1);
  Ptr<Ipv4> ipv4_ = n->GetObject<Ipv4> ();
  Simulator::Schedule (Seconds (failure_time),&Ipv4::SetDown, ipv4_, ipv4_->GetInterfaceForDevice (failed_link.Get (0)));

  n = nodes.Get (17-1);
  ipv4_ = n->GetObject<Ipv4> ();
  Simulator::Schedule (Seconds (failure_time),&Ipv4::SetDown, ipv4_, ipv4_->GetInterfaceForDevice (failed_link.Get (1)));






  ///////////////// Applications /////////////////

  // Install helics sinks
  ApplicationContainer PMUs;
  path = std::filesystem::current_path();
  inputFile = path + std::string("/contrib/helics/examples/aggr-topology.csv");
  std::ifstream PMUsStream(inputFile.c_str(), std::ifstream::in);

  std::getline(PMUsStream, line);
  if (line != "PMU;Target" && line != "PMU;Target;") {
    std::cout << line << std::endl;
    throw std::invalid_argument("Invalid csv header or missing file");
  }

  std::srand (1);
  
  std::vector<std::string> PMU_names;
  while (std::getline(PMUsStream, line)) {
    std::string name;
    std::stringstream s(line);
    if (std::getline(s, word, ';')) {
      name = word;
    } else { throw std::invalid_argument("Invalid PMU file");}

    std::string destination;
    std::getline(s, word, ';');
    if (word.size () != 0) {
      destination = word;
    } else { throw std::invalid_argument("Invalid PMU file");}

    line.clear();

    std::vector<std::string> keys = {"Dynawo/U_" + name, "Dynawo/Phi_" + name};
    PMUs.Add (helicsHelper.InstallPMU (nodes.Get (std::stoi (name) - 1), name, destination, sampling_period_PMU, stop_time, helics_federate, keys, std::rand (), true, '*', 500));
    PMU_names.push_back (name);
  }
  PMUsStream.close ();

  double PDC_timer = 0.05;
  ApplicationContainer PDCs;
  PDCs.Add (helicsHelper.InstallPDC (nodes.Get (1), "PDC1", "SPDC", PDC_timer, 8, padding_b, padding_c, padding_l)); // PDC at bus 2 (care zero indexing), time = 50ms, expecting data from 8 PMUs
  PDCs.Add (helicsHelper.InstallPDC (nodes.Get (26), "PDC2", "SPDC", PDC_timer, 10, padding_b, padding_c, padding_l));
  PDCs.Add (helicsHelper.InstallPDC (nodes.Get (5), "PDC3", "SPDC", PDC_timer, 12, padding_b, padding_c, padding_l));
  PDCs.Add (helicsHelper.InstallPDC (nodes.Get (20), "PDC4", "SPDC", PDC_timer, 9, padding_b, padding_c, padding_l));

  double SPDC_timer = 0.05;
  ApplicationContainer SPDC = helicsHelper.InstallSPDC (nodes.Get (15), "SPDC", "SPDC", SPDC_timer, 4, PMU_names, log_file, padding_b, padding_c, padding_SPDC);  // Destination not used for now

  PDCs.Start (Seconds (0.0));
  PDCs.Stop (Seconds (stop_time + PDC_timer));
  PMUs.Start (Seconds (start_time));
  PMUs.Stop (Seconds (stop_time));
  SPDC.Start (Seconds (0.0));
  SPDC.Stop (Seconds (stop_time + PDC_timer + SPDC_timer));

  if ((PDC_timer + SPDC_timer) >= cleanup_time)
  {
    NS_FATAL_ERROR ("Cleanup time too short");
  }

  // p2p.EnablePcapAll ("ieee39");
  // csma.EnablePcap ("second", csmaDevices.Get (1), true);
  
  /*// Flow monitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  // The module provides the following attributes in ns3::FlowMonitor:

  //   MaxPerHopDelay (Time, default 10s): The maximum per-hop delay that should be considered;
  //   StartTime (Time, default 0s): The time when the monitoring starts;
  //   DelayBinWidth (double, default 0.001): The width used in the delay histogram;
  //   JitterBinWidth (double, default 0.001): The width used in the jitter histogram;
  //   PacketSizeBinWidth (double, default 20.0): The width used in the packetSize histogram;
  //   FlowInterruptionsBinWidth (double, default 0.25): The width used in the flowInterruptions histogram;
  //   FlowInterruptionsMinTime (double, default 0.5): The minimum inter-arrival time that is considered a flow interruption.
  
  flowMonitor = flowHelper.InstallAll();*/

  Simulator::Stop (Seconds (stop_time + cleanup_time));

  NS_LOG_INFO("Running Simulation...");
  Simulator::Run ();

  // flowMonitor->SerializeToXmlFile("NameOfFile.xml", true, false);

  Simulator::Destroy ();
  NS_LOG_INFO("Simulation complete.");
  return 0;
}

