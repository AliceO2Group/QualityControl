///
/// \file   qcSpy.cxx
/// \author Barthelemy von Haller
///
/// A little tool to connect to a FairMQ device (or any ZeroMQ device actually) and get the TObjects it
/// is publishing. It can be any object inheriting from TObject.
///
/// TODO for the qcSpy
/// - fix whatever thread and memory issues there are (?)

// std
#include <iostream>
// ROOT
#include <TApplication.h>
// QC
#include "QualityControl/SpyMainFrame.h"
#include "QualityControl/SpyDevice.h"

using namespace std;
using namespace AliceO2::QualityControl::Gui;

int main(int argc, char *argv[])
{
  // Arguments parsing
  std::string configurationSource;
  if(argc > 1) {
    configurationSource = argv[1];
    cout << "configuration file : " << configurationSource << endl;
  } else {
    cout << "no configuration file passed as argument, database won't work." << endl;
  }

  TApplication theApp("App", &argc, argv);

  auto *device = new SpyDevice();
  auto *frame = new SpyMainFrame(device, configurationSource);
  device->setFrame(frame);

  theApp.Run();

  return EXIT_SUCCESS;
}
