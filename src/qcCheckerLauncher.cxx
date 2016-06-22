///
/// \file   qcCheckerLauncher.cxx
/// \author Barthelemy von Haller
///

// std
#include <iostream>
// boost
#include <boost/exception/diagnostic_information.hpp>
// ROOT
#include <TApplication.h>
#include <TROOT.h>
// FairRoot
#include <FairMQTransportFactoryZMQ.h>
// QC
#include "QualityControl/Checker.h"

using namespace std;
using namespace AliceO2::QualityControl::Core;
using namespace AliceO2::QualityControl::Checker;

int main(int argc, char *argv[])
{

  cout << "hello" << endl;
  Checker receiver;

  try {
    FairMQChannel histoChannel;
    histoChannel.UpdateType("sub");
    histoChannel.UpdateMethod("connect");
    histoChannel.UpdateAddress("tcp://localhost:5556");
    histoChannel.UpdateSndBufSize(10000);
    histoChannel.UpdateRcvBufSize(10000);
    histoChannel.UpdateRateLogging(0);
    receiver.fChannels["data-in"].push_back(histoChannel);

    // Get the proper transport factory
#ifdef NANOMSG
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
    FairMQTransportFactory *transportFactory = new FairMQTransportFactoryZMQ();
#endif

    receiver.SetTransport(transportFactory);

    receiver.ChangeState("INIT_DEVICE");
    receiver.WaitForEndOfState("INIT_DEVICE");

    receiver.ChangeState("INIT_TASK");
    receiver.WaitForEndOfState("INIT_TASK");

    receiver.ChangeState("RUN");
    receiver.InteractiveStateLoop();
  } catch (...) {
    string diagnostic = boost::current_exception_diagnostic_information();
    std::cerr << "Unexpected exception, diagnostic information follows:\n" << diagnostic << endl;
    if (diagnostic == "No diagnostic information available.") {
      throw;
    }
  }

  return EXIT_SUCCESS;
}
