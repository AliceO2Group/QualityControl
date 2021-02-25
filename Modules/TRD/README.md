
Quality Control for the TRD
===========================

This README should evolve into documentation on how to run, interpret and
contribute to the QC for the TRD.

Current Status
==============

At the point of writing (Feb 2021) a single QC task is available that reads
digits and produces and ADC histogram. The task can be run offline and pushes
the histogram into the QC database at CERN.

Quickstart
==========

In order to run a TRD QC task, you will need to build the [O2] and
[QualityControl] packages. The GitHub landing pages have good instructions,
and I will not repeat them here.

[O2]: https://github.com/AliceO2Group/AliceO2/
[QualityControl]: https://github.com/AliceO2Group/QualityControl

You will need a `trddigits.root` file. You can either generate one with
`o2-sim`, followed by running the digitizer:
```
o2-sim -n ${NEVENTS}
o2-sim-digitizer-workflow --onlyDet TRD
```
Jorge has written a [simulation cheat-sheet] in the O2 repository with some
more hints on running the simulation. Alternatively, it is possible to convert
a run 2 raw file to run 3 `trddigits.root` format.

[simulation cheat-sheet]: https://github.com/AliceO2Group/AliceO2/tree/dev/Detectors/TRD/simulation

We need a DPL device to provide digits to the task, and `o2-trd-trap-sim` does
the trick. A dedicated digit reader was discussed and should be trivial to
implement, but is not available as of now. The actual QC needs to be run with
a configuration file, which is available in QC repository.
```
o2-trd-trap-sim | o2-qc --config json://${QC_DIR}/Modules/TRD/DigitsTask.json
```
