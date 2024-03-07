# Muon Tracks QC

## TracksTask

The tracks task processes the forward muon tracks and produces a number of plots showing the kinematics and time distributions of the tracks.
The type of tracks that are processed is controlled via the `GID` list in the task configuration. The following options can be selected:
* `MCH`: all standalone MCH tracks are considered, independently of their matching with MFT and MID
* `MFT-MCH`: tracks matched from MFT to MCH, independently of their matching with MID
* `MCH-MID`: tracks matched from MCH to MID, independently of their matching with MFT
* `MFT-MCH-MID`: global forward tracks matched to all three detectors

For each selection listed in the `GID` parameter, two sets of similar plots are produced, one for all the tracks, the second only considering the tracks that pass some standard cuts, namely:
* `cutRAbsMin < RAbs < cutRAbsMax`
* `cutEtaMin < eta < cutEtaMax`
* `pT > cutPtMin`
* `sigmaPDCA < nSigmaPDCA`
* `chi2 < cutChi2Max`

All the cuts are computed using the standalone MCH tracks parameters, regardless of the matching, in order to have a consistent selection for all track types.

In addition to the single-track cuts listed above, a cut (`diMuonTimeCut`) on the time difference (in nanoseconds) between track pairs is applied when filling the di-muon invariant mass plot.

For each selection, the following histograms are filled:
* track kinematic parameters (pT, eta, phi) and their correlations
* distribution of the BC ids associated to the tracks
* tracks multiplicity per TF
* track quality parameters (chi2, RAbs, P*DCA, matching chi2 and probablity)
* 2-D distributions of the MCH track impact points at the MFT exit and MID entrance planes

The plots are saved in sub-folders inside `GLO/MUONTracks`, one folder for each `GID`. An additional sub-folder is aslo created for the plots after selection cuts.
In the example below, the folder structure would look like this:

```
GLO/MUONTracks/MFT-MCH/
GLO/MUONTracks/MFT-MCH/WithCuts/
GLO/MUONTracks/MCH-MID/
GLO/MUONTracks/MCH-MID/WithCuts/
GLO/MUONTracks/MFT-MCH-MID/
GLO/MUONTracks/MFT-MCH-MID/WithCuts/
```

#### Configuration
```json
{
  "qc": {
    "tasks": {
      "TaskMUONTracks": {
        "active": "true",
        "className": "o2::quality_control_modules::muon::TracksTask",
        "moduleName": "QcMUONCommon",
        "detectorName": "GLO",
        "taskName": "MUONTracks",
        "cycleDurationSeconds": "300",
        "maxNumberCycles": "-1",
        "dataSource": {
          "type": "direct",
          "query": "trackMCH:MCH/TRACKS;trackMCHROF:MCH/TRACKROFS;trackMCHTRACKCLUSTERS:MCH/TRACKCLUSTERS;mchtrackdigits:MCH/CLUSTERDIGITS;trackMFT:MFT/TRACKS;trackMFTROF:MFT/MFTTrackROF;trackMFTClIdx:MFT/TRACKCLSID;alpparMFT:MFT/ALPIDEPARAM;trackMID:MID/TRACKS;trackMIDROF:MID/TRACKROFS;trackMIDTRACKCLUSTERS:MID/TRACKCLUSTERS;trackClMIDROF:MID/TRCLUSROFS;matchMCHMID:GLO/MTC_MCHMID;fwdtracks:GLO/GLFWD"
        },
        "taskParameters": {
          "maxTracksPerTF": "600",
          "cutRAbsMin": "17.6",
          "cutRAbsMax": "89.5",
          "cutEtaMin": "-4.0",
          "cutEtaMax": "-2.5",
          "cutPtMin": "0.5",
          "cutPtMin": "0.5",
          "nSigmaPDCA": "6",
          "cutChi2Max": "1000",
          "diMuonTimeCut": "100",
          "fullHistos": "0",
          "GID" : "MFT-MCH,MCH-MID,MFT-MCH-MID"
        },
        "grpGeomRequest": {
          "geomRequest": "Aligned",
          "askGRPECS": "true",
          "askGRPLHCIF": "false",
          "askGRPMagField": "true",
          "askMatLUT": "false",
          "askTime": "false",
          "askOnceAllButField": "false",
          "needPropagatorD": "false"
        },
        "location": "remote"
      }
    }
  }
}
```

## Tracks Post-processing

The tracks post-processing task takes the plots produced by the tracks task and computes the matching efficiency from their ratios.
Each data source in the task configuration specifies the path of the plots from matched tracks(numerator), the path of the reference plots from un-matched tracks (denominator), and the path where to store the matching efficiency plots (ratios). The data source also contains a `names` parameter with the list of plots to be  compared. Each plot name can be followed by an optional number, separated by a semi-colon, that indicates the rebinning factor to be applied to the plots before computing the ratio. 

#### Configuration

```json
{
  "qc": {
    "postprocessing": {
      "MUONTracks": {
        "active": "true",
        "className": "o2::quality_control_modules::muon::TracksPostProcessing",
        "moduleName": "QcMUONCommon",
        "detectorName": "GLO",
        "dataSources": [
          {
            "plotsPath": "GLO/MO/MUONTracks/MCH-MID/WithCuts",
            "refsPath": "MCH/MO/Tracks/WithCuts",
            "outputPath": "MCH-MID/WithCuts/MatchEff",
            "names": [ "TrackPt:5", "TrackEta:5", "TrackPhi:10", "TrackEtaPt", "TrackPhiPt", "TrackPosAtMID" ]
          },
          {
            "plotsPath": "GLO/MO/MUONTracks/MCH-MID/WithCuts",
            "refsPath": "MCH/MO/Tracks/WithCuts",
            "outputPath": "MCH-MID/WithCuts/MatchEff",
            "names": [ "TrackPt:5", "TrackEta:5", "TrackPhi:10", "TrackEtaPt", "TrackPhiPt", "TrackPosAtMID" ]
          },
          {
            "plotsPath": "GLO/MO/MUONTracks/MFT-MCH-MID",
            "refsPath": "MCH/MO/Tracks",
            "outputPath": "MFT-MCH-MID/MatchEff-MCH",
            "names": [ "TrackPt:5", "TrackEta:5", "TrackPhi:10", "TrackEtaPt", "TrackPhiPt", "TrackPosAtMID", "TrackPosAtMFT" ]
          },
          {
            "plotsPath": "GLO/MO/MUONTracks/MFT-MCH-MID",
            "refsPath": "GLO/MO/MUONTracks/MCH-MID",
            "outputPath": "MFT-MCH-MID/MatchEff-MCH-MID",
            "names": [ "TrackPt:5", "TrackEta:5", "TrackPhi:10", "TrackEtaPt", "TrackPhiPt", "TrackPosAtMID", "TrackPosAtMFT" ]
          },
          {
            "plotsPath": "GLO/MO/MUONTracks/MFT-MCH-MID/WithCuts",
            "refsPath": "MCH/MO/Tracks/WithCuts",
            "outputPath": "MFT-MCH-MID/WithCuts/MatchEff-MCH",
            "names": [ "TrackPt:5", "TrackEta:5", "TrackPhi:10", "TrackEtaPt", "TrackPhiPt", "TrackPosAtMID", "TrackPosAtMFT" ]
          },
          {
            "plotsPath": "GLO/MO/MUONTracks/MFT-MCH-MID/WithCuts",
            "refsPath": "GLO/MO/MUONTracks/MCH-MID/WithCuts",
            "outputPath": "MFT-MCH-MID/WithCuts/MatchEff-MCH-MID",
            "names": [ "TrackPt:5", "TrackEta:5", "TrackPhi:10", "TrackEtaPt", "TrackPhiPt", "TrackPosAtMID", "TrackPosAtMFT" ]
          }
        ],
        "initTrigger": [
          "userorcontrol"
        ],
        "updateTrigger": [
          "newobject:qcdb:GLO/MO/MUONTracks/MCH-MID/TrackPt"
        ],
        "stopTrigger": [
          "userorcontrol"
        ]
      }
    }
  }
}
```

## Matching Efficiency Checker

The matching efficiency checker takes the output of the tracks post-processing and verifies that the efficiency values in each plot are within configurable limits. The limits are defined separately for each histogram, or group of histograms, whose name matches a given string.
For 1-D plots, the check can be additionally restricetd to one or more ranges in the x variable, in case some kinematical regions need to be excluded.

For example, the following line corresponds to an acceptable range, for the plots named `TrackEta`, of `[0.3,1.0]` and only checked within the eta ranges `[-4.0,-3.5]` and `[-3.0,-2.5]`:

```
"range:TrackEta": "0.3,1.0:-4.0,-3.5:-3.0,-2.5"
```

If no limit is specified for a given plot, the corresponding values are not checked and the plot is only beautified.

For 2-D plots, the beautification only consists in fixing the z-axis range to `[0.0,1.2]`.
For 1-D plots, the acceptable range is also indicated with horizontal lines, and the plot is painted in red if the quality is judged to be bad.

#### Configuration

```json
{
  "qc": {
    "checks": {
      "MUONMatchCheck": {
        "active": "true",
        "className": "o2::quality_control_modules::muon::MatchingEfficiencyCheck",
        "moduleName": "QcMUONCommon",
        "detectorName": "GLO",
        "policy": "OnAll",
        "extendedCheckParameters": {
          "default": {      
            "default": {      
              "range:TrackEta": "0.3,1.0:-4.0,-3.5:-3.0,-2.5",
              "range:TrackPt": "0.4,1.0:0.5,2.0",
              "range:WithCuts": "0.4,1.0"
            }
          }
        },
        "dataSource": [
          {
            "type": "PostProcessing",
            "name": "MUONTracks",
             "MOs" : [
               "MCH-MID/MatchEff/TrackPhi",
               "MCH-MID/MatchEff/TrackPt",
               "MCH-MID/MatchEff/TrackEta",
               "MCH-MID/WithCuts/MatchEff/TrackPhi",
               "MCH-MID/WithCuts/MatchEff/TrackPt",
               "MCH-MID/WithCuts/MatchEff/TrackEta",
               "MCH-MID/WithCuts/MatchEff/TrackPosAtMID"
            ]
          }
        ]
      }
    }
  }
}
```