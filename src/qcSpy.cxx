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
  TApplication theApp("App", &argc, argv);

  SpyDevice *device = new SpyDevice();
  SpyMainFrame *frame = new SpyMainFrame(device);
  device->setFrame(frame);

  theApp.Run();

  return EXIT_SUCCESS;
}
