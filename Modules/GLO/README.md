# GLO
Documents the available task in the module and their parameters.
## ITS-TPC Matching Task
### Parameters
#### MC
 - `isMC=false` produce additional MC plots
 - `useTrkPID=false` propagate tracks with their pid hypothesis
#### ITS
 - `minPtITSCut=0.1f` minimum ITS track momentum
 - `etaITSCut=1.4f` maximum ITS track eta
 - `minNITSClustersCut=0` minimum number of ITS clusters
 - `maxChi2PerClusterITS=1e10f` maximum chi2/ITS cluster
#### TPC
 - `minPtTPCCut=0.1f` minimum TPC track momentum
 - `etaITSCut=1.4f` maximum ITS track eta
 - `minTPCClustersCut=60` minimum number of TPC clusters
 - `minDCACut=100.f` minimum TPC track DCA Z
 - `minDCACutY=10.f` minimum TPC track DCA Y
#### ITS-TPC
 - `minPtCut=0.1f` minimum ITS-TPC momentum
 - `maxPtCut=20.f` maximum ITS-TPC momentum
 - `etaCut=1.4f` maximum ITS-TPC track eta
#### Additional sync parameters
 - `isSync=false` synchronous processing, needed for all following parameters
#### MTC ratios
 - `doMTCRatios=false` produce MTC ratio plots
#### K0s
 - `doK0QC=false` produce K0s plots
 - `maxK0Eta=0.8f` maximum K0s eta
 - `refitK0=false` refit K0 prongs
 - `cutK0Mass=0.05f` cut around K0s peak
 - `trackSourcesK0` SVertexer input sources
 - `publishK0s3D=false` publish 3D cycle,integral histograms
 - `splitK0sMassOccupancy=float` splitting point in TPC occupancy to define low and high region by default off
 - `splitK0sMassPt=float` splitting point in pt to define low and high region by default off
 + K0Fitter options
#### ITS-PV
 - `doPVITSQC=false` produce ITS vs PV plots (not implemented yet)

## ITS-TPC Matching Check
### Parameters
#### Pt
 - `showPt=false` show check on MTC pt
 - `thresholdPt=0.5f` threshold on MTC pt
 - `minPt=1.0f` check range left
 - `maxPt=1.999f` check range right
#### Phi
 - `showPhi=false` show check on MTC phi
 - `thresholdPhi=0.3f` threshold on MTC phi
#### Eta
 - `showEta=false` show check on MTC eta
 - `thresholdEta=0.4f` threshold on MTC eta
 - `minEta=-0.8f` check range left
 - `maxEta=0.8f` check range right
#### K0
 - `showK0s=false` show check on K0s mass
 - `acceptableK0sRError=0.2f` acceptable relative error to pdg value
 - `acceptableK0sUncertainty=0.2f` acceptable uncertainty to pdg value
 + K0Fitter options
#### Other
 - `limitRanges=5` maximum number of bad intervals shown

## K0sFitReductor
Trends mean and sigma of fit.
### Output
 - `mean` aggregated mass value
 - `sigma` aggregated sigma value

## MTCReductor
Trends MTC at given pt.
### Output
 - `mtc` mtc efficiency
### Parameters
 - `pt` take value at this pt

## PVITSReductor
Trends constant + slope of straight line fit in given range.
### Output
 - `pol0` constant
 - `pol1` slope
### Parameters
 - `r0` start fit range
 - `r1` end fit range

## K0Fitter
Does a `pol2 + Gaus` fit.
### Parameters
 - `k0sBackgroundRejLeft=0.48` reject region in background fit from left side of mass peak
 - `k0sBackgroundRejRight=0.51` reject region in background fit to right side of mass peak
 - `k0sBackgroundRangeLeft=0.45` absolute left range to fit background
 - `k0sBackgroundRangeRight=0.54` absolute right range to fit background
