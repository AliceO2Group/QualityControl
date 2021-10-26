# This is the documentation of the PHOS Quality Control

## PHOS QC contains 3 modules:
 - RawQcTask analyzes raw data converted to cells and HW errors

 - ClusterQCTask analyzes clusters

 - CalibQcTask analyzes modifications introduced by calibration

## Classification of HW decoding errors
  HW errors summary is shown in histo **ErrorTypePerDDL**. 
 - X axis shows FEE card number (0...13) of first branch, bin 15 means general SRU error or unknown FEC number, bin 16 - error n TRU. Bins 17..32 the same for second branch of same DDL. 
 - Y axis represents DDL 0...13
 - Bin content shows summary or errors: as there might be several kinds of errors in same FEC errors are paked as bits of the bin content and new error adds bit if it was not set before. The number of errors are shown in histogram NumberOfErrors with same X and Y axis.
 Bit meanings:
  - bit 0: not used
  - bit 1: not used
  - bit 2: not used
  - bit 3: incorrect hw address: wrong chip number
  - bit 4: error in mapping
  - bit 5: channel header error (header mark not found)
  - bit 6: channel payload error, incorrect payload size
  - bit 7: incorrect hw address

  - bins 15 or 30 (general SRU errors)
    - bit 2: wrong FEC number
  - bins 16 or 32 (general SRU errors)
      bit 1: wrong TRU header

  - non-existing DDL 14 corresponds to error in header
    - FEC = 0: Page was not found (page index outside range)
    - FEC = 1: Header cannot be decoded (format incorrect)
    - FEC = 2: Payload cannot be decoded (format incorrect)
    - FEC = 3: Header in memory not belonging to requested superpage
    - FEC = 4: Payload in memory not belonging to requested superpage
    - FEC = 16: wrong DDL number  

## Avaiable configurations 
```bash
# monitor pedestal run: HG/LG pedestals (mean and RMS) and HW decoding errors
02-raw-file-reader-workflow  --input-conf PHSraw.cfg | \
o2-phos-reco-workflow  --input-type raw --output-type cells --disable-root-output --pedestal on --keepHGLG on | \
o2-qc --config json://${QUALITYCONTROL_ROOT}/phosPedestal.json

# monitor raw data: cells and HW decoding errors
o2-raw-file-reader-workflow  --input-conf PHSraw.cfg | \
o2-phos-reco-workflow  --input-type raw --output-type cells --disable-root-output | \
o2-qc --config json://${QUALITYCONTROL_ROOT}/phosRaw.json

# monitor raw data: cells and HW decoding errors and clusters
o2-raw-file-reader-workflow  --input-conf PHSraw.cfg | \
o2-phos-reco-workflow  --input-type raw --output-type cells --disable-root-output | \
o2-phos-reco-workflow  --input-type cells --output-type clusters --disable-root-input --disable-root-output --disable-mc | \
o2-qc --config json://${QUALITYCONTROL_ROOT}/phosRawClu.json 

# monitor clusters only
o2-phos-reco-workflow  --input-type digits --output-type clusters  --disable-mc --disable-root-output | \
o2-qc --config json://${QUALITYCONTROL_ROOT}/phosClusters.json 
```

## Calibration QC
  quality of calibration is checked by comparing existing CCDB entry and new one. Module **CalibQcTask** shows difference between new and old calibrations. 
  Available configurations
```bash
# Pedestal calibration
o2-raw-file-reader-workflow  --input-conf PHSraw.cfg | \
o2-phos-reco-workflow  --input-type raw --output-type cells --disable-root-output --pedestal on --keepHGLG on | \
o2-phos-calib-workflow  --pedestals --forceupdate | \
o2-calibration-ccdb-populator-workflow | \
o2-qc --config json://${QUALITYCONTROL_ROOT}/phosCalibPed.json

# HG/LG ratio with LED runs
o2-raw-file-reader-workflow  --input-conf PHSraw.cfg | \
o2-phos-reco-workflow  --input-type raw --output-type cells --disable-root-output --keepHGLG on | \
o2-phos-calib-workflow  --hglgratio |\
o2-calibration-ccdb-populator-workflow | \
o2-qc --config json://${QUALITYCONTROL_ROOT}/phosCalibLED.json
```


