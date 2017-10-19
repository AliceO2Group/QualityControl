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
#include <QualityControl/QcInfoLogger.h>

using namespace std;
using namespace o2::quality_control::gui;
using namespace o2::quality_control::core;

int main(int argc, char *argv[])
{
  // Arguments parsing
  std::string configurationSource;
  if (argc > 1) {
    configurationSource = argv[1];
  } else {
    QcInfoLogger::GetInstance() << "no configuration file passed as argument, database won't work."
                                << QcInfoLogger::endm;
  }

  TApplication theApp("App", &argc, argv);

  auto *device = new SpyDevice();
  auto *frame = new SpyMainFrame(device, configurationSource);
  device->setFrame(frame);

  theApp.Run();

  return EXIT_SUCCESS;
}
