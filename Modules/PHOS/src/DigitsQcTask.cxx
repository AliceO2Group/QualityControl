///
/// \file   DigitsQcTask.cxx
/// \author Dmitri Peresunko
/// \author Dmitry Blau
///

#include <TCanvas.h>
#include <TH2.h>

#include <DataFormatsPHOS/PHOSBlockHeader.h>
#include <DataFormatsPHOS/TriggerRecord.h>
#include <DataFormatsPHOS/Digit.h>
#include "QualityControl/QcInfoLogger.h"
#include "PHOS/DigitsQcTask.h"
#include "PHOSBase/Geometry.h"

namespace o2
{
namespace quality_control_modules
{
namespace phos
{

DigitsQcTask::~DigitsQcTask()
{
}

void DigitsQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize PHOS DigitsQcTask" << AliceO2::InfoLogger::InfoLogger::endm;

  //General
  for (int mod = 1; mod < nMod; mod++) {
    mBadMap[mod] = new TH2C(Form("BadMap%d", mod), Form("mod %d", mod), 64, 0., 64., 56, 0., 56.);
    mBadMap[mod]->SetStats(0);
    mBadMap[mod]->SetLineColor(kBlue - 8);
    mBadMap[mod]->GetXaxis()->SetNdivisions(508, kFALSE);
    mBadMap[mod]->GetYaxis()->SetNdivisions(514, kFALSE);
    mBadMap[mod]->GetXaxis()->SetTitle("x, cells");
    mBadMap[mod]->GetYaxis()->SetTitle("z, cells");
    try {
      getObjectsManager()->startPublishing(mBadMap[mod]);
    } catch (boost::exception& e) {
      getObjectsManager()->stopPublishing(Form("BadMap%d", mod));
      getObjectsManager()->startPublishing(mBadMap[mod]);
    }
  }

  publishPhysicsObjects();
  //  publishPedestalObjects() ;
  //  publishLEDObjects() ;

  // initialize geometry
  if (!mGeometry)
    mGeometry = o2::phos::Geometry::GetInstance();

  getBadMap();
}

void DigitsQcTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  reset();
}

void DigitsQcTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void DigitsQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  //  QcInfoLogger::GetInstance() << "HOSDigitQC: Start monitor data" << AliceO2::InfoLogger::InfoLogger::endm;

  // check if we have payoad
  auto dataref = ctx.inputs().get("phos-digits");
  auto const* phosheader = o2::framework::DataRefUtils::getHeader<o2::phos::PHOSBlockHeader*>(dataref);
  if (!phosheader->mHasPayload) {
    QcInfoLogger::GetInstance() << "No more digits" << AliceO2::InfoLogger::InfoLogger::endm;
    //ctx.services().get<o2::framework::ControlService>().readyToQuit(false);
    return;
  }

  // Get payload and loop over digits
  auto digitcontainer = ctx.inputs().get<gsl::span<o2::phos::Digit>>("phos-digits");
  auto triggerrecords = ctx.inputs().get<gsl::span<o2::phos::TriggerRecord>>("phos-triggerecords");

  //  QcInfoLogger::GetInstance() << "Received " << digitcontainer.size() << " digits " << AliceO2::InfoLogger::InfoLogger::endm;
  int eventcouter = 0;
  for (auto trg : triggerrecords) {
    if (!trg.getNumberOfObjects())
      continue;
    QcInfoLogger::GetInstance() << QcInfoLogger::Debug << "Next event " << eventcouter << " has " << trg.getNumberOfObjects() << " digits" << QcInfoLogger::endm;
    gsl::span<const o2::phos::Digit> eventdigits(digitcontainer.data() + trg.getFirstEntry(), trg.getNumberOfObjects());
    processPhysicsEvent(eventdigits);
    //OR Pedestal or LED event
    eventcouter++;
  }
}

void DigitsQcTask::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void DigitsQcTask::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void DigitsQcTask::reset()
{
  // clean all the monitor objects here

  QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
  //TODO reset
}
//___________________________________________
void DigitsQcTask::processPhysicsEvent(gsl::span<const o2::phos::Digit> event)
{

  //fill few histograms
  const Double_t cutCell = 0.03; //Minimal energy to fill cell map
  // const Double_t cutCellTrig = 0.5;
  // const Double_t cutEtime = 0.5;
  // const Int_t kbdrL =-1 ; // trigger patch wrt clusters
  // const Int_t kbdrR = 3 ; //

  float modTotCellEnergy[nMod] = { 0 };
  int modNcell[nMod] = { 0 };

  for (auto digit : event) {
    float e = digit.getAmplitude();
    // Converts the absolute numbering into the following array
    //  relid[0] = PHOS Module number 1:module
    //  relid[1] = Row number inside a PHOS module (Phi coordinate)
    //  relid[2] = Column number inside a PHOS module (Z coordinate)
    char relid[3];
    mGeometry->absToRelNumbering(digit.getAbsId(), relid);
    int mod = relid[0];
    int ix = relid[1] + 1; //NB!
    int iz = relid[2] + 1; //NB!

    if (e > cutCell) {
      float ecellMean = mCellEmean2D[mod]->GetBinContent(ix, iz);
      float ncellmean = mCellN2D[mod]->GetBinContent(ix, iz);
      mCellN2D[mod]->Fill(ix - 0.5, iz - 0.5);
      mCellEmean2D[mod]->SetBinContent(ix, iz, (ecellMean * ncellmean + e) / (ncellmean + 1.));
      modNcell[mod]++;
      modTotCellEnergy[mod] += e;
      mCellSp[mod]->Fill(e, mAcceptanceCorrection[mod]);

      // if (e > cutCellTrig) {
      //   int flag2=0;
      //   for(int ixTr= TMath::Max(0,ix-kbdrR);  ixTr < TMath::Min(64,ix-kbdrL); ixTr++){
      //     for(int izTr= TMath::Max(0,iz-kbdrR);  izTr < TMath::Min(56,iz-kbdrL); izTr++){
      //       if(mTrig[mod][ixTr][izTr]){ //there is fired trigger
      //         flag2=1;
      //         mTrig[mod][ixTr][izTr]=kFALSE;
      //         mTRGoccupancy[mod]->Fill(ixTr+0.5,izTr+0.5,1.) ;
      //       }
      //     }
      //   }
      //   if(flag2){
      //     mCellSpTrig[mod]->Fill(e, mAcceptanceCorrection[mod]);
      // }
      // else{
      //   mCellSpFake[mod]->Fill(e, fAcceptanceCorrection[mod]);
    }
  }

  // if(mTrigL1[mod][ix][iz][0]){ //for low threshold
  //   mL1occupancy[mod]->Fill(ix+0.5,iz+0.5,1.) ;
  // }
  // if(mPHOSL0Trig){
  //   mL1occupancy[mod]->Fill(ix+0.5,iz+0.5,1.) ;
  // }

  // mTRGsignal[mod]->Fill(ix+0.5,iz+0.5,fTrigSig2x2[mod][ix][iz]);

  // if(mPHOSL1TrigLo){
  //   mL1signal[mod]->Fill(ix+0.5,iz+0.5,fTrigSig2x2[mod][ix][iz]);
  // }

  for (int mod = 1; mod < nMod; mod++) {
    if (modNcell[mod] > 0) {
      modTotCellEnergy[mod] /= modNcell[mod];
    }
    mCellMeanEnergy[mod]->Fill(modTotCellEnergy[mod]);
    mCellN[mod]->Fill(modNcell[mod] * mAcceptanceCorrection[mod]);
  }

  // //Mark fake triggers: trigger without cluster
  // for(Int_t ix=1; ix<=64;ix++){
  //   for(Int_t iz=1; iz<=56; iz++){
  //     if(mTrig[mod][ix][iz]){ //there is fired trigger without cluster
  //           mFakeTrig[mod]->Fill(ix+0.5,iz+0.5,1.) ;
  //     }
  //   }
  // }
}
//___________________________________________
void DigitsQcTask::publishPhysicsObjects()
{
  //Prepare objects for physics runs

  for (int mod = 1; mod < nMod; mod++) {
    // if(!mTRGoccupancy[mod] || !getObjectsManager()->getObject(((moTRGoccupancy[mod]->GetName()).c_str())) {
    //   mTRGoccupancy[mod] = new TH2F(Form("TRGoccup%d",mod), Form("Number of fired triggers per channel in mod %d",mod), 64, 0., 64., 56, 0., 56.);
    // }
    // else{
    //   mTRGoccupancy[mod]->Reset();
    // }
    // if(!mTRGsignal[mod] || !getObjectsManager()->getObject(((mTRGsignal[mod]->GetName()).c_str())) {
    //   mTRGsignal[mod] = new TH2F(Form("TRGsignal%d",mod), Form("Sum of amplitudes of fired triggers per channel in mod %d",mod), 64, 0., 64., 56, 0., 56.);
    // }
    // else{
    //   mTRGsignal[mod]->Reset();
    // }

    // if(!mL1occupancy[mod] || !getObjectsManager()->getObject(((moL1occupancy[mod]->GetName()).c_str())) {
    //   mL1occupancy[mod]=new TH2F(Form("L1occup%d",mod), Form("Number of fired L1 Lo threshold triggers per channel in mod %d",mod), 64, 0., 64., 56, 0., 56.);
    // }
    // else{
    //   mL1occupancy[mod]->Reset();
    // }
    // if(!mFakeTrig[mod] || !getObjectsManager()->getObject(((moFakeTrig[mod]->GetName()).c_str())) {
    //   mFakeTrig[mod]=new TH2F(Form("TRGfake%d",mod), Form("Number of fake triggers fired in channel in mod %d",mod), 64, 0., 64., 56, 0., 56.);
    // }
    // else{
    //   mFakeTrig[mod]->Reset();
    // }
    if (!mTimeE[mod]) {
      mTimeE[mod] = new TH2F(Form("TimeM%d", mod), Form("Cell time vs cell energy in module %d", mod), 100, 0., 20., 200, -300.e-9, 300.e-9);
      mTimeE[mod]->GetXaxis()->SetTitle("E_{digit} (GeV)");
      mTimeE[mod]->GetYaxis()->SetTitle("#tau_{digit} (s)");
      try {
        getObjectsManager()->startPublishing(mTimeE[mod]);
      } catch (boost::exception& e) {
        getObjectsManager()->stopPublishing(Form("TimeM%d", mod));
        getObjectsManager()->startPublishing(mTimeE[mod]);
      }
    } else {
      mTimeE[mod]->Reset();
    }

    if (!mCellN[mod]) {
      mCellN[mod] = new TH1F(Form("CellMeanNM%d", mod), Form("Average number of cells in module %d", mod), 1000, 0., 1000.);
      mCellN[mod]->GetXaxis()->SetTitle("N_{cell}/event");
      mCellN[mod]->GetYaxis()->SetTitle("dN_{events}/dN_{cell}");
      try {
        getObjectsManager()->startPublishing(mCellN[mod]);
      } catch (boost::exception& e) {
        getObjectsManager()->stopPublishing(Form("CellMeanNM%d", mod));
        getObjectsManager()->startPublishing(mCellN[mod]);
      }
    } else {
      mCellN[mod]->Reset();
    }
    if (!mCellMeanEnergy[mod]) {
      mCellMeanEnergy[mod] = new TH1F(Form("CellMeanEnM%d", mod), Form("Average cells energy, mod %d", mod), 100, 0., 10.);
      mCellMeanEnergy[mod]->GetXaxis()->SetTitle("<E_{cell}> (GeV)");
      mCellMeanEnergy[mod]->GetYaxis()->SetTitle("dN_{events}/d<E_{cell}>");
      try {
        getObjectsManager()->startPublishing(mCellMeanEnergy[mod]);
      } catch (boost::exception& e) {
        getObjectsManager()->stopPublishing(Form("CellMeanEnM%d", mod));
        getObjectsManager()->startPublishing(mCellMeanEnergy[mod]);
      }
    } else {
      mCellMeanEnergy[mod]->Reset();
    }
    if (!mCellEmean2D[mod]) {
      mCellEmean2D[mod] = new TH2F(Form("CellE2D%d", mod), Form("Cell total energy, mod %d", mod), 64, 0., 64., 56, 0., 56.);
      mCellEmean2D[mod]->GetXaxis()->SetTitle("Cell_{#phi}");
      mCellEmean2D[mod]->GetXaxis()->SetTitle("Cell_{z}");
      mCellEmean2D[mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mCellEmean2D[mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      try {
        getObjectsManager()->startPublishing(mCellEmean2D[mod]);
      } catch (boost::exception& e) {
        getObjectsManager()->stopPublishing(Form("CellE2D%d", mod));
        getObjectsManager()->startPublishing(mCellEmean2D[mod]);
      }
    } else {
      mCellEmean2D[mod]->Reset();
    }

    if (!mCellN2D[mod]) {
      mCellN2D[mod] = new TH2F(Form("CellN2D%d", mod), Form("Cell multiplicity, mod %d", mod), 64, 0., 64., 56, 0., 56.);
      mCellN2D[mod]->GetXaxis()->SetTitle("Cell_{#phi}");
      mCellN2D[mod]->GetXaxis()->SetTitle("Cell_{z}");
      mCellN2D[mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mCellN2D[mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      try {
        getObjectsManager()->startPublishing(mCellN2D[mod]);
      } catch (boost::exception& e) {
        getObjectsManager()->stopPublishing(Form("CellN2D%d", mod));
        getObjectsManager()->startPublishing(mCellN2D[mod]);
      }
    } else {
      mCellN2D[mod]->Reset();
    }

    // if(!mCellN2Dlow[mod] ||  !getObjectsManager()->getObject(Form("CellN2Dlow%d", mod))) {
    //   mCellN2Dlow[mod] = new TH2F(Form("CellN2Dlow%d", mod), Form("Cell multiplicity, mod %d", mod), 64, 0., 64., 56, 0., 56.);
    //   mCellN2Dlow[mod]->GetXaxis()->SetTitle("Cell_{#phi}");
    //   mCellN2Dlow[mod]->GetXaxis()->SetTitle("Cell_{z}");
    //   mCellN2Dlow[mod]->GetXaxis()->SetNdivisions(508,kFALSE);
    //   mCellN2Dlow[mod]->GetYaxis()->SetNdivisions(514,kFALSE);
    //   getObjectsManager()->startPublishing(mCellN2Dlow[mod]) ;
    // }
    // else{
    //   mCellN2D[mod]->Reset();
    // }
    // if(!mCellN2Dhigh[mod] ||  !getObjectsManager()->getObject(Form("CellN2D%d", mod))) {
    //   mCellN2Dhigh[mod] = new TH2F(Form("CellN2D%d", mod), Form("Cell multiplicity, mod %d", mod), 64, 0., 64., 56, 0., 56.);
    //   mCellN2Dhigh[mod]->GetXaxis()->SetTitle("Cell_{#phi}");
    //   mCellN2Dhigh[mod]->GetXaxis()->SetTitle("Cell_{z}");
    //   mCellN2Dhigh[mod]->GetXaxis()->SetNdivisions(508,kFALSE);
    //   mCellN2Dhigh[mod]->GetYaxis()->SetNdivisions(514,kFALSE);
    //   getObjectsManager()->startPublishing(mCellN2Dhigh[mod]) ;
    // }
    // else{
    //   mCellN2Dhigh[mod]->Reset();
    // }
    if (!mCellSp[mod]) {
      mCellSp[mod] = new TH1F(Form("CellSpectrM%d", mod), Form("Cell spectrum, mod %d", mod), 199, 0.01, 20.00);
      mCellSp[mod]->GetXaxis()->SetTitle("E_{cell} (GeV)");
      mCellSp[mod]->GetYaxis()->SetTitle("dN/dE_{cell}");
      try {
        getObjectsManager()->startPublishing(mCellSp[mod]);
      } catch (boost::exception& e) {
        getObjectsManager()->stopPublishing(Form("CellSpectrM%d", mod));
        getObjectsManager()->startPublishing(mCellSp[mod]);
      }
    } else {
      mCellSp[mod]->Reset();
    }
    // if(!mCellSpTrig[mod] ||  !getObjectsManager()->getObject(((moCellSpTrig[mod]->GetName()).c_str())) {
    //   mCellSpTrig[mod]= new TH1F(Form("CellSpectrumTrigM%d", mod), Form("Cell trig spectrum, mod %d", mod), 199, 0.01, 20.00);
    //   mCellSpTrig[mod]->GetXaxis()->SetTitle("E_{cell} (GeV)");
    //   mCellSpTrig[mod]->GetYaxis()->SetTitle("dN/dE_{cell}");
    // }
    // else{
    //   mCellSpTrig[mod]->Reset();
    // }
    // if(!mCellSpFake[mod] ||  !getObjectsManager()->getObject(((moCellSpFake[mod]->GetName()).c_str())) {
    //   mCellSpFake[mod] = new TH1F(Form("CellSpectrumFakeM%d", mod), Form("Cell faketrig spectrum, mod %d", mod), 199, 0.01, 20.00);
    //   mCellSpFake[mod]->GetXaxis()->SetTitle("E_{cell} (GeV)");
    //   mCellSpFake[mod]->GetYaxis()->SetTitle("dN/dE_{cell}");
    // }
    // else{
    //   mCellSpFake[mod]->Reset();
    // }
  }

  // //Lines to draw limits in histograms
  // TLine line(0, 0, 1, 1);
  // line.SetLineColor(kBlue);
  // line.SetLineWidth(2);
  // line.SetLineStyle(5);

  // for (Int_t mod = 1; mod < 2; mod++) {
  //   moCellMeanEnergy[mod]->GetListOfFunctions()->Add(line.DrawLine(fCellEmeanMin, 0., fCellEmeanMin, 1.)->Clone());
  //   moCellMeanEnergy[mod]->GetListOfFunctions()->Add(line.DrawLine(fCellEmeanMax, 0., fCellEmeanMax, 1.)->Clone());
  //   moCellN[mod]->GetListOfFunctions()->Add(line.DrawLine(fCellMultiplicityMin, 0., fCellMultiplicityMin, 1.)->Clone());
  //   moCellN[mod]->GetListOfFunctions()->Add(line.DrawLine(fCellMultiplicityMax, 0., fCellMultiplicityMax, 1.)->Clone());
  //   moCellSp[mod]->GetListOfFunctions()->Add(text.Clone());

  //   moCluMeanEnergy[mod]->GetListOfFunctions()->Add(line.DrawLine(fCluEmeanMin, 0., fCluEmeanMin, 1.)->Clone());
  //   moCluMeanEnergy[mod]->GetListOfFunctions()->Add(line.DrawLine(fCluEmeanMax, 0., fCluEmeanMax, 1.)->Clone());
  //   moCluN[mod]->GetListOfFunctions()->Add(line.DrawLine(fCluMultiplicityMin, 0., fCluMultiplicityMin, 1.)->Clone());
  //   moCluN[mod]->GetListOfFunctions()->Add(line.DrawLine(fCluMultiplicityMax, 0., fCluMultiplicityMax, 1.)->Clone());
  // }
}
//___________________________________________
void DigitsQcTask::publishPedestalObjects()
{
}
//___________________________________________
void DigitsQcTask::publishLEDObjects()
{
}
//___________________________________________
void DigitsQcTask::getBadMap()
{

  // Set a default storage if it is not done yet and if your QA need it
  // AliCDBManager* man = AliCDBManager::Instance();
  // if (!man->IsDefaultStorageSet()) {
  //   man->SetDefaultStorage(gSystem->Getenv("AMORE_CDB_URI"));
  // }
  // // Set a default run if it is not set yet and you need it
  // man->SetRun(fRun);

  // // Load last OCDB object
  // AliCDBEntry *entryCalib = man->Get("PHOS/Calib/EmcGainPedestals");
  // if (entryCalib)
  //   fCalibData = (AliPHOSEmcCalibData*) entryCalib->GetObject();
  // AliCDBEntry *entryBadMap = man->Get("PHOS/Calib/EmcBadChannels");
  // AliPHOSEmcBadChannelsMap * badMapData = 0x0;
  // if (entryBadMap) {
  //   badMapData = (AliPHOSEmcBadChannelsMap*) entryBadMap->GetObject();
  // }

  for (int mod = 1; mod < nMod; mod++) {
    mAcceptanceCorrection[mod] = 0.;
    // for (int ix = 1; ix <= 64; ix++) {
    //   for (int iz = 1; iz <= 56; iz++) {
    //     if (badMapData->IsBadChannel(mod, iz, ix)) {
    //       mBadMap[mod]->SetBinContent(ix, iz, 1.5);
    //       mAcceptanceCorrection[mod] += 1.;
    //     }
    //   }
    // }

    if (mAcceptanceCorrection[mod] < 56 * 64)
      mAcceptanceCorrection[mod] = 64 * 56 / (64 * 56 - mAcceptanceCorrection[mod]);
    else
      mAcceptanceCorrection[mod] = 0; //module does not exist
  }
}

} // namespace phos
} // namespace quality_control_modules
} // namespace o2
