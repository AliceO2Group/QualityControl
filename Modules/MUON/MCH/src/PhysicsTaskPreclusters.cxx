// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   PhysicsTaskPreclusters.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
/// \author Sebastien Perrin
///

#include <TCanvas.h>
#include <TH1.h>
#include <TF1.h>
#include <TH2.h>
#include <TFile.h>
#include <algorithm>

#include "MCH/PhysicsTaskPreclusters.h"
#ifdef MCH_HAS_MAPPING_FACTORY
#include "MCHMappingFactory/CreateSegmentation.h"
#endif
#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingInterface/CathodeSegmentation.h"
#include "MCHMappingSegContour/CathodeSegmentationContours.h"
#include "MCHRawDecoder/PageDecoder.h"
#include "QualityControl/QcInfoLogger.h"
#include <Framework/InputRecord.h>

using namespace std;
using namespace o2::mch::raw;
using namespace o2::quality_control::core;

//#define QC_MCH_SAVE_TEMP_ROOTFILE 1

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{
PhysicsTaskPreclusters::PhysicsTaskPreclusters() : TaskInterface() {}

PhysicsTaskPreclusters::~PhysicsTaskPreclusters() {}

void PhysicsTaskPreclusters::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize PhysicsTaskPreclusters" << AliceO2::InfoLogger::InfoLogger::endm;

  for (auto de : o2::mch::raw::deIdsForAllMCH) {

    TH2F* h = new TH2F(TString::Format("%sCluster_Charge_DE%03d", getHistoPath(de).c_str(), de),
                       TString::Format("Cluster charge (DE%03d)", de), 1000, 0, 50000, 4, 0, 4);
    h->GetYaxis()->SetBinLabel(1, "#splitline{ nB <= 1}{nNB <= 1}");
    h->GetYaxis()->SetBinLabel(2, "#splitline{ nB >= 2}{nNB <= 1}");
    h->GetYaxis()->SetBinLabel(3, "#splitline{ nB <= 1}{nNB >= 2}");
    h->GetYaxis()->SetBinLabel(4, "#splitline{ nB >= 2}{nNB >= 2}");
    mHistogramClchgDE.insert(make_pair(de, h));
    getObjectsManager()->startPublishing(h);
    TH1F* h1 = new TH1F(TString::Format("%sCluster_Charge_OnCycle_DE%03d", getHistoPath(de).c_str(), de),
                        TString::Format("Cluster charge on cycle (DE%03d)", de), 1000, 0, 50000);
    mHistogramClchgDEOnCycle.insert(make_pair(de, h1));

    h = new TH2F(TString::Format("%sCluster_Size_DE%03d", getHistoPath(de).c_str(), de),
                 TString::Format("Cluster size (DE%03d)", de), 10, 0, 10, 3, 0, 3);
    h->GetYaxis()->SetBinLabel(1, "B");
    h->GetYaxis()->SetBinLabel(2, "NB");
    h->GetYaxis()->SetBinLabel(3, "B+NB");
    mHistogramClsizeDE.insert(make_pair(de, h));
    getObjectsManager()->startPublishing(h);

    // Histograms using the XY Mapping

    {
      DetectorHistogram* hXY0 = new DetectorHistogram(TString::Format("%sPreclusters_NBgood_XY_%03d", getHistoPath(de).c_str(), de),
                                                      TString::Format("Preclusters XY (DE%03d NB, good)", de), de);
      mHistogramPreclustersXY[0].insert(make_pair(de, hXY0));
      DetectorHistogram* hXY1 = new DetectorHistogram(TString::Format("%sPreclusters_Bgood_XY_%03d", getHistoPath(de).c_str(), de),
                                                      TString::Format("Preclusters XY (DE%03d B, good)", de), de);
      mHistogramPreclustersXY[1].insert(make_pair(de, hXY1));
      DetectorHistogram* hXY2 = new DetectorHistogram(TString::Format("%sPreclustersNBgoodAndSomethingB_XY_%03d", getHistoPath(de).c_str(), de),
                                                      TString::Format("Preclusters XY (DE%03d B)", de), de);
      mHistogramPreclustersXY[2].insert(make_pair(de, hXY2));
      DetectorHistogram* hXY3 = new DetectorHistogram(TString::Format("%sPreclustersBgoodAndSomethingNB_XY_%03d", getHistoPath(de).c_str(), de),
                                                      TString::Format("Preclusters XY (DE%03d NB)", de), de);
      mHistogramPreclustersXY[3].insert(make_pair(de, hXY3));

      MergeableTH2Ratio* hXYPseudo0 = new MergeableTH2Ratio(TString::Format("%sPseudoeff_B_XY_%03d", getHistoPath(de).c_str(), de),
                                                            TString::Format("Pseudo-efficiency XY (DE%03d B)", de),
                                                            hXY2, hXY0);
      mHistogramPseudoeffXY[0].insert(make_pair(de, hXYPseudo0));
      getObjectsManager()->startPublishing(hXYPseudo0);

      MergeableTH2Ratio* hXYPseudo1 = new MergeableTH2Ratio(TString::Format("%sPseudoeff_NB_XY_%03d", getHistoPath(de).c_str(), de),
                                                            TString::Format("Pseudo-efficiency XY (DE%03d NB)", de),
                                                            hXY3, hXY1);
      mHistogramPseudoeffXY[1].insert(make_pair(de, hXYPseudo1));
      getObjectsManager()->startPublishing(hXYPseudo1);
    }
  }

  mHistogramPseudoeff_NumsAndDens[0] = new GlobalHistogram("Pseudoeff_den_Mergeable", "Count - Good clusters");
  mHistogramPseudoeff_NumsAndDens[0]->init();
  mHistogramPseudoeff_NumsAndDens[0]->SetOption("colz");
  getObjectsManager()->startPublishing(mHistogramPseudoeff_NumsAndDens[0]);
  mHistogramPseudoeff_NumsAndDens[1] = new GlobalHistogram("Pseudoeff_num_Mergeable", "Count - Good clusters associated to something on the other cathode");
  mHistogramPseudoeff_NumsAndDens[1]->init();
  mHistogramPseudoeff_NumsAndDens[1]->SetOption("colz");
  getObjectsManager()->startPublishing(mHistogramPseudoeff_NumsAndDens[1]);

  mHistogramPseudoeff = new MergeableTH2Ratio("Pseudoeff_Mergeable", "Pseudo-efficiency - Good clusters with something on other side/Good clusters",
                                              mHistogramPseudoeff_NumsAndDens[1], mHistogramPseudoeff_NumsAndDens[0]);
  getObjectsManager()->startPublishing(mHistogramPseudoeff);
  mHistogramPseudoeff->SetOption("colz");

  // 1D histograms for mean pseudoeff per DE (integrated or per elapsed cycle) - Used in trending
  mMeanPseudoeffPerDE_DoesGoodNBHaveSomethingB = new MergeableTH1PseudoEfficiencyPerDE("MeanPseudoeff_Mergeable_DoesGoodNBHaveSomethingB", "Mean Pseudoeff of each DE (Good clusters on NB associated with something on B)", mHistogramPreclustersXY[3], mHistogramPreclustersXY[1]);
  getObjectsManager()->startPublishing(mMeanPseudoeffPerDE_DoesGoodNBHaveSomethingB);
  mMeanPseudoeffPerDE_DoesGoodBHaveSomethingNB = new MergeableTH1PseudoEfficiencyPerDE("MeanPseudoeff_Mergeable_DoesGoodBHaveSomethingNB", "Mean Pseudoeff of each DE (Good clusters on B associated with something on NB)", mHistogramPreclustersXY[2], mHistogramPreclustersXY[0]);
  getObjectsManager()->startPublishing(mMeanPseudoeffPerDE_DoesGoodBHaveSomethingNB);

  mMeanPseudoeffPerDE_DoesGoodNBHaveSomethingB_Cycle = new MergeableTH1PseudoEfficiencyPerDECycle("MeanPseudoeffPerDE_DoesGoodNBHaveSomethingB_OnCycle", "Mean Pseudoeff of each DE during the cycle (Good clusters on NB associated with something on B)", mHistogramPreclustersXY[3], mHistogramPreclustersXY[1]);
  getObjectsManager()->startPublishing(mMeanPseudoeffPerDE_DoesGoodNBHaveSomethingB_Cycle);
  mMeanPseudoeffPerDE_DoesGoodBHaveSomethingNB_Cycle = new MergeableTH1PseudoEfficiencyPerDECycle("MeanPseudoeffPerDE_DoesGoodBHaveSomethingNB_OnCycle", "Mean Pseudoeff of each DE during the cycle (Good clusters on B associated with something on NB)", mHistogramPreclustersXY[2], mHistogramPreclustersXY[0]);
  getObjectsManager()->startPublishing(mMeanPseudoeffPerDE_DoesGoodBHaveSomethingNB_Cycle);

  mMPVCycle = new MergeableTH1MPVPerDECycle("MPV_Mergeable_OnCycle", "MPV of each DE cluster charge during the cycle", mHistogramClchgDEOnCycle);
  getObjectsManager()->startPublishing(mMPVCycle);
}

void PhysicsTaskPreclusters::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PhysicsTaskPreclusters::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PhysicsTaskPreclusters::monitorData(o2::framework::ProcessingContext& ctx)
{
  bool verbose = false;
  // get the input preclusters and associated digits
  auto preClusters = ctx.inputs().get<gsl::span<o2::mch::PreCluster>>("preclusters");
  auto digits = ctx.inputs().get<gsl::span<o2::mch::Digit>>("preclusterdigits");

  bool print = false;
  for (auto& p : preClusters) {
    if (!plotPrecluster(p, digits)) {
      print = true;
    }
  }

  if (print && verbose) {
    printPreclusters(preClusters, digits);
  }
}

//_________________________________________________________________________________________________
// compute the center-of-gravity of a given pre-cluster
static void CoG(gsl::span<const o2::mch::Digit> precluster, double& Xcog, double& Ycog, bool isWide[2])
{

  double xmin = 1E9;
  double ymin = 1E9;
  double xmax = -1E9;
  double ymax = -1E9;
  double charge[] = { 0.0, 0.0 };
  int multiplicity[] = { 0, 0 };
  isWide[0] = isWide[1] = false;
  // isWide tells if a given precluster is extended enough on a given cathode. On the bending side for exemple, a wide precluster would have at least 2 pads fired in the x direction (so when clustering it, we obtain a meaningful value for x). If a precluster is not wide and on a single cathode, when clustering, one of the coordinates will not be computed properly and set to the center of the pad by default.

  double x[] = { 0.0, 0.0 };
  double y[] = { 0.0, 0.0 };

  double xsize[] = { 0.0, 0.0 };
  double ysize[] = { 0.0, 0.0 };

  std::set<double> padPos[2];

  int detid = precluster[0].getDetID();
  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(detid);

  for (ssize_t i = 0; (unsigned)i < precluster.size(); ++i) {
    const o2::mch::Digit& digit = precluster[i];
    int padid = digit.getPadID();

    // position and size of current pad
    double padPosition[2] = { segment.padPositionX(padid), segment.padPositionY(padid) };
    double padSize[2] = { segment.padSizeX(padid), segment.padSizeY(padid) };

    // update of xmin/max et ymin/max
    xmin = std::min(padPosition[0] - 0.5 * padSize[0], xmin);
    xmax = std::max(padPosition[0] + 0.5 * padSize[0], xmax);
    ymin = std::min(padPosition[1] - 0.5 * padSize[1], ymin);
    ymax = std::max(padPosition[1] + 0.5 * padSize[1], ymax);

    // cathode index
    int cathode = segment.isBendingPad(padid) ? 0 : 1;

    // update of the cluster position, size, charge and multiplicity
    x[cathode] += padPosition[0] * digit.getADC();
    y[cathode] += padPosition[1] * digit.getADC();
    xsize[cathode] += padSize[0];
    ysize[cathode] += padSize[1];
    charge[cathode] += digit.getADC();

    if (cathode == 0) {
      padPos[0].insert(padPosition[1]);
    } else if (cathode == 1) {
      padPos[1].insert(padPosition[0]);
    }

    multiplicity[cathode] += 1;
  }

  // Computation of the CoG coordinates for the two cathodes
  for (int cathode = 0; cathode < 2; ++cathode) {
    if (charge[cathode] != 0) {
      x[cathode] /= charge[cathode];
      y[cathode] /= charge[cathode];
    }
    if (multiplicity[cathode] != 0) {
      double sqrtCharge = sqrt(charge[cathode]);
      xsize[cathode] /= (multiplicity[cathode] * sqrtCharge);
      ysize[cathode] /= (multiplicity[cathode] * sqrtCharge);
    } else {
      xsize[cathode] = 1E9;
      ysize[cathode] = 1E9;
    }
  }

  isWide[0] = (padPos[0].size() > 1);
  isWide[1] = (padPos[1].size() > 1);

  // each CoG coordinate is taken from the cathode with the best precision
  Xcog = (xsize[0] < xsize[1]) ? x[0] : x[1];
  Ycog = (ysize[0] < ysize[1]) ? y[0] : y[1];
}

//_________________________________________________________________________________________________
bool PhysicsTaskPreclusters::plotPrecluster(const o2::mch::PreCluster& preCluster, gsl::span<const o2::mch::Digit> digits)
{
  // filter out single-pad clusters
  if (preCluster.nDigits < 2) {
    return true;
  }

  // get the digits of this precluster
  auto preClusterDigits = digits.subspan(preCluster.firstDigit, preCluster.nDigits);

  // whether a cathode has digits or not
  bool cathode[2] = { false, false };
  // total charge on each cathode
  float chargeSum[2] = { 0, 0 };
  // largest signal in each cathode
  float chargeMax[2] = { 0, 0 };
  // number of digits in each cathode
  int multiplicity[2] = { 0, 0 };

  int detid = preClusterDigits[0].getDetID();
  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(detid);

  // loop over digits and collect information on charge and multiplicity
  for (const o2::mch::Digit& digit : preClusterDigits) {
    int padid = digit.getPadID();

    // cathode index
    int cid = segment.isBendingPad(padid) ? 0 : 1;
    cathode[cid] = true;
    chargeSum[cid] += digit.getADC();
    multiplicity[cid] += 1;

    if (digit.getADC() > chargeMax[cid]) {
      chargeMax[cid] = digit.getADC();
    }
  }

  // compute center-of-gravity of the charge distribution
  double Xcog, Ycog;
  bool isWide[2];
  CoG(preClusterDigits, Xcog, Ycog, isWide);

  // criteria to define a "good" charge cluster in one cathode:
  // - two or more digits
  // - at least one digit with amplitude > 50 ADC
  bool isGood[2] = { (chargeMax[0] > 50) && (isWide[0]), (chargeMax[1] > 50) && (isWide[1]) };
  //bool isGood[2] = {isWide[0], isWide[1]};

  // Filling histograms to be used for Pseudo-efficiency computation
  if (isGood[1]) {
    // good cluster on non-bending side, check if there is data from the bending side as well
    auto hXY0 = mHistogramPreclustersXY[0].find(detid);
    if ((hXY0 != mHistogramPreclustersXY[0].end()) && (hXY0->second != NULL)) {
      hXY0->second->Fill(Xcog, Ycog, 0.5, 0.5);
    }
    if (cathode[0]) { //Check if associated to something on Bending
      auto hXY1 = mHistogramPreclustersXY[2].find(detid);
      if ((hXY1 != mHistogramPreclustersXY[2].end()) && (hXY1->second != NULL)) {
        hXY1->second->Fill(Xcog, Ycog, 0.5, 0.5);
      }
    }
  }

  if (isGood[0]) {
    // good cluster on bending side, check if there is data from the non-bending side as well
    auto hXY0 = mHistogramPreclustersXY[1].find(detid);
    if ((hXY0 != mHistogramPreclustersXY[1].end()) && (hXY0->second != NULL)) {
      hXY0->second->Fill(Xcog, Ycog, 0.5, 0.5);
    }
    if (cathode[1]) { //Check if associated to something on Non-Bending
      auto hXY1 = mHistogramPreclustersXY[3].find(detid);
      if ((hXY1 != mHistogramPreclustersXY[3].end()) && (hXY1->second != NULL)) {
        hXY1->second->Fill(Xcog, Ycog, 0.5, 0.5);
      }
    }
  }

  // This code is slow. Will be re-introduced after some optimizations
  //const o2::mch::mapping::CathodeSegmentation& csegment = segment.bending();
  //o2::mch::contour::Contour<double> envelop = o2::mch::mapping::getEnvelop(csegment);
  //std::vector<o2::mch::contour::Vertex<double>> vertices = envelop.getVertices();
  //o2::mch::contour::BBox<double> bbox = o2::mch::mapping::getBBox(csegment);
  //std::cout<<"DE "<<de<<"  BBOX "<<bbox<<std::endl;

  // skip a fiducial border around the active area, so that only fully-contained clusters are considered
  //if(Xcog < (bbox.xmin() + 5)) return true;
  //if(Xcog > (bbox.xmax() - 5)) return true;
  //if(Ycog < (bbox.ymin() + 5)) return true;
  //if(Ycog > (bbox.ymax() - 5)) return true;

  // cluster size, separately on each cathode and combined
  auto hSize = mHistogramClsizeDE.find(detid);
  if ((hSize != mHistogramClsizeDE.end()) && (hSize->second != NULL)) {
    hSize->second->Fill(multiplicity[0], 0);
    hSize->second->Fill(multiplicity[1], 1);
    hSize->second->Fill(multiplicity[0] + multiplicity[1], 2);
  }

  float chargeTot = chargeSum[0] + chargeSum[1];
  auto hCharge = mHistogramClchgDE.find(detid);
  if ((hCharge != mHistogramClchgDE.end()) && (hCharge->second != NULL)) {
    if ((multiplicity[0] <= 1) && (multiplicity[1] <= 1)) {
      hCharge->second->Fill(chargeTot, 0);
    } else if ((multiplicity[0] > 1) && (multiplicity[1] == 1)) {
      hCharge->second->Fill(chargeTot, 1);
    } else if ((multiplicity[0] <= 1) && (multiplicity[1] > 1)) {
      hCharge->second->Fill(chargeTot, 2);
    } else if ((multiplicity[0] > 1) && (multiplicity[1] > 1)) {
      hCharge->second->Fill(chargeTot, 3);
    }
  }

  auto hChargeOnCycle = mHistogramClchgDEOnCycle.find(detid);
  if ((hChargeOnCycle != mHistogramClchgDEOnCycle.end()) && (hChargeOnCycle->second != NULL)) {
    hChargeOnCycle->second->Fill(chargeTot);
  }

  return (cathode[0] && cathode[1]);
}

//_________________________________________________________________________________________________
void PhysicsTaskPreclusters::printPrecluster(gsl::span<const o2::mch::Digit> preClusterDigits)
{
  float chargeSum[2] = { 0, 0 };
  float chargeMax[2] = { 0, 0 };

  int detid = preClusterDigits[0].getDetID();
  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(detid);

  for (ssize_t i = 0; (unsigned)i < preClusterDigits.size(); ++i) {
    const o2::mch::Digit& digit = preClusterDigits[i];
    int padid = digit.getPadID();

    // cathode index
    int cid = segment.isBendingPad(padid) ? 0 : 1;
    chargeSum[cid] += digit.getADC();

    if (digit.getADC() > chargeMax[cid]) {
      chargeMax[cid] = digit.getADC();
    }
  }

  double Xcog, Ycog;
  bool isWide[2];
  CoG(preClusterDigits, Xcog, Ycog, isWide);

  ILOG(Info, Support) << "\n\n\n====================\n"
                              << "[pre-cluster] nDigits = " << preClusterDigits.size() << "  charge = " << chargeSum[0] << " " << chargeSum[1] << "   CoG = " << Xcog << "," << Ycog << AliceO2::InfoLogger::InfoLogger::endm;
  for (auto& d : preClusterDigits) {
    float X = segment.padPositionX(d.getPadID());
    float Y = segment.padPositionY(d.getPadID());
    bool bend = !segment.isBendingPad(d.getPadID());
    ILOG(Info, Support) << fmt::format("  DE {:4d}  PAD {:5d}  ADC {:6d}  TIME ({})",
                                               d.getDetID(), d.getPadID(), d.getADC(), d.getTime())
                                << "\n"
                                << fmt::format("  CATHODE {}  PAD_XY {:+2.2f} , {:+2.2f}", (int)bend, X, Y) << AliceO2::InfoLogger::InfoLogger::endm;
  }
  ILOG(Info, Support) << "\n====================\n\n"
                              << AliceO2::InfoLogger::InfoLogger::endm;
}

//_________________________________________________________________________________________________
void PhysicsTaskPreclusters::printPreclusters(gsl::span<const o2::mch::PreCluster> preClusters, gsl::span<const o2::mch::Digit> digits)
{
  for (auto& preCluster : preClusters) {
    // get the digits of this precluster
    auto preClusterDigits = digits.subspan(preCluster.firstDigit, preCluster.nDigits);
    printPrecluster(preClusterDigits);
  }
}

void PhysicsTaskPreclusters::computePseudoEfficiency()
{
  for (auto de : o2::mch::raw::deIdsForAllMCH) {
    for (int i = 0; i < 2; i++) {
      auto ih = mHistogramPreclustersXY[i + 2].find(de);
      if (ih == mHistogramPreclustersXY[i + 2].end()) {
        continue;
      }
      // Getting the histogram with the clusters positions exists (either on B, NB, or B and NB)
      TH2F* hB = ih->second;
      if (!hB) {
        continue;
      }

      // Getting the histogram with all the preclusters (denominator of pseudo-efficiency)
      ih = mHistogramPreclustersXY[i].find(de);
      if (ih == mHistogramPreclustersXY[i].end()) {
        continue;
      }
      TH2F* hAll = ih->second;
      if (!hAll) {
        continue;
      }

      // Checking the Histograms where Pseudo-efficiency should be stored and resetting them
      auto ihp = mHistogramPseudoeffXY[i].find(de);
      if (ihp == mHistogramPseudoeffXY[i].end()) {
        continue;
      }
      MergeableTH2Ratio* hEff = ihp->second;
      if (!hEff) {
        continue;
      }

      // Computing the Pseudo-efficiency by dividing the distribution of clusters (either on B, NB, or B and NB) by the total distribution of all clusters.
      ihp->second->update();
    }
  }

  // Same procedure but in GlobalHistograms
  mHistogramPseudoeff_NumsAndDens[0]->add(mHistogramPreclustersXY[0], mHistogramPreclustersXY[1]);
  mHistogramPseudoeff_NumsAndDens[1]->add(mHistogramPreclustersXY[2], mHistogramPreclustersXY[3]);

  mHistogramPseudoeff->update();

  mMeanPseudoeffPerDE_DoesGoodNBHaveSomethingB->update();
  mMeanPseudoeffPerDE_DoesGoodBHaveSomethingNB->update();

  mMeanPseudoeffPerDE_DoesGoodNBHaveSomethingB_Cycle->update();
  mMeanPseudoeffPerDE_DoesGoodBHaveSomethingNB_Cycle->update();

  mMPVCycle->update();
}

void PhysicsTaskPreclusters::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;

  computePseudoEfficiency();
}

void PhysicsTaskPreclusters::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;

#ifdef QC_MCH_SAVE_TEMP_ROOTFILE
  computePseudoEfficiency();

  TFile f("mch-qc-preclusters.root", "RECREATE");

  {
    auto hMean = mMeanPseudoeffPerDE_DoesGoodNBHaveSomethingB;
    auto hMean2 = mMeanPseudoeffPerDE_DoesGoodBHaveSomethingNB;
    auto hMeanCycle = mMeanPseudoeffPerDE_DoesGoodNBHaveSomethingB_Cycle;
    auto hMeanCycle2 = mMeanPseudoeffPerDE_DoesGoodBHaveSomethingNB_Cycle;
    hMean->Write();
    hMean2->Write();
    hMeanCycle->Write();
    hMeanCycle2->Write();
  }

  {
    for (int i = 0; i < 4; i++) {
      for (auto& h2 : mHistogramPreclustersXY[i]) {
        if (h2.second != nullptr) {
          h2.second->Write();
        }
      }
    }
    for (int i = 0; i < 2; i++) {
      for (auto& h2 : mHistogramPseudoeffXY[i]) {
        if (h2.second != nullptr) {
          h2.second->Write();
        }
      }
    }
    for (auto& h : mHistogramClchgDE) {
      if (h.second != nullptr) {
        h.second->Write();
      }
    }
    for (auto& h : mHistogramClchgDEOnCycle) {
      if (h.second != nullptr) {
        h.second->Write();
        h.second->Reset();
      }
    }
    for (auto& h : mHistogramClsizeDE) {
      if (h.second != nullptr) {
        h.second->Write();
        h.second->Reset();
      }
    }
  }

  mHistogramPseudoeff_NumsAndDens[0]->Write();
  mHistogramPseudoeff_NumsAndDens[1]->Write();

  mHistogramPseudoeff->Write();

  mMPVCycle->Write();

  f.Close();

#endif
}

void PhysicsTaskPreclusters::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Reseting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
