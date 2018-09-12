// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   SpyMainFrame.h
/// \author Barthelemy von Haller
///

#ifndef QC_GUI_SPYMAINFRAME_H
#define QC_GUI_SPYMAINFRAME_H

//std
#include <map>
// ROOT
#include <TGMdiMainFrame.h>
#include <TGStatusBar.h>
#include <TGButton.h>
#include <TGLabel.h>
#include <TGMenu.h>
#include <TRootEmbeddedCanvas.h>
#include <TGTextEntry.h>
#include <TGCanvas.h>
#include <TGComboBox.h>
#include "QualityControl/Quality.h"

namespace o2 {
namespace quality_control {
namespace repository {
class DatabaseInterface;
}
namespace gui {

class SpyDevice;

class SpyMainFrame: public TGMainFrame
{
  public:
    /**
     * \param spyDevice Needed to connect the buttons to their slot in the Device that acts as Controller.
     */
    SpyMainFrame(SpyDevice *spyDevice, std::string configurationSource);
    ~SpyMainFrame() override;

    enum MenuIDs
    {
      FILE_EXIT,
    };

    void close();
    void menuHandler(Int_t id);
    void updateList(std::string name/*, o2::quality_control::core::Quality quality*/, std::string taskName = "");
    /**
     * \param listName The name as it appears in the list and that might be different from obj->GetName()
     */
    void displayObject(TObject *obj);
    void start(); // slot
    void stop(); // slot
    void displayObject(const char* objectName); // slot
    void ToggleSource(Bool_t on);
    bool dbIsSelected();
    void dbDisplayObject(std::string objectName);
    void dbRun();

  private:
    void constructWindow();
    void removeAllObjectsButtons();

    TGHorizontalFrame *mBottomButtonFrame;
    TGHorizontalFrame *mObjectsBrowserFrame;
    TGMenuBar* mMenuBar;
    TGVerticalFrame *mObjectsListFrame;
    TGCanvas *mScrollObjectsListContainer;
    TRootEmbeddedCanvas *mCanvas;
    std::map<std::string, TGButton*> mMapButtons;
    // For the form
    TGButtonGroup *mRadioButtonGroup;
    TGRadioButton *mSourceDb, *mSourceFairmq;
    TGLabel *mSourceLabel, *mTypeLabel, *mAddressLabel;
    TGTextEntry *mAddressField;
    TGTextButton *mStartButton, *mStopButton;
    TGComboBox *mTypeField;
    TGLabel *mTaskLabel;
    TGTextEntry *mTaskField;

    SpyDevice* mController;
    TObject* mDrawnObject;
    std::unique_ptr<o2::quality_control::repository::DatabaseInterface> mDbInterface;

    bool mDbRunning;

  ClassDef(SpyMainFrame,1);
};

} /* namespace gui */
} /* namespace quality_control */
} /* namespace o2 */

#endif /* QC_GUI_SPYMAINFRAME_H */
