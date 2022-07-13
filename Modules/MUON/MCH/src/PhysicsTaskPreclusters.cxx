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

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{
PhysicsTaskPreclusters::PhysicsTaskPreclusters() : TaskInterface()
{
  mIsSignalDigit = o2::mch::createDigitFilter(20, true, true);
}

PhysicsTaskPreclusters::~PhysicsTaskPreclusters() {}

void PhysicsTaskPreclusters::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize PhysicsTaskPreclusters" << AliceO2::InfoLogger::InfoLogger::endm;

  mDiagnostic = false;
  if (auto param = mCustomParameters.find("Diagnostic"); param != mCustomParameters.end()) {
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mDiagnostic = true;
    }
  }

  mElec2DetMapper = createElec2DetMapper<ElectronicMapperGenerated>();
  mDet2ElecMapper = createDet2ElecMapper<ElectronicMapperGenerated>();
  mFeeLink2SolarMapper = createFeeLink2SolarMapper<ElectronicMapperGenerated>();
  mSolar2FeeLinkMapper = createSolar2FeeLinkMapper<ElectronicMapperGenerated>();

  const uint32_t nElecXbins = PhysicsTaskPreclusters::sMaxFeeId * PhysicsTaskPreclusters::sMaxLinkId * PhysicsTaskPreclusters::sMaxDsId;

  // Histograms in electronics coordinates
  mHistogramPseudoeffElec = std::make_shared<MergeableTH2Ratio>("Pseudoeff_Elec", "Pseudoeff", nElecXbins, 0, nElecXbins, 64, 0, 64, true);
  publishObject(mHistogramPseudoeffElec, "colz", false, false);

  // 1D histograms for mean pseudoeff per DE (integrated or per elapsed cycle) - Used in trending
  mHistogramMeanPseudoeffPerDE[0] = std::make_shared<TH1F>("MeanPseudoeffPerDE_B", "Mean Pseudoeff for each DE (B)", getDEindexMax(), 0, getDEindexMax());
  publishObject(mHistogramMeanPseudoeffPerDE[0], "E", false, false);
  mHistogramMeanPseudoeffPerDE[1] = std::make_shared<TH1F>("MeanPseudoeffPerDE_NB", "Mean Pseudoeff for each DE (NB)", getDEindexMax(), 0, getDEindexMax());
  publishObject(mHistogramMeanPseudoeffPerDE[1], "E", false, false);

  mHistogramPreclustersPerDE = std::make_shared<MergeableTH1Ratio>("PreclustersPerDE", "Number of pre-clusters for each DE", getDEindexMax(), 0, getDEindexMax());
  publishObject(mHistogramPreclustersPerDE, "hist", false, false);
  mHistogramPreclustersSignalPerDE = std::make_shared<MergeableTH1Ratio>("PreclustersSignalPerDE", "Number of pre-clusters (with signal) for each DE", getDEindexMax(), 0, getDEindexMax());
  publishObject(mHistogramPreclustersSignalPerDE, "hist", false, false);

  // Histograms in global detector coordinates
  mHistogramPseudoeffST12 = std::make_shared<MergeableTH2Ratio>("Pseudoeff_ST12", "ST12 Pseudoeff", 10, 0, 10, 10, 0, 10, true);
  publishObject(mHistogramPseudoeffST12, "colz", false, false);

  mHistogramNumST12 = std::make_shared<GlobalHistogram>("Num_ST12", "Number of hits (ST12)", 0, 5, mHistogramPseudoeffST12->getNum());
  mHistogramNumST12->init();
  mHistogramDenST12 = std::make_shared<GlobalHistogram>("Den_ST12", "Number of orbits (ST12)", 0, 5, mHistogramPseudoeffST12->getDen());
  mHistogramDenST12->init();
  mAllHistograms.push_back(mHistogramDenST12->getHist());

  mHistogramPseudoeffST345 = std::make_shared<MergeableTH2Ratio>("Pseudoeff_ST345", "ST345 Pseudoeff", 10, 0, 10, 10, 0, 10, true);
  publishObject(mHistogramPseudoeffST345, "colz", false, false);

  mHistogramNumST345 = std::make_shared<GlobalHistogram>("Num_ST345", "Number of hits (ST345)", 1, 10, mHistogramPseudoeffST345->getNum());
  mHistogramNumST345->init();
  mAllHistograms.push_back(mHistogramNumST345->getHist());
  mHistogramDenST345 = std::make_shared<GlobalHistogram>("Den_ST345", "Number of orbits (ST345)", 1, 10, mHistogramPseudoeffST345->getDen());
  mHistogramDenST345->init();
  mAllHistograms.push_back(mHistogramDenST345->getHist());

  for (auto de : o2::mch::raw::deIdsForAllMCH) {

    auto h = std::make_shared<TH1F>(TString::Format("Expert/%sCluster_Charge_%03d", getHistoPath(de).c_str(), de),
                                    TString::Format("Cluster charge (DE%03d)", de), 1000, 0, 50000);
    mHistogramClchgDE.insert(make_pair(de, h));
    publishObject(h, "hist", false, false);

    auto h1 = std::make_shared<TH1F>(TString::Format("Expert/%sCluster_Charge_OnCycle_DE%03d", getHistoPath(de).c_str(), de),
                                     TString::Format("Cluster charge on cycle (DE%03d)", de), 1000, 0, 50000);
    mHistogramClchgDEOnCycle.insert(make_pair(de, h1));

    h = std::make_shared<TH1F>(TString::Format("Expert/%sCluster_Size_%03d", getHistoPath(de).c_str(), de),
                               TString::Format("Cluster size (DE%03d)", de), 10, 0, 10);
    mHistogramClsizeDE.insert(make_pair(de, h));
    publishObject(h, "hist", false, false);

    h = std::make_shared<TH1F>(TString::Format("Expert/%sCluster_Size_B_%03d", getHistoPath(de).c_str(), de),
                               TString::Format("Cluster size (DE%03d B)", de), 10, 0, 10);
    mHistogramClsizeDE_B.insert(make_pair(de, h));
    publishObject(h, "hist", false, false);

    h = std::make_shared<TH1F>(TString::Format("Expert/%sCluster_Size_NB_%03d", getHistoPath(de).c_str(), de),
                               TString::Format("Cluster size (DE%03d NB)", de), 10, 0, 10);
    mHistogramClsizeDE_NB.insert(make_pair(de, h));
    publishObject(h, "hist", false, false);

    // Histograms using the XY Mapping
    {
      // Bending side
      auto hmB = std::make_shared<MergeableTH2Ratio>(TString::Format("Expert/%sPseudoeff_B_XY_%03d", getHistoPath(de).c_str(), de),
                                                     TString::Format("Pseudo-efficiency XY (DE%03d B)", de), true);
      mHistogramPseudoeffXY[0].insert(make_pair(de, hmB));
      publishObject(hmB, "colz", false, true);

      auto hNumB = std::make_shared<DetectorHistogram>(TString::Format("Expert/%sPreclusters_num_B_XY_%03d", getHistoPath(de).c_str(), de),
                                                       TString::Format("Preclusters XY (DE%03d B, num)", de), de, 0,
                                                       hmB->getNum());
      mHistogramPreclustersXY[0].insert(make_pair(de, hNumB));
      mAllHistograms.push_back(hNumB.get()->getHist());

      auto hDenB = std::make_shared<DetectorHistogram>(TString::Format("Expert/%sPreclusters_den_B_XY_%03d", getHistoPath(de).c_str(), de),
                                                       TString::Format("Preclusters XY (DE%03d B, den)", de), de, 0,
                                                       hmB->getDen());
      mHistogramPreclustersXY[1].insert(make_pair(de, hDenB));
      mAllHistograms.push_back(hDenB.get()->getHist());

      // Non-bending side
      auto hmNB = std::make_shared<MergeableTH2Ratio>(TString::Format("Expert/%sPseudoeff_NB_XY_%03d", getHistoPath(de).c_str(), de),
                                                      TString::Format("Pseudo-efficiency XY (DE%03d NB)", de), true);
      mHistogramPseudoeffXY[1].insert(make_pair(de, hmNB));
      publishObject(hmNB, "colz", false, true);

      auto hNumNB = std::make_shared<DetectorHistogram>(TString::Format("Expert/%sPreclusters_num_NB_XY_%03d", getHistoPath(de).c_str(), de),
                                                        TString::Format("Preclusters XY (DE%03d NB, num)", de), de, 0,
                                                        hmNB->getNum());
      mHistogramPreclustersXY[2].insert(make_pair(de, hNumNB));
      mAllHistograms.push_back(hNumNB.get()->getHist());

      auto hDenNB = std::make_shared<DetectorHistogram>(TString::Format("Expert/%sPreclusters_den_NB_XY_%03d", getHistoPath(de).c_str(), de),
                                                        TString::Format("Preclusters XY (DE%03d NB, den)", de), de, 0,
                                                        hmNB->getDen());
      mHistogramPreclustersXY[3].insert(make_pair(de, hDenNB));
      mAllHistograms.push_back(hDenNB.get()->getHist());
    }
  }
}

void PhysicsTaskPreclusters::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PhysicsTaskPreclusters::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

static void updateTFcount(TH1D* hDen)
{
  for (int bin = 1; bin <= hDen->GetXaxis()->GetNbins(); bin++) {
    auto x = hDen->GetXaxis()->GetBinCenter(bin);
    hDen->Fill(x);
  }
}

void PhysicsTaskPreclusters::monitorData(o2::framework::ProcessingContext& ctx)
{
  bool verbose = false;
  // get the input preclusters and associated digits
  auto preClusters = ctx.inputs().get<gsl::span<o2::mch::PreCluster>>("preclusters");
  auto digits = ctx.inputs().get<gsl::span<o2::mch::Digit>>("preclusterdigits");

  ILOG(Info, Support) << fmt::format("Received {} pre-clusters and {} digits", preClusters.size(), digits.size()) << AliceO2::InfoLogger::InfoLogger::endm;

  updateTFcount(mHistogramPreclustersPerDE->getDen());
  updateTFcount(mHistogramPreclustersSignalPerDE->getDen());

  bool print = true;
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
bool PhysicsTaskPreclusters::getFecChannel(int deId, int padId, int& fecId, int& channel)
{
  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(deId);

  channel = segment.padDualSampaChannel(padId);

  int dsId = segment.padDualSampaId(padId);

  // Using the mapping to go from Digit info (de, pad) to Elec info (fee, link) and fill Elec Histogram,
  // where one bin is one physical pad
  // get the unique solar ID and the DS address associated to this digit
  std::optional<DsElecId> dsElecId = mDet2ElecMapper(DsDetId{ deId, dsId });
  if (!dsElecId) {
    return false;
  }
  uint32_t dsAddr = dsElecId->elinkId();
  uint32_t solarId = dsElecId->solarId();

  std::optional<FeeLinkId> feeLinkId = mSolar2FeeLinkMapper(solarId);
  if (!feeLinkId) {
    return false;
  }
  uint32_t feeId = feeLinkId->feeId();
  uint32_t linkId = feeLinkId->linkId();

  fecId = feeId * PhysicsTaskPreclusters::sMaxLinkId * PhysicsTaskPreclusters::sMaxDsId + (linkId % PhysicsTaskPreclusters::sMaxLinkId) * PhysicsTaskPreclusters::sMaxDsId + dsAddr;

  return true;
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
  // whether the pre-cluster contains at least one signal-like digit in each cathode
  float hasSignal[2] = { false, false };
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

    if (mIsSignalDigit(digit)) {
      hasSignal[cid] = true;
    }
  }

  mHistogramPreclustersPerDE->getNum()->Fill(getDEindex(detid));
  if (hasSignal[0] || hasSignal[1]) {
    mHistogramPreclustersSignalPerDE->getNum()->Fill(getDEindex(detid));
  }

  // compute center-of-gravity of the charge distribution
  double Xcog, Ycog;
  bool isWide[2];
  CoG(preClusterDigits, Xcog, Ycog, isWide);

  int padIdB = -1;
  int padIdNB = -1;
  int fecIdB = -1;
  int channelB = -1;
  int fecIdNB = -1;
  int channelNB = -1;
  if (segment.findPadPairByPosition(Xcog, Ycog, padIdB, padIdNB)) {
    getFecChannel(detid, padIdB, fecIdB, channelB);
    getFecChannel(detid, padIdNB, fecIdNB, channelNB);
  }

  // criteria to define a "good" charge cluster in one cathode:
  bool isGoodDen[2] = { hasSignal[1] && isWide[1], hasSignal[0] && isWide[0] };
  bool isGoodNum[2] = { cathode[0] && isWide[0], cathode[1] };

  // Filling histograms to be used for Pseudo-efficiency computation
  if (isGoodDen[0]) {
    // good cluster on non-bending side, check if there is data from the bending side as well
    if (fecIdB >= 0 && channelB >= 0) {
      mHistogramPseudoeffElec->getDen()->Fill(fecIdB, channelB);
    }
    auto hXY0 = mHistogramPreclustersXY[1].find(detid);
    if ((hXY0 != mHistogramPreclustersXY[1].end()) && (hXY0->second != NULL)) {
      hXY0->second->Fill(Xcog, Ycog, 0.5, 0.5);
    }
    if (isGoodNum[0]) { // Check if associated to something on Bending
      if (fecIdB >= 0 && channelB >= 0) {
        mHistogramPseudoeffElec->getNum()->Fill(fecIdB, channelB);
      }
      auto hXY1 = mHistogramPreclustersXY[0].find(detid);
      if ((hXY1 != mHistogramPreclustersXY[0].end()) && (hXY1->second != NULL)) {
        hXY1->second->Fill(Xcog, Ycog, 0.5, 0.5);
      }
    }
  }

  if (isGoodDen[1]) {
    // good cluster on bending side, check if there is data from the non-bending side as well
    if (fecIdNB >= 0 && channelNB >= 0) {
      mHistogramPseudoeffElec->getDen()->Fill(fecIdNB, channelNB);
    }
    auto hXY0 = mHistogramPreclustersXY[3].find(detid);
    if ((hXY0 != mHistogramPreclustersXY[3].end()) && (hXY0->second != NULL)) {
      hXY0->second->Fill(Xcog, Ycog, 0.5, 0.5);
    }
    if (isGoodNum[1]) { // Check if associated to something on Non-Bending
      if (fecIdNB >= 0 && channelNB >= 0) {
        mHistogramPseudoeffElec->getNum()->Fill(fecIdNB, channelNB);
      }
      auto hXY1 = mHistogramPreclustersXY[2].find(detid);
      if ((hXY1 != mHistogramPreclustersXY[2].end()) && (hXY1->second != NULL)) {
        hXY1->second->Fill(Xcog, Ycog, 0.5, 0.5);
      }
    }
  }

  // This code is slow. Will be re-introduced after some optimizations
  // const o2::mch::mapping::CathodeSegmentation& csegment = segment.bending();
  // o2::mch::contour::Contour<double> envelop = o2::mch::mapping::getEnvelop(csegment);
  // std::vector<o2::mch::contour::Vertex<double>> vertices = envelop.getVertices();
  // o2::mch::contour::BBox<double> bbox = o2::mch::mapping::getBBox(csegment);

  // skip a fiducial border around the active area, so that only fully-contained clusters are considered
  // if(Xcog < (bbox.xmin() + 5)) return true;
  // if(Xcog > (bbox.xmax() - 5)) return true;
  // if(Ycog < (bbox.ymin() + 5)) return true;
  // if(Ycog > (bbox.ymax() - 5)) return true;

  if (hasSignal[0] || hasSignal[1]) {
    // cluster size, separately on each cathode and combined
    auto hSize = mHistogramClsizeDE.find(detid);
    if ((hSize != mHistogramClsizeDE.end()) && (hSize->second != NULL)) {
      hSize->second->Fill(multiplicity[0] + multiplicity[1]);
    }
    hSize = mHistogramClsizeDE_B.find(detid);
    if ((hSize != mHistogramClsizeDE_B.end()) && (hSize->second != NULL)) {
      hSize->second->Fill(multiplicity[0]);
    }
    hSize = mHistogramClsizeDE_NB.find(detid);
    if ((hSize != mHistogramClsizeDE_NB.end()) && (hSize->second != NULL)) {
      hSize->second->Fill(multiplicity[1]);
    }

    // total cluster charge
    float chargeTot = chargeSum[0] + chargeSum[1];
    auto hCharge = mHistogramClchgDE.find(detid);
    if ((hCharge != mHistogramClchgDE.end()) && (hCharge->second != NULL)) {
      hCharge->second->Fill(chargeTot);
    }

    auto hChargeOnCycle = mHistogramClchgDEOnCycle.find(detid);
    if ((hChargeOnCycle != mHistogramClchgDEOnCycle.end()) && (hChargeOnCycle->second != NULL)) {
      hChargeOnCycle->second->Fill(chargeTot);
    }
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
  // update mergeable ratios
  mHistogramPseudoeffElec->update();

  for (auto de : o2::mch::raw::deIdsForAllMCH) {
    for (int i = 0; i < 2; i++) {
      auto h = mHistogramPseudoeffXY[i].find(de);
      if ((h != mHistogramPseudoeffXY[i].end()) && (h->second != nullptr)) {
        h->second->update();
      }
    }
  }

  mHistogramNumST12->set(mHistogramPreclustersXY[0], mHistogramPreclustersXY[2]);
  mHistogramDenST12->set(mHistogramPreclustersXY[1], mHistogramPreclustersXY[3]);

  mHistogramNumST345->set(mHistogramPreclustersXY[0], mHistogramPreclustersXY[2]);
  mHistogramDenST345->set(mHistogramPreclustersXY[1], mHistogramPreclustersXY[3]);

  mHistogramPseudoeffST12->update();
  mHistogramPseudoeffST345->update();

  mHistogramPreclustersPerDE->update();
  mHistogramPreclustersSignalPerDE->update();
}

void PhysicsTaskPreclusters::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;

  computePseudoEfficiency();
}

void PhysicsTaskPreclusters::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;

  computePseudoEfficiency();
}

void PhysicsTaskPreclusters::reset()
{
  // clean all the monitor objects here
  ILOG(Info, Support) << "Reseting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;

  for (auto h : mAllHistograms) {
    h->Reset();
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
