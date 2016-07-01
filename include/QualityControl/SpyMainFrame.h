///
/// \file   SpyMainFrame.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_SRC_QCSPY_H_
#define QUALITYCONTROL_SRC_QCSPY_H_

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

namespace AliceO2 {
namespace QualityControl {
namespace Gui {

class SpyDevice;

class SpyMainFrame: public TGMainFrame
{
  public:
    /**
     * \param spyDevice Needed to connect the buttons to their slot in the Device that acts as Controller.
     */
    SpyMainFrame(SpyDevice *spyDevice);
    virtual ~SpyMainFrame();

    enum MenuIDs
    {
      FILE_EXIT,
    };

    void closeWindow();
    void menuHandler(Int_t id);
    void updateList(std::string name);
    /**
     * \param listName The name as it appears in the list and that might be different from obj->GetName()
     */
    void displayObject(TObject *obj);
    void start(); // slot
    void stop(); // slot
    void displayObject(const char* objectName); // slot

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
    TGLabel *mTypeLabel, *mAddressLabel;
    TGTextEntry *mAddressField;
    TGTextButton *mStartButton, *mStopButton;
    TGComboBox *mTypeField;

    SpyDevice* mController;
    TObject* mDrawnObject;

  ClassDef(SpyMainFrame,1);
};

} /* namespace Gui */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_SRC_QCSPY_H_ */
