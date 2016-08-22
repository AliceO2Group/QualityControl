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
  Checker checker;

  try {
    // todo create the number of channels required to listen to all the tasks we have been assigned.
    // todo we need to get this information from somewhere (config ? )
    vector <string> taskAddresses = {"tcp://localhost:5556", "tcp://localhost:5558"};
    for (auto address : taskAddresses) {
      checker.createChannel("sub", "connect", address, "data-in");
    }

    // Get the proper transport factory
#ifdef NANOMSG
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
    FairMQTransportFactory *transportFactory = new FairMQTransportFactoryZMQ();
#endif

    checker.SetTransport(transportFactory);

    checker.ChangeState("INIT_DEVICE");
    checker.WaitForEndOfState("INIT_DEVICE");

    checker.ChangeState("INIT_TASK");
    checker.WaitForEndOfState("INIT_TASK");

    checker.ChangeState("RUN");
    checker.InteractiveStateLoop();
  }
  catch (const boost::exception &e) {
    cerr << diagnostic_information(e) << endl;
  }
  catch (const std::exception &e) {
    cerr << "Error: " << e.what() << endl;
  }

  return EXIT_SUCCESS;
}
