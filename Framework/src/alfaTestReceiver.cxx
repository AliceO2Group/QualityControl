///
/// @file    alfaTestReceiver.cxx
/// @author  Barthelemy von Haller
///

#include "QualityControl/AlfaReceiverForTests.h"
#include "runFairMQDevice.h"


namespace bpo = boost::program_options;
using namespace std;
using namespace AliceO2::QualityControl::Core;

void addCustomOptions(bpo::options_description& options)
{
}

FairMQDevicePtr getDevice(const FairMQProgOptions& /*config*/)
{
  return new AlfaReceiverForTests();
}

/*#include "FairMQParser.h"
#include "FairMQProgOptions.h"
#include <FairMQTransportFactoryZMQ.h>



int main(int argc, char* argv[])
{
  AlfaReceiverForTests receiver;
  FairMQProgOptions config;

  try
  {
    // Get the config for the receiver
    config.UserParser<FairMQParser::JSON>("alfa.json", "receiver");

    // Set the channels based on the config
    receiver.fChannels = config.GetFairMQMap();

    // Get the proper transport factory
#ifdef NANOMSG
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif

    receiver.SetTransport(transportFactory);

    receiver.ChangeState("INIT_DEVICE");
    receiver.WaitForEndOfState("INIT_DEVICE");

    receiver.ChangeState("INIT_TASK");
    receiver.WaitForEndOfState("INIT_TASK");

    receiver.ChangeState("RUN");
    receiver.InteractiveStateLoop();
  }
  catch (std::exception& e)
  {
    cout << e.what();
    cout << "Command line options are the following: ";
    config.PrintHelp();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}*/
