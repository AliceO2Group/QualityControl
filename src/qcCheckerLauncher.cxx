///
/// \file   qcCheckerLauncher.cxx
/// \author Barthelemy von Haller
///

// std
#include <iostream>
#include <signal.h>
// boost
#include <boost/program_options.hpp>
#include <boost/exception/diagnostic_information.hpp>
// ROOT
#include <TApplication.h>
#include <TROOT.h>
// FairRoot
#include <FairMQTransportFactoryZMQ.h>
// O2
#include "Common/signalUtilities.h"
// QC
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Checker.h"
#include "QualityControl/Version.h"

namespace po = boost::program_options;

using namespace std;
using namespace AliceO2::QualityControl::Core;
using namespace AliceO2::QualityControl::Checker;

int main(int argc, char *argv[])
{

  cout << "hello" << endl;
  Checker receiver;
//  FairMQProgOptions config;

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
  } catch (std::exception &e) {
    cout << e.what();
//    cout << "Command line options are the following: ";
//    config.PrintHelp();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
