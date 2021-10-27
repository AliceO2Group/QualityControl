// Copyright CERN and copyright holders of ALICE O2. This software is
// // distributed under the terms of the GNU General Public License v3 (GPL
// // Version 3), copied verbatim in the file "COPYING".
// //
// // See http://alice-o2.web.cern.ch/license for full licensing information.
// //
// // In applying this license CERN does not waive the privileges and immunities
// // granted to it by virtue of its status as an Intergovernmental Organization
// // or submit itself to any jurisdiction.
//
///
///  ITSNoisyPixelTask.cxx
///
///

#include "QualityControl/QcInfoLogger.h"
#include "ITS/ITSNoisyPixelTask.h"

#include <sstream>
#include <TCanvas.h>
#include <DataFormatsParameters/GRPObject.h>
#include <ITSMFTReconstruction/DigitPixelReader.h>
#include <DetectorsCommonDataFormats/NameConf.h>
#include <DataFormatsITSMFT/ROFRecord.h>
#include <ITSMFTReconstruction/ChipMappingITS.h>
#include <DataFormatsITSMFT/ClusterTopology.h>
#include <Framework/InputRecord.h>

using o2::itsmft::Digit;
using namespace o2::itsmft;
using namespace o2::its;

namespace o2::quality_control_modules::its
{

ITSNoisyPixelTask::ITSNoisyPixelTask() : TaskInterface()
{
  o2::base::GeometryManager::loadGeometry(mGeomPath.c_str());
  mGeom = o2::its::GeometryTGeo::Instance();
}

ITSNoisyPixelTask::~ITSNoisyPixelTask()
{

  if (mEnableOrderedHitsObject) {
    delete hOrderedHitsAddressIB;
    delete hOrderedHitsAddressML;
    delete hOrderedHitsAddressOL;
  }

  for (Int_t iLayer = 0; iLayer < NLayer; iLayer++) {

    if (!mEnableLayers[iLayer])
      continue;

    // INNER BARREL
    if (iLayer < 3) {

      delete hOccupancyIB[iLayer];

      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++)
        delete hNoisyPixelMapIB[iLayer][iStave];

    }

    // OUTER BARREL
    else {

      delete hOccupancyOB[iLayer - 3];

      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++)
        delete hNoisyPixelMapOB[iLayer - 3][iStave];
    }
  }
}

void ITSNoisyPixelTask::initialize(o2::framework::InitContext& /*ctx*/)
{

  ILOG(Info, Support) << "initialize ITSNoisyPixelTask" << AliceO2::InfoLogger::InfoLogger::endm;

  getJsonParameters();
  createAllHistos();

  publishHistos();
  std::ifstream file(mDictPath.c_str());

  if (file.good()) {
    LOG(INFO) << "Running with dictionary: " << mDictPath;
    mDict.readBinaryFile(mDictPath);
  } else {
    LOG(INFO) << "Running without dictionary !";
  }
}

void ITSNoisyPixelTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ITSNoisyPixelTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ITSNoisyPixelTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  int difference;
  start = std::chrono::high_resolution_clock::now();

  ILOG(Info, Support) << "START DOING QC General" << AliceO2::InfoLogger::InfoLogger::endm;
  auto clusArr = ctx.inputs().get<gsl::span<o2::itsmft::CompClusterExt>>("compclus");
  auto digits = ctx.inputs().get<const std::vector<o2::itsmft::Digit>>("digits");

  auto clusRofArr = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("clustersrof");
  int ROFCycle = clusRofArr.size();

  int lay = -1, sta, ssta, mod, chip;

  int col = 0, row = 0, ChipID = 0;

  uint64_t label;

  // re-normalizing occupancy histograms before filling them with new hits
  long double norm_factor = (long double)mROFcounter / (long double)(mROFcounter + ROFCycle);

  for (Int_t iLayer = 0; iLayer < NLayer; iLayer++) {
    if (!mEnableLayers[iLayer])
      continue;

    if (iLayer < 3)
      hOccupancyIB[iLayer]->Scale(norm_factor);

    else

      hOccupancyOB[iLayer - 3]->Scale(norm_factor);
  }

  for (auto&& pixeldata : digits) {
    ChipID = pixeldata.getChipIndex();

    col = pixeldata.getColumn();
    row = pixeldata.getRow();

    label = 1024 * 512;
    label = label * ChipID + 1024 * row + col;

    hashtable[label]++;

    mGeom->getChipId(ChipID, lay, sta, ssta, mod, chip);

    if (lay < 3) {

      Double_t Addr[3] = { (double)col, (double)row, (double)chip };
      hOccupancyIB[lay]->Fill(chip + 1, sta, 1. / ((mROFcounter + ROFCycle) * 5.24e5));
      hNoisyPixelMapIB[lay][sta]->Fill(Addr);

    } else {

      std::vector<int> XY = MapOverHIC(col, row, chip);
      Double_t Addr[3] = { (double)XY[0], (double)XY[1], (double)mod };

      hOccupancyOB[lay - 3]->Fill(mod, sta, 1. / ((mROFcounter + ROFCycle) * 5.24e5 * mNChipsPerHic[lay]));
      hNoisyPixelMapOB[lay - 3][sta]->Fill(Addr);
    }
  }

  mROFcounter += ROFCycle;
  mROFcycle += ROFCycle;

  // SHOWING NOISY PIXELS IN ORDER OF HITS
  // x-axis label will be displayed as L<layer>_<stave>-<chip>;<column>;<row>
  if (mEnableOrderedHitsObject && mROFcycle >= mOccUpdateFrequency) {

    hOrderedHitsAddressIB->Reset();
    hOrderedHitsAddressML->Reset();
    hOrderedHitsAddressOL->Reset();

    std::multimap<int, uint64_t, std::greater<int>> orderedHitsIB;
    std::multimap<int, uint64_t, std::greater<int>> orderedHitsML;
    std::multimap<int, uint64_t, std::greater<int>> orderedHitsOL;

    for (auto [key, value] : hashtable) {

      int chipid_ = (int)(key / (1024 * 512));

      mGeom->getChipId(chipid_, lay, sta, ssta, mod, chip);

      if (!mEnableLayers[lay])
        continue;

      if (lay < 3)
        orderedHitsIB.insert({ value, key });
      else if (lay < 5)
        orderedHitsML.insert({ value, key });
      else
        orderedHitsOL.insert({ value, key });
    }

    int counterbin = 1;
    for (auto [kkey, vvalue] : orderedHitsIB) {

      int chipid_ = (int)(vvalue / (1024 * 512));
      mGeom->getChipId(chipid_, lay, sta, ssta, mod, chip);

      int column_ = (int)((vvalue % (1024 * 512)) % 1024);
      int row_ = (int)((vvalue % (1024 * 512)) / 1024);
      hOrderedHitsAddressIB->GetXaxis()->SetBinLabel(counterbin, Form("L%d_%d-%d;%d;%d", lay, sta, chip, column_, row_));

      hOrderedHitsAddressIB->SetBinContent(counterbin, 1. * kkey / mROFcounter);

      if (counterbin == nmostnoisy) {
        counterbin = 1;
        break;
      } else
        counterbin++;
    }

    for (auto [kkey, vvalue] : orderedHitsML) {

      int chipid_ = (int)(vvalue / (1024 * 512));
      mGeom->getChipId(chipid_, lay, sta, ssta, mod, chip);

      int column_ = (int)((vvalue % (1024 * 512)) % 1024);
      int row_ = (int)((vvalue % (1024 * 512)) / 1024);
      hOrderedHitsAddressML->GetXaxis()->SetBinLabel(counterbin, Form("L%d_%d-%d;%d;%d", lay, sta, chip, column_, row_));

      hOrderedHitsAddressML->SetBinContent(counterbin, 1. * kkey / mROFcounter);

      if (counterbin == nmostnoisy) {
        counterbin = 1;
        break;
      } else
        counterbin++;
    }

    for (auto [kkey, vvalue] : orderedHitsOL) {

      int chipid_ = (int)(vvalue / (1024 * 512));
      mGeom->getChipId(chipid_, lay, sta, ssta, mod, chip);

      int column_ = (int)((vvalue % (1024 * 512)) % 1024);
      int row_ = (int)((vvalue % (1024 * 512)) / 1024);
      hOrderedHitsAddressOL->GetXaxis()->SetBinLabel(counterbin, Form("L%d_%d-%d;%d;%d", lay, sta, chip, column_, row_));

      hOrderedHitsAddressOL->SetBinContent(counterbin, 1. * kkey / mROFcounter);

      if (counterbin == nmostnoisy) {
        counterbin = 1;
        break;
      } else
        counterbin++;
    }

    mROFcycle = 0;
  }

  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  mTotalTimeInQCTask += difference;
  ILOG(Info) << "Time in QC Noisy Pixel Task:  " << difference << ENDM;
}

void ITSNoisyPixelTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ITSNoisyPixelTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ITSNoisyPixelTask::reset()
{
  ILOG(Info, Support) << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;

  if (mEnableOrderedHitsObject) {
    hOrderedHitsAddressIB->Reset();
    hOrderedHitsAddressML->Reset();
    hOrderedHitsAddressOL->Reset();
  }

  for (Int_t iLayer = 0; iLayer < NLayer; iLayer++) {
    if (!mEnableLayers[iLayer])
      continue;

    // INNER BARREL
    if (iLayer < 3) {
      hOccupancyIB[iLayer]->Reset();

      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++)
        hNoisyPixelMapIB[iLayer][iStave]->Reset();

    } //END INNER BARREL
    // OUTER BARRREL
    else {

      hOccupancyOB[iLayer - 3]->Reset();

      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++)
        hNoisyPixelMapOB[iLayer - 3][iStave]->Reset();
    }
  }
}

void ITSNoisyPixelTask::createAllHistos()
{

  if (mEnableOrderedHitsObject) {
    hOrderedHitsAddressIB = new TH1D("OrderedHitsAddressIB", "OrderedHitsAddresIB", nmostnoisy, 0, nmostnoisy);
    hOrderedHitsAddressIB->SetTitle("Most noisy pixels in IB");
    formatAxes(hOrderedHitsAddressIB, "#", "Hits/ROF");
    addObject(hOrderedHitsAddressIB);

    hOrderedHitsAddressML = new TH1D("OrderedHitsAddressML", "OrderedHitsAddresML", nmostnoisy, 0, nmostnoisy);
    hOrderedHitsAddressML->SetTitle("Most noisy pixels in ML");
    formatAxes(hOrderedHitsAddressML, "#", "Hits/ROF");
    addObject(hOrderedHitsAddressML);

    hOrderedHitsAddressOL = new TH1D("OrderedHitsAddressOL", "OrderedHitsAddresOL", nmostnoisy, 0, nmostnoisy);
    hOrderedHitsAddressOL->SetTitle("Most noisy pixels in OL");
    formatAxes(hOrderedHitsAddressOL, "#", "Hits/ROF");
    addObject(hOrderedHitsAddressOL);
  }

  for (Int_t iLayer = 0; iLayer < NLayer; iLayer++) {

    if (!mEnableLayers[iLayer])
      continue;

    // INNER BARREL
    if (iLayer < 3) {

      hOccupancyIB[iLayer] = new TH2D(Form("Layer%d/PixelOccupancy", iLayer), Form("Layer%dPixelOccupancy", iLayer), mNChipsPerHic[iLayer], 0, mNChipsPerHic[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hOccupancyIB[iLayer]->SetTitle(Form("Hits per pixel per ROF, L%d", iLayer));
      addObject(hOccupancyIB[iLayer]);
      formatAxes(hOccupancyIB[iLayer], "Chip Number", "Stave Number", 1, 1.10);
      hOccupancyIB[iLayer]->SetStats(0);

      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {

        Int_t thn_binning[3] = { 1024, 512, mNChipsPerHic[iLayer] };
        Double_t thn_min[3] = { 0, 0, 0 }, thn_max[3] = { 1024, 512, (double)mNChipsPerHic[iLayer] };
        hNoisyPixelMapIB[iLayer][iStave] = new THnSparseD(Form("Layer%d/Stave%d/NoisyPixelMapVsChip", iLayer, iStave), Form("Layer%dStave%dNoisyPixelMapVsChip", iLayer, iStave), 3, thn_binning, thn_min, thn_max);
        hNoisyPixelMapIB[iLayer][iStave]->SetTitle(Form("Pixels Hits Map on L%d_%d. x:colum, y:row, z:chip", iLayer, iStave));
        addObject(hNoisyPixelMapIB[iLayer][iStave]);

      } // end of per-stave objects
    }   // END OF INNER BARREL
    // OUTER BARREL
    else {

      hOccupancyOB[iLayer - 3] = new TH2D(Form("Layer%d/PixelOccupancy", iLayer), Form("Layer%dPixelOccupancy", iLayer), mNHicPerStave[iLayer], 0, mNHicPerStave[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hOccupancyOB[iLayer - 3]->SetTitle(Form("Hits per pixel per ROF L%d", iLayer));
      addObject(hOccupancyOB[iLayer - 3]);
      formatAxes(hOccupancyOB[iLayer - 3], "HIC Number", "Stave Number", 1, 1.10);
      hOccupancyOB[iLayer - 3]->SetStats(0);

      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {

        Int_t thn_binning_ob[3] = { 1024 * 7, 512 * 2, mNHicPerStave[iLayer] };
        Double_t thn_min_ob[3] = { 0, 0, 0 }, thn_max_ob[3] = { 1024 * 7, 512 * 2, (double)mNHicPerStave[iLayer] };
        hNoisyPixelMapOB[iLayer - 3][iStave] = new THnSparseD(Form("Layer%d/Stave%d/NoisyPixelMapVsHIC", iLayer, iStave), Form("Layer%dStave%dNoisyPixelMapVsHIC", iLayer, iStave), 3, thn_binning_ob, thn_min_ob, thn_max_ob);
        hNoisyPixelMapOB[iLayer - 3][iStave]->SetTitle(Form("Pixels Hits Map on L%d_%d. x:colum along HIC, y:row along HIC, z:HIC", iLayer, iStave));
        addObject(hNoisyPixelMapOB[iLayer - 3][iStave]);
      }
    }
  }
}

void ITSNoisyPixelTask::getJsonParameters()
{
  mDictPath = mCustomParameters["clusterDictionaryPath"];
  mGeomPath = mCustomParameters["geomPath"];
  mOccUpdateFrequency = std::stoi(mCustomParameters["orderedPlotsUpdateFrequency"]);
  int request_nmostnoisy = std::stoi(mCustomParameters["orderedPlotsBinNumber"]);
  if (request_nmostnoisy > 0)
    nmostnoisy = request_nmostnoisy;
  mEnableOrderedHitsObject = (mOccUpdateFrequency >= 0 && request_nmostnoisy > 0);

  for (int ilayer = 0; ilayer < NLayer; ilayer++) {

    if (mCustomParameters["layer"][ilayer] != '0') {
      mEnableLayers[ilayer] = 1;
      LOG(INFO) << "enable layer : " << ilayer;
    } else {
      mEnableLayers[ilayer] = 0;
    }
  }
}

void ITSNoisyPixelTask::addObject(TObject* aObject)
{
  if (!aObject) {
    LOG(INFO) << " ERROR: trying to add non-existent object ";
    return;
  } else
    mPublishedObjects.push_back(aObject);
}

void ITSNoisyPixelTask::formatAxes(TH1* h, const char* xTitle, const char* yTitle, float xOffset, float yOffset)
{
  h->GetXaxis()->SetTitle(xTitle);
  h->GetYaxis()->SetTitle(yTitle);
  h->GetXaxis()->SetTitleOffset(xOffset);
  h->GetYaxis()->SetTitleOffset(yOffset);
}

std::vector<int> ITSNoisyPixelTask::MapOverHIC(int col, int row, int chip)
{
  /*
   Assuming the following order:
   -----------------------------
   | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
   -----------------------------
   | 7 | 8 | 9 |10 |11 |12 |13 |
   -----------------------------
   */
  int X, Y;
  if (chip < 7) {
    X = (6 - chip) * 1024 + col;
    Y = 512 + row;
  }
  if (chip >= 7) {
    X = (chip - 7) * 1024 + col;
    Y = row;
  }
  std::vector<int> V{ X, Y };
  return V;
}

void ITSNoisyPixelTask::publishHistos()
{
  for (unsigned int iObj = 0; iObj < mPublishedObjects.size(); iObj++) {
    getObjectsManager()->startPublishing(mPublishedObjects.at(iObj));
    LOG(INFO) << " Object will be published: " << mPublishedObjects.at(iObj)->GetName();
  }
}

} // namespace o2::quality_control_modules::its
