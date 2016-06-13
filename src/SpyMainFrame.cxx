///
/// \file   SpyMainFrame.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/SpyMainFrame.h"
#include <TROOT.h>
#include <TApplication.h>
#include <TGSplitter.h>
#include <TH1F.h>
#include <TCanvas.h>
#include <TGButton.h>
#include <iostream>
#include "QualityControl/SpyDevice.h"
#include <TSystem.h>
#include <TClass.h>

using namespace std;

namespace AliceO2 {
namespace QualityControl {
namespace Gui {

SpyMainFrame::SpyMainFrame(SpyDevice *spyDevice)
    : TGMainFrame(gClient->GetRoot(), 1024, 576, kFixedSize), mController(spyDevice), mDrawnObject(nullptr)
{
  // use hierarchical cleaning
  SetCleanup(kDeepCleanup);
  Connect("CloseWindow()", "AliceO2::QualityControl::Gui::SpyMainFrame", this, "closeWindow()");
  SetWindowName("Quality Control Spy");

  constructWindow();
}

SpyMainFrame::~SpyMainFrame()
{
  if(mDrawnObject) {
    delete mDrawnObject;
  }
  std::map<std::string, TGButton*>::iterator iter;
  for(iter = mMapButtons.begin() ; iter != mMapButtons.end() ; iter++) {
    delete iter->second;
  }
  Disconnect(this);
}

void SpyMainFrame::constructWindow()
{
  // prepare the layout of frames from top to bottom
  fMenuBar = new TGMenuBar(this);
  AddFrame(fMenuBar, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 0, 0, 0, 0));
  fObjectsBrowserFrame = new TGHorizontalFrame(this, 1, 1, kChildFrame | kSunkenFrame);
  AddFrame(fObjectsBrowserFrame, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY , 0, 0, 0, 0));
  fBottomButtonFrame = new TGHorizontalFrame(this, 1, 40, kChildFrame | kSunkenFrame | kFixedHeight);
  AddFrame(fBottomButtonFrame, new TGLayoutHints(kLHintsExpandX , 0, 0, 0, 0));
  fStatusBar = new TGStatusBar(this, 50, 10);
  AddFrame(fStatusBar, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 0, 0, 0, 0));

  // menu
  TGPopupMenu* fFile = new TGPopupMenu(gClient->GetRoot());
  fFile->Connect("Activated(Int_t)", "AliceO2::QualityControl::Gui::SpyMainFrame", this, "menuHandler(Int_t)");
  fFile->AddEntry("Exit", FILE_EXIT);
  fMenuBar->AddPopup("&File", fFile, new TGLayoutHints(kLHintsTop | kLHintsLeft));

  // browser
  fObjectsListFrame = new TGVerticalFrame(fObjectsBrowserFrame, 200, 576, kChildFrame | kFixedWidth | kSunkenFrame);
  fObjectsBrowserFrame->AddFrame(fObjectsListFrame, new TGLayoutHints(kLHintsLeft | kLHintsExpandY, 0, 0, 0, 0));
  fCanvas = new TRootEmbeddedCanvas("embedded", fObjectsBrowserFrame, 100, 100);
  fObjectsBrowserFrame->AddFrame(fCanvas, new TGLayoutHints(kLHintsExpandY | kLHintsExpandX));

  // Usual conclusion of a ROOT gui
  MapSubwindows();
  Resize();
  MapWindow();
}

void SpyMainFrame::closeWindow()
{
  // Got close message for this MainFrame. Terminates the application.
  mController->stop();
  gApplication->Terminate();
}

void SpyMainFrame::menuHandler(Int_t id)
{
  switch (id) {
    case SpyMainFrame::FILE_EXIT:
      closeWindow();
      break;
    default:
      break;
  };
}

void SpyMainFrame::displayObject(TObject *obj)
{
  fCanvas->GetCanvas()->cd();
  if(mDrawnObject) {
    delete mDrawnObject;
    gPad->Clear();
  }
  mDrawnObject = obj->DrawClone();
  gPad->Modified();
  gPad->Update();
  gSystem->ProcessEvents();
}

void SpyMainFrame::updateList(string name)
{
  if(mMapButtons.count(name) == 0) {
    TGTextButton *button = new TGTextButton(fObjectsListFrame, name.c_str());
    fObjectsListFrame->AddFrame(button,new TGLayoutHints(kLHintsExpandX | kLHintsTop));
    string methodCall = string("displayObject(=\"") + name + "\")";
    button->Connect("Clicked()", "AliceO2::QualityControl::Gui::SpyDevice", mController, methodCall.c_str());
    mMapButtons[name] = button;
    this->MapSubwindows();
    this->Resize();
    gSystem->ProcessEvents();
  } else if(mDrawnObject){
    // if it is the one we display, redraw it
    string noSpaceName = mDrawnObject->GetName();
    boost::erase_all(noSpaceName, " ");
    if(noSpaceName == name) {
      // TODO ideally here you would use a slot
      mController->displayObject(name.c_str());
    }
  }

}

} /* namespace Gui */
} /* namespace QualityControl */
} /* namespace AliceO2 */
