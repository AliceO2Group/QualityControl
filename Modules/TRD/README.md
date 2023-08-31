# Quality Control for the TRD


The QC code covers Tasks, Checks, Aggregators and Trending.
The configuration is done via json files. For running online at P2 we are using a single json file only:

    consul://o2/components/qc/ANY/any/trd-full-qcmn

For tests, both on staging and in production we can use another json file which we can select both for the QC merger node and for the EPN nodes:

    consul://o2/components/qc/ANY/any/trd-full-qcmn-test

For asynchronous production we are currently running basically the same thing as in synchronous, only without data sampling.

For both synch and asynch productions we urgently need

- tracking efficiency as function of pT and eta-phi
- trending of calibration parameters (gain, vDrift, ExB, t0)
- trending of basic observables, such as triggers per TF, tracklet and digit count per trigger, ...
- checks for the available plots, especially the ones foreseen for the shifters layout
- the chamber status from DCS in order to properly mask half chambers which are not expected to send data
- certainly others I cannot think of at the moment...


## Current Status

### Tasks

| class name | task name  | synch reco  | asynch reco  |
|---|---|---|---|
| RawDataTask  | RawData  | enabled  |  disabled |
| DigitsTask  |  Digits | enabled  | enabled  |
| TrackletsTask  |  Tracklets | enabled  | enabled |
| PulseHeightTrackMatch  | PHTrackMatch  | enabled, if ITSTPCTRD matching  | enabled, if ITSTPCTRD matching  |
| TrackingTask  |  Tracking | enabled, if ITSTPCTRD matching  | enabled, if ITSTPCTRD matching  |


In case new tasks are added please make sure that it uses a unique port number. The port range assigned to TRD can be found in <https://alice-flp.docs.cern.ch/Developers/KB/port_assignment/#subsystem-port-ranges-for-remote-connections-from-epns>. It goes from 29850 to 29899.

### Checks

TODO

### Aggregators

TODO

### Trending

TODO


## Quickstart

For local tests you can append the `o2-qc` binary to any workflow in O2 with the following parameters:

    o2-qc --config json://$HOME/alice/QualityControl/Modules/TRD/TRDQC.json

Adjust the TRDQC.json file according to your needs, e.g. add additional checks, disable some tasks depending on the inputs you have available etc.
