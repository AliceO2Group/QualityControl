///
/// \file   SpyMainFrame.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/SpyMainFrame.h"
#include <TROOT.h>
#include <TApplication.h>
#include <TH1F.h>
#include <TCanvas.h>
#include "QualityControl/SpyDevice.h"
#include <TSystem.h>
#include <TClass.h>
#include <TGButtonGroup.h>
#include <QualityControl/DatabaseFactory.h>
#include <Configuration/Configuration.h>
#include <Configuration/ConfigurationFactory.h>

using namespace std;
using namespace AliceO2::QualityControl::Repository;
using namespace AliceO2::Configuration;

namespace AliceO2 {
namespace QualityControl {
namespace Gui {

SpyMainFrame::SpyMainFrame(SpyDevice *spyDevice, string configurationSource)
  : TGMainFrame(gClient->GetRoot(), 1024, 640, kFixedSize), mController(spyDevice), mDrawnObject(nullptr),
    mDbRunning(false)
{
  if (configurationSource.length() > 0) {
    try {
      unique_ptr<ConfigurationInterface> config = ConfigurationFactory::getConfiguration(configurationSource);
      mDbInterface = DatabaseFactory::create("MySql");
      mDbInterface->connect(config->get<string>("database/host").value(),
                            config->get<string>("database/name").value(),
                            config->get<string>("database/username").value(),
                            config->get<string>("database/password").value());
    } catch (std::string s) {
      std::cerr << s << endl;
      throw;
    } catch (...) { // catch already here the configuration exception and print it
      // because if we are in a constructor, the exception could be lost
      string diagnostic = boost::current_exception_diagnostic_information();
      std::cerr << "Unexpected exception, diagnostic information follows:\n" << diagnostic << endl;
      throw;
    }
  } else {
    mDbInterface = 0;
  }
  cout << "mDbInterface : " << mDbInterface << endl;

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
  std::map<std::string, TGButton *>::iterator iter;
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
  auto *fFile = new TGPopupMenu(gClient->GetRoot());
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
  mSourceLabel = new TGLabel(mBottomButtonFrame, "Source :");
  mBottomButtonFrame->AddFrame(mSourceLabel, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 8));
  mRadioButtonGroup = new TGHButtonGroup(mBottomButtonFrame);
  mRadioButtonGroup->SetRadioButtonExclusive(kTRUE);
  mSourceFairmq = new TGRadioButton(mRadioButtonGroup, "FairMQ");
  mSourceDb = new TGRadioButton(mRadioButtonGroup, "Database");
  mSourceFairmq->SetOn();
  if (mDbInterface == 0) {
    mSourceDb->SetEnabled(false);
    mSourceDb->SetToolTipText("Pass a config file to enable the database option.");
  }
  mSourceDb->Connect("Toggled(Bool_t)", "AliceO2::QualityControl::Gui::SpyMainFrame", this, "ToggleSource(Bool_t)");
  mSourceFairmq->Connect("Toggled(Bool_t)", "AliceO2::QualityControl::Gui::SpyMainFrame", this, "ToggleSource(Bool_t)");
  mBottomButtonFrame->AddFrame(mRadioButtonGroup, new TGLayoutHints(kLHintsCenterY | kLHintsLeft));

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
  mAddressField->Resize(200, 18);
  mAddressField->SetText("tcp://localhost:5556");
  mBottomButtonFrame->AddFrame(mAddressField, new TGLayoutHints(kLHintsCenterY | kLHintsLeft));

  mTaskLabel = new TGLabel(mBottomButtonFrame, "Task :");
  mTaskLabel->Disable(true);
  mBottomButtonFrame->AddFrame(mTaskLabel, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 25, 0, 0, 0));
  mTaskField = new TGTextEntry(mBottomButtonFrame);
  mTaskField->Resize(100, 18);
  mTaskField->SetText("myTask_1");
  mTaskField->SetEnabled(false);
  mBottomButtonFrame->AddFrame(mTaskField, new TGLayoutHints(kLHintsCenterY | kLHintsLeft));

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
  mController->stopSpy();
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

void SpyMainFrame::ToggleSource(Bool_t on)
{
  if (dbIsSelected()) {
    mTypeLabel->Disable(true);
    mTypeField->SetEnabled(false);
    mAddressLabel->Disable(true);
    mAddressField->SetEnabled(false);
    mTaskField->SetEnabled(true);
    mTaskLabel->Disable(false);
    mStartButton->SetText("Update list");
    mStopButton->SetEnabled(false);
  } else {
    stop();
    mTypeLabel->Disable(false);
    mTypeField->SetEnabled(true);
    mAddressLabel->Disable(false);
    mAddressField->SetEnabled(true);
    mTaskField->SetEnabled(false);
    mTaskLabel->Disable(true);
    mStartButton->SetText("Start");
    mStopButton->SetEnabled(false);
  }
  Resize();
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

void SpyMainFrame::displayObject(const char *objectName)
{
  // callback for the buttons of the monitorobjects. We don't use a slot in SpyDevice because we did not succeed
  // to generate a dictionary for it with the latest ROOT.
  if (dbIsSelected()) {
    dbDisplayObject(objectName);
  } else {
    // TODO ideally here you would use a slot
    mController->displayObject(objectName);
  }
}

void SpyMainFrame::updateList(string name, string taskName)
{
  if (mMapButtons.count(name) == 0) { // object unknown yet
    auto *button = new TGTextButton(mObjectsListFrame, name.c_str());
    mObjectsListFrame->AddFrame(button, new TGLayoutHints(kLHintsExpandX | kLHintsTop));
    string methodCall = string("displayObject(=\"");
    methodCall += taskName.length() > 0 ? taskName + "/" : "";
    methodCall += name + "\")";
    button->Connect("Clicked()", "AliceO2::QualityControl::Gui::SpyMainFrame", this, methodCall.c_str());
    mMapButtons[name] = button;
    this->MapSubwindows();
    this->Resize();
    gSystem->ProcessEvents();
  } else if (mDrawnObject) { // object is known and it is the one displayed -> redraw it
    string noSpaceName = mDrawnObject->GetName();
    boost::erase_all(noSpaceName, " ");
    if (noSpaceName == name) {
      displayObject(name.c_str());
    }
  }
}

void SpyMainFrame::start()
{
  if (dbIsSelected()) {
    dbRun();
  } else {
    mSourceFairmq->SetEnabled(false);
    mSourceDb->SetEnabled(false);
    mSourceLabel->Disable(true);
    mStopButton->SetEnabled(true);
    mStartButton->SetEnabled(false);
    string address = mAddressField->GetText();
    string type = mTypeField->GetSelectedEntry()->GetTitle();
    mController->startChannel(address, type);
  }
}

void SpyMainFrame::stop()
{
  removeAllObjectsButtons();
  // tell the controller
  if (dbIsSelected()) {
    mDbRunning = false;
  } else {
    // toggle buttons and remove list
    if (!mSourceFairmq->IsEnabled()) {
      mSourceFairmq->SetEnabled(true);
      mSourceDb->SetEnabled(true);
      mSourceLabel->Disable(false);
    }
    mStopButton->SetEnabled(false);
    mStartButton->SetEnabled(true);
    mController->stopChannel();
  }
  delete mDrawnObject;
  mDrawnObject = nullptr;
}

bool SpyMainFrame::dbIsSelected()
{
  return mSourceDb->IsOn();
}

void SpyMainFrame::dbRun()
{
  // Get list of objects
  string s = "data_";
  s += mTaskField->GetText();
  vector<string> objectNames = mDbInterface->getPublishedObjectNames(s.c_str());

  // UpdateList for each
  for (auto name : objectNames) {
    updateList(name, mTaskField->GetText());
  }
}

void SpyMainFrame::dbDisplayObject(string objectName)
{
  string taskName = objectName.substr(0, objectName.find("/"));
  string objectNameOnly = objectName.substr(objectName.find("/") + 1);
  AliceO2::QualityControl::Core::MonitorObject *mo = mDbInterface->retrieve(taskName, objectNameOnly);
  if (mo) {
    displayObject(mo->getObject());
    delete mo;
  } else {
    cerr << "mo " << objectNameOnly << " of task " << taskName << " could not be retrieved from database" << endl;
  }
}

void SpyMainFrame::removeAllObjectsButtons()
{
  std::map<std::string, TGButton *>::iterator iter;
  for (iter = mMapButtons.begin(); iter != mMapButtons.end(); iter++) {
    // remove the button from its container
    mObjectsListFrame->RemoveFrame(iter->second);
    // hide it
    iter->second->UnmapWindow();
    // remove its parent
    iter->second->ReparentWindow(gClient->GetDefaultRoot());
    // finally delete it
    delete iter->second;
//    cout << "deleted " << iter->first << endl;
  }
  mMapButtons.clear();
  Layout();
}
} /* namespace Gui */
} /* namespace QualityControl */
} /* namespace AliceO2 */
