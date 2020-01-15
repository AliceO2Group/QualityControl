///
/// \file   ITSRawTask.h
/// \author Zhaozhong Shi
/// \author Markus Keil
/// \author Mario Sitta
/// \brief  ITS QC task for raw data analysis
///

#ifndef QC_MODULE_ITS_ITSRAWTASK_H
#define QC_MODULE_ITS_ITSRAWTASK_H

#include "QualityControl/TaskInterface.h"

#include <TH2F.h>
#include <TPaveText.h>
#include <TEllipse.h>
#include <ITSMFTReconstruction/RawPixelReader.h>
#include <ITSBase/GeometryTGeo.h>
#include <ITSMFTReconstruction/DigitPixelReader.h>

class TH1F;

using namespace o2::quality_control::core;

namespace o2 {
namespace quality_control_modules {
namespace its {

class ITSRawTask: public TaskInterface // todo add back the "final" when doxygen is fixed
{

    using ChipPixelData = o2::itsmft::ChipPixelData;
    using PixelReader = o2::itsmft::PixelReader;

  public:
    /// \brief Constructor
    ITSRawTask();
    /// Destructor
    ~ITSRawTask() override;

    // Definition of the methods for the template method pattern
    void initialize(o2::framework::InitContext &ctx) override;
    void startOfActivity(Activity &activity) override;
    void startOfCycle() override;
    void monitorData(o2::framework::ProcessingContext &ctx) override;
    void endOfCycle() override;
    void endOfActivity(Activity &activity) override;
    void reset() override;
    void setNChips(int n)
    {
      mChips.resize(n);
      mChipsOld.resize(n);
    }
    void ConfirmXAxis(TH1 *h);
    void ReverseYAxis(TH1 *h);

  private:
    void createHistos();
    void createGlobalHistos();
   // void createIBHistos(int aLayer);
    void createLayerHistos(int aLayer);
    void createStaveHistos(int aLayer, int aStave);
    void createEtaPhiHitmap(int aLayer);
    void createChipStaveOcc(int aLayer);
    void createHicHistos(int aLayer, int aStave, int aHic);
    void publishHistos();
    void addMetadata(int runID, int EpID, int fileID);
    void formatAxes(TH1 *h, const char* xTitle, const char* yTitle, float xOffset = 1., float yOffset = 1.);
    void formatPaveText(TPaveText *aPT, float aTextSize, Color_t aTextColor, short aTextAlign, const char *aText);
    void getHicCoordinates (int aLayer, int aChip, int aCol, int aRow, int& aHicRow, int& aHicCol);
    void getProcessStatus (int aInfoFile, int& aFileFinish);
    void updateFile (int aRunID, int aEpID,int aFileID);
    void resetHitmaps();
    void resetOccupancyPlots();
    void updateOccupancyPlots(int nEvents);
    void addObject(TObject* aObject, bool published = true);
    void enableLayers();
    void formatStatistics(TH2 *h); 
    void format2DZaxis(TH2 *h);    

    ChipPixelData *mChipData = nullptr;
    std::vector<ChipPixelData> mChips;
    std::vector<ChipPixelData> mChipsOld;
    o2::itsmft::PixelReader *mReader = nullptr;
    std::unique_ptr<o2::itsmft::DigitPixelReader> mReaderMC;
    o2::itsmft::RawPixelReader<o2::itsmft::ChipMappingITS> mReaderRaw;
    o2::itsmft::ChipInfo chipInfo;
    UInt_t mCurrROF = o2::itsmft::PixelData::DummyROF;
    int *mCurr; // pointer on the 1st row of currently processed mColumnsX
    int *mPrev; // pointer on the 1st row of previously processed mColumnsX
    static constexpr int NCols = 1024;
    static constexpr int NRows = 512;
    const int NColHis = 1024;
    const int NRowHis = 512;

    int mSizeReduce = 4;

    const int occUpdateFrequency = 1000000;
    
    int mDivisionStep = 32;
    static constexpr int NPixels = NRows * NCols;
    static constexpr int NLayer = 7;
    static constexpr int NLayerIB = 3;

    const int ChipBoundary[NLayer + 1] = { 0, 108, 252, 432, 3120, 6480, 14712, 24120 };
    const int NStaves[NLayer] = { 12, 16, 20, 24, 30, 42, 48 };
    const int nHicPerStave[NLayer] = {1, 1, 1, 8, 8, 14, 14};
    const int nChipsPerHic[NLayer] = {9, 9, 9, 14, 14, 14, 14};
    int mlayerEnable[NLayer] = {0, 0, 0, 0, 0, 0, 0};
    const float etaCoverage[NLayer] = {2.5, 2.3, 2.0, 1.5, 1.4, 1.4, 1.3};
    const double PhiMin = 0;
    const double PhiMax = 3.284; //???

    TH1D *hErrorPlots;
    TH1D *hFileNameInfo;
    TH2D *hErrorFile;
    TH1D *hInfoCanvas;
        
    TH1D *hOccupancyPlot[NLayer];
    TH2I *hEtaPhiHitmap[NLayer];
    TH2D *hChipStaveOccupancy[NLayer];
    TH2I *hHicHitmap[7][48][14];
    TH2I *hChipHitmap[7][48][14][14];
    TH2I *hIBHitmap[3];
    const std::vector<o2::itsmft::Digit> *mDigits = nullptr;

    o2::its::GeometryTGeo *gm = o2::its::GeometryTGeo::Instance();

    static constexpr int NError = 11;
    std::array<unsigned int, NError> mErrors;
    std::array<unsigned int, NError> mErrorPre;
    std::array<unsigned int, NError> mErrorPerFile;

    //unsigned int Error[NError];
    TPaveText *pt[NError];
    TPaveText *ptFileName;
    TPaveText *ptNFile;
    TPaveText *ptNEvent;
    TPaveText *bulbGreen;
    TPaveText *bulbRed;
    TPaveText *bulbYellow;

    std::vector<TObject*> m_objects;
    std::vector<TObject*> m_publishedObjects;

    TString ErrorType[NError] = { "Error ID 1: ErrPageCounterDiscontinuity", "Error ID 2: ErrRDHvsGBTHPageCnt",
        "Error ID 3: ErrMissingGBTHeader", "Error ID 4: ErrMissingGBTTrailer", "Error ID 5: ErrNonZeroPageAfterStop",
        "Error ID 6: ErrUnstoppedLanes", "Error ID 7: ErrDataForStoppedLane", "Error ID 8: ErrNoDataForActiveLane",
        "Error ID 9: ErrIBChipLaneMismatch", "Error ID 10: ErrCableDataHeadWrong",
        "Error ID 11: Jump in RDH_packetCounter" };
    const int NFiles = 24;
    TEllipse *bulb;

    int mTotalDigits = 0;
    int mNEvent;
    int mNEventPre;
    int mTotalFileDone;
    //	int FileRest;

    int mCounted;
    int mTotalCounted = 10000;
    int mYellowed;
};

} // namespace simpleds
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_ITS_ITSRAWTASK_H

