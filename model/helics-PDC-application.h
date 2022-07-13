#ifndef HELICS_PDC_APPLICATION
#define HELICS_PDC_APPLICATION

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "ns3/random-variable-stream.h"

#include <map>
#include <string>

#include "helics-id-tag.h"
#include "helics-PMU-application.h"
#include "helics/helics.hpp"

namespace ns3 {

class Socket;
class Packet;
class InetSocketAddress;
class Inet6SocketAddress;

/**
 * \ingroup helicsapplication
 * \brief A Helics Application
 *
 * Every packet sent should be returned by the server and received here.
 */
class HelicsPDCApplication : public HelicsPMUApplication
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  HelicsPDCApplication ();

  virtual ~HelicsPDCApplication ();

  virtual void SetSubscriptions (std::shared_ptr<helics::CombinationFederate> fed, std::vector<std::string> keys) override;
  void SetTimer (const double);
  double GetTimer (void) const;
  void SetNbAggregates (const unsigned int);
  virtual void StartSampling (helics::Time time) override;
  void SetSeed (int seed);

protected:
  virtual void DoDispose (void) override;
  virtual void StartApplication (void) override;
  virtual void StopApplication (void) override;
  virtual void DoRead (std::unique_ptr<helics::Message> message) override;
  virtual void GenerateMeasurement (helics::Time time) override;
  
  double timer;
  unsigned int nb_aggregates;
  std::map<std::string, std::vector<std::string>> payloads;
  std::map<std::string, EventId> send_events;
  std::map<std::string, double> first_arrival;
};

} // namespace ns3

#endif /* HELICS_PDC_APPLICATION */
