#ifndef HELICS_SPDC_APPLICATION_H
#define HELICS_SPDC_APPLICATION_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "ns3/random-variable-stream.h"

#include <map>
#include <string>

#include "helics-id-tag.h"
#include "helics-PDC-application.h"
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
class HelicsSPDCApplication : public HelicsPDCApplication
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId ();

  HelicsSPDCApplication ();
  virtual ~HelicsSPDCApplication ();
  virtual void StartApplication () override;
  void setPMUs (std::vector<std::string> pmu_names_);
  void setLogFile (std::string log);

protected:
  virtual void DoDispose () override;
  virtual void DoRead (std::unique_ptr<helics::Message> message) override;
  void ReadPayloads ();
  void TakeControlActions ();
  void PrintLogs ();

  std::vector<std::string> PMU_names;  //<! Names of the PMUs that send (directly or indirectly) data to the SPDC
  std::map<std::string, std::map<std::string, double>> delays;  //<! Delays of all received payloads, first key is timeStamp, second is PMU_name
  std::string log_file;
};

} // namespace ns3

#endif /* HELICS_SPDC_APPLICATION_H */
