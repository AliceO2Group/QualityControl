void checkEff(const char *fn1="QC.root", const char *fn2="QC.root", long ts=1660430519000){
  // get TOF calibrations
  system(Form("o2-ccdb-downloadccdbfile -p TOF/Calib/ChannelCalib -t %ld",ts));

  TFile *fcal = new TFile("TOF/Calib/ChannelCalib/snapshot.root");
  o2::dataformats::CalibTimeSlewingParamTOF *cal = (o2::dataformats::CalibTimeSlewingParamTOF *) fcal->Get("ccdb_object");

  int rebin = 1;
  
  TFile *f1 = new TFile(fn1);
  TFile *f2 = new TFile(fn2);

  o2::quality_control::core::MonitorObjectCollection *mon = (o2::quality_control::core::MonitorObjectCollection *) f1->Get("TOF/Digits");
  o2::quality_control::core::MonitorObject *obj = (o2::quality_control::core::MonitorObject *) mon->FindObject("HitMapNoiseFiltered"); // TH2F
  TH2F *map = (TH2F *) obj->getObject();
  
  obj = (o2::quality_control::core::MonitorObject *) mon->FindObject("DecodingErrors"); // TH2I
  TH2I *decode = (TH2I *) obj->getObject();

  obj = (o2::quality_control::core::MonitorObject *) mon->FindObject("OrbitVsCrate"); // TH2I
  TProfile2D *orbit = (TProfile2D *) obj->getObject();

  obj = (o2::quality_control::core::MonitorObject *) mon->FindObject("EventCounter"); // TH2F
  TH2F *ec = (TH2F *) obj->getObject();

  int nCounts = 0;
  for(int ib=1; ib < 72; ib++){
    if(ec->GetBinContent(ib, 1) > nCounts){
      nCounts = ec->GetBinContent(ib, 1);
    }
  }
  
  TH1D *hpro = (TH1D *) orbit->ProfileY();
  int norb = 0;
  hpro->Divide(hpro);
  for(int ib=1; ib <= hpro->GetNbinsX(); ib++){
    if(hpro->GetBinContent(ib) == 0){
      break;
    }
    norb = ib;
  }

  
  printf("n orbits = %d - nCounts = %d\n",norb, nCounts);
  
  mon = (o2::quality_control::core::MonitorObjectCollection *) f2->Get("TOF/MatchTrAll");

  obj = (o2::quality_control::core::MonitorObject *) mon->FindObject("mHistoExpTrackedStrip"); // TH1F
  TH1F *trk = (TH1F *) obj->getObject();
  obj = (o2::quality_control::core::MonitorObject *) mon->FindObject("mHistoExpMatchedStrip"); // TH1F
  TH1F *mtch = (TH1F *) obj->getObject();
  obj = (o2::quality_control::core::MonitorObject *) mon->FindObject("mHistoExpMatchedStripFromCh"); // TH1F
  TH1F *mtchFromCh = (TH1F *) obj->getObject();

  TH1F *hRefMatching = mtch; // reference to strip for matching: using channel info or traking kinematics
  
  trk->Add(mtch, -1);
  trk->Add(hRefMatching, 1);
  
  TH2F *actv = new TH2F(*map);
  actv->SetName("hActiveMap");
  actv->Reset();

  TH2F *hEff2D = new TH2F(*map);
  hEff2D->SetName("hEff2D");
  hEff2D->Reset();
  hEff2D->SetStats(0);
  hEff2D->SetTitle("Eff 2D");
  
  float thr = map->Integral() / map->GetNbinsX() / map->GetNbinsX() * 0.05;
  for(int i=1; i<=map->GetNbinsX(); i++){
    for(int j=1; j<=map->GetNbinsY(); j++){
      if(map->GetBinContent(i,j) > thr){
	actv->SetBinContent(i,j,1);
      }
    }
  }
  
  TCanvas *cMap = new TCanvas;
  cMap->Divide(5,1);
  cMap->cd(1);
  actv->SetTitle("Active map");
  actv->Draw("col");
  cMap->cd(2);
  TH1F *hEffStrip = new TH1F(*hRefMatching);
  hEffStrip->SetName("hEffStrip");
  hEffStrip->SetTitle("hAcceptance");
  hEffStrip->GetYaxis()->SetTitle("#varepsilon");
  hEffStrip->SetStats(0);
  hEffStrip->Reset();
  TH1F *hEffDAQ = new TH1F(*hEffStrip);
  hEffDAQ->SetName("hEffDAQ");
  hEffDAQ->SetTitle("hEffDAQ");
  TH1F *hEffProb = new TH1F(*hEffStrip);
  hEffProb->SetName("hEffProb");
  hEffProb->SetTitle("hEffProblematicCh");
  hEffStrip->Draw();

  for(int i=1; i<=map->GetNbinsX(); i++){
    for(int j=1; j<=map->GetNbinsY(); j++){
      hEffStrip->AddBinContent(((i-1)/4)*91 + j, actv->GetBinContent(i,j)*0.25);
    }
  }
  for(int i=1; i<=map->GetNbinsX(); i++){
    for(int j=1; j<=map->GetNbinsY(); j++){
      hEffStrip->SetBinError(((i-1)/4)*91 + j, 0);
    }
  }
  hEffStrip->SetLineColor(2);
  
  cMap->cd(3);
  hEffDAQ->Draw();
  cMap->cd(4);
  hEffProb->Draw();
  
  TCanvas *cEff = new TCanvas;
  cEff->Divide(2,1);
  cEff->cd(1);
  TH1F *hEff = new TH1F(*hRefMatching);
  hEff->SetMarkerStyle(20);
  hEff->SetName("hEff");
  hEff->Divide(hRefMatching,trk,1,1,"B");
  hEff->GetYaxis()->SetTitle("#varepsilon");
  hEff->Draw("P");
  hEff->SetStats(0);
  
  TH1F *hEffN = new TH1F(*hEff);
  hEffN->SetName("hEffN");
  cEff->cd(2);
  hEffN->Draw("P");
  hEffN->GetYaxis()->SetTitle("normalized #varepsilon");

  new TCanvas;
  hEffN->Draw("P");
  
  for(int i=1; i < 18; i++){
    TLine *l = new TLine(i*91,0,i*91,1);
    l->SetLineColor(2);
    l->Draw("SAME");
  }
    
  for(int isec=0; isec < 18; isec++){
    for(int istrip=0; istrip<91;istrip++){
      int nact = 0;
      float nerr = 0;
      float goodorb = 0;
      float goodch = 0;
      for(int ich=0; ich<96; ich++){
	int channel = (91*isec + istrip)*96 + ich;
	int det[5];
	o2::tof::Geo::getVolumeIndices(channel, det);
	int isec = det[0] * 4 + det[4] / 12 + 1;
	if(actv->GetBinContent(isec, istrip+1)){
	  nact++;

	  int ech = o2::tof::Geo::getECHFromCH(channel);
	  int crate = o2::tof::Geo::getCrateFromECH(ech);
	  int trm = o2::tof::Geo::getTRMFromECH(ech);
	  float derr = 0;
	  if(decode->GetBinContent(crate+1,trm+1)){
	    if(decode->GetBinContent(crate+1, 1)){
	      int norm = decode->GetBinContent(crate+1, 1);
	      if(norm > nCounts){
		norm = nCounts;
	      }
	      derr = decode->GetBinContent(crate+1,trm+1)/norm;
	      nerr += derr;
	    }
	  }
	  float localorb = 0;
	  for(int iorb=1; iorb <= norb;iorb++){
	    localorb += orbit->GetBinContent(crate+1, iorb) / norb;
	    goodorb += orbit->GetBinContent(crate+1, iorb) / norb;
	  }
	  if(!cal->isProblematic(channel)){
	    goodch++;
	    hEff2D->SetBinContent(isec, istrip+1, hEff2D->GetBinContent(isec, istrip+1) + localorb*(1- derr)/24);
	  }
	}
      }
      if(nact > 0){
	hEffDAQ->SetBinContent(istrip+1+isec*91, goodorb/nact*(1 - nerr/nact));
	hEffProb->SetBinContent(istrip+1+isec*91, goodch/nact);
      } else {
	hEffDAQ->SetBinContent(istrip+1+isec*91, 1);
      }
      hEffDAQ->SetBinError(istrip+1+isec*91, 0);      
      hEffProb->SetBinError(istrip+1+isec*91, 0);      
    }
  }

  TH1F *hEffTot = new TH1F(*hEffStrip);
  hEffTot->SetName("hEffTot");
  hEffTot->SetTitle("hEffTot");

  hEffTot->Multiply(hEffProb);
  hEffTot->Multiply(hEffDAQ);

  hEffTot->RebinX(rebin);
  hEffTot->Scale(1./rebin);
  hEffDAQ->RebinX(rebin);
  hEffDAQ->Scale(1./rebin);
  hEffStrip->RebinX(rebin);
  hEffStrip->Scale(1./rebin);
  hEffProb->RebinX(rebin);
  hEffProb->Scale(1./rebin);
  hEff->RebinX(rebin);
  hEff->Scale(1./rebin);
  hEffN->RebinX(rebin);
  hEffN->Scale(1./rebin);

  hEffN->Divide(hEffTot);

  hEff->SetMaximum(1.0);
  hEffN->SetMaximum(1.0);

  hEffStrip->SetMaximum(1.0);
  hEffStrip->SetMinimum(0);
  hEffDAQ->SetMaximum(1.0);
  hEffDAQ->SetMinimum(0);
  hEffProb->SetMaximum(1.0);
  hEffProb->SetMinimum(0);
  hEffTot->SetMaximum(1.0);
  hEffTot->SetMinimum(0);

  cMap->cd(5);
  hEffTot->Draw();

  TCanvas *cDistr = new TCanvas;
  TH1F *hDistr = new TH1F("hDistr","#varepsilon distribution (2.7% PHOS HOLES); #varepsilon; fraction",101,0,1.01);
  hDistr->SetLineColor(2);
  TH1F *hDistrN = new TH1F("hDistrN","normalized #varepsilon distribution (2.7% PHOS HOLES); normalized #varepsilon; fraction",101,0,1.01);
  for(int i=1; i<=hEffN->GetNbinsX(); i++){
    hDistrN->Fill(hEffN->GetBinContent(i));
    hDistr->Fill(hEff->GetBinContent(i));
  }
  hDistrN->Draw();
  hDistr->Draw("SAME");
  hDistr->SetStats(0);
  hDistrN->Scale(1./91/18);
  hDistr->Scale(1./91/18);
  hDistrN->SetStats(1);

  TCanvas *cCheck = new TCanvas;
  cCheck->Divide(2,1);
  cCheck->cd(1);
  map->Draw("colz");
  cCheck->cd(2);
  hEff2D->Draw("colz");

}
