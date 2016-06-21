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
  if (mDrawnObject) {
    delete mDrawnObject;
  }
  std::map<std::string, TGButton*>::iterator iter;
  for (iter = mMapButtons.begin(); iter != mMapButtons.end(); iter++) {
    delete iter->second;
  }
  Disconnect(this);
}

void SpyMainFrame::constructWindow()
{
  // prepare the layout of frames from top to bottom
  mMenuBar = new TGMenuBar(this);
  AddFrame(mMenuBar, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 0, 0, 0, 0));
  mObjectsBrowserFrame = new TGHorizontalFrame(this, 1, 1, kChildFrame | kSunkenFrame);
  AddFrame(mObjectsBrowserFrame, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 0, 0, 0, 0));
  mBottomButtonFrame = new TGHorizontalFrame(this, 1, 30, kChildFrame | kSunkenFrame | kFixedHeight);
  AddFrame(mBottomButtonFrame, new TGLayoutHints(kLHintsExpandX, 0, 0, 0, 0));

// menu
  TGPopupMenu* fFile = new TGPopupMenu(gClient->GetRoot());
  fFile->Connect("Activated(Int_t)", "AliceO2::QualityControl::Gui::SpyMainFrame", this, "menuHandler(Int_t)");
  fFile->AddEntry("Exit", FILE_EXIT);
  mMenuBar->AddPopup("&File", fFile, new TGLayoutHints(kLHintsTop | kLHintsLeft));

  // browser
  mScrollObjectsListContainer = new TGCanvas(mObjectsBrowserFrame, 218, 576);
  mObjectsListFrame = new TGVerticalFrame(mScrollObjectsListContainer->GetViewPort(), 200, 576,
      kChildFrame | kFixedWidth | kSunkenFrame);
  mScrollObjectsListContainer->SetContainer(mObjectsListFrame);
  mObjectsBrowserFrame->AddFrame(mScrollObjectsListContainer,
      new TGLayoutHints(kLHintsLeft | kLHintsExpandY, 0, 0, 0, 0));
  mCanvas = new TRootEmbeddedCanvas("embedded", mObjectsBrowserFrame, 100, 100);
  mObjectsBrowserFrame->AddFrame(mCanvas, new TGLayoutHints(kLHintsExpandY | kLHintsExpandX));

  // form
  mTypeLabel = new TGLabel(mBottomButtonFrame, "Type :");
  mBottomButtonFrame->AddFrame(mTypeLabel, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 8));
  mTypeField = new TGComboBox(mBottomButtonFrame);
  mBottomButtonFrame->AddFrame(mTypeField, new TGLayoutHints(kLHintsCenterY | kLHintsLeft));
  mTypeField->AddEntry("sub", 0);
  mTypeField->Select(0);
  mTypeField->Resize(75, 18);
  mAddressLabel = new TGLabel(mBottomButtonFrame, "Address (url:port) :");
  mBottomButtonFrame->AddFrame(mAddressLabel, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 25, 0, 0, 0));
  mAddressField = new TGTextEntry(mBottomButtonFrame);
  mAddressField->Resize(300, 18);
  mAddressField->SetText("tcp://localhost:5556");
  mBottomButtonFrame->AddFrame(mAddressField, new TGLayoutHints(kLHintsCenterY | kLHintsLeft));
  mStartButton = new TGTextButton(mBottomButtonFrame);
  mStartButton->SetText("Start");
  mStartButton->Connect("Clicked()", "AliceO2::QualityControl::Gui::SpyMainFrame", this, "start()");
  mBottomButtonFrame->AddFrame(mStartButton, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 25, 0, 0, 0));
  mStopButton = new TGTextButton(mBottomButtonFrame);
  mStopButton->SetText("Stop");
  mStopButton->Connect("Clicked()", "AliceO2::QualityControl::Gui::SpyMainFrame", this, "stop()");
  mStopButton->SetEnabled(false);
  mBottomButtonFrame->AddFrame(mStopButton, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 25, 0, 0, 0));

  // Usual conclusion of a ROOT gui design
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
  mCanvas->GetCanvas()->cd();
  if (mDrawnObject) {
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
  if (mMapButtons.count(name) == 0) {
    TGTextButton *button = new TGTextButton(mObjectsListFrame, name.c_str());
    mObjectsListFrame->AddFrame(button, new TGLayoutHints(kLHintsExpandX | kLHintsTop));
    string methodCall = string("displayObject(=\"") + name + "\")";
    button->Connect("Clicked()", "AliceO2::QualityControl::Gui::SpyDevice", mController, methodCall.c_str());
    mMapButtons[name] = button;
    this->MapSubwindows();
    this->Resize();
    gSystem->ProcessEvents();
  } else if (mDrawnObject) {
    // if it is the one we display, redraw it
    string noSpaceName = mDrawnObject->GetName();
    boost::erase_all(noSpaceName, " ");
    if (noSpaceName == name) {
      // TODO ideally here you would use a slot
      mController->displayObject(name.c_str());
    }
  }
}

void SpyMainFrame::start()
{
  // toggle buttons
  mStopButton->SetEnabled(true);
  mStartButton->SetEnabled(false);
  // tell the controller
  string address = mAddressField->GetText();
  string type = mTypeField->GetSelectedEntry()->GetTitle();
  cout << "start channel with " << address << " type " << endl;
  mController->startChannel(address, type);
}

void SpyMainFrame::stop()
{
  // toggle buttons and remove list
  mStopButton->SetEnabled(false);
  mStartButton->SetEnabled(true);
  removeAllObjectsButtons();
  // tell the controller
  mController->stopChannel();
}

void SpyMainFrame::removeAllObjectsButtons()
{
  std::map<std::string, TGButton*>::iterator iter;
  for (iter = mMapButtons.begin(); iter != mMapButtons.end(); iter++) {
    // remove the button from its container
    mObjectsListFrame->RemoveFrame(iter->second);
    // hide it
    iter->second->UnmapWindow();
    // remove its parent
    iter->second->ReparentWindow(gClient->GetDefaultRoot());
    // finally delete it
    delete iter->second;
    cout << "deleted " << iter->first << endl;
  }
  mMapButtons.clear();
  Layout();
}
} /* namespace Gui */
} /* namespace QualityControl */
} /* namespace AliceO2 */
