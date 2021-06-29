# This is the documentation of the TOF QC

## List of available JSON configurations
`tofdigits.json`
- Monitoring TOF digits

`tofdigits_multinode.json`
- Monitoring TOF digits with the remote machine configuration

`toffull.json`
- Monitoring both TOF RAW data and TOF digits

`toffull_multinode.json`
- Monitoring both TOF RAW data and TOF digits with the remote machine configuration 

`tofpostprocessdiagnosticpercrate.json`
- Postprocessing the diagnostic words from the cards to the crate view 

`tofraw.json`
- Monitoring both TOF RAW data

`tofraw_multinode.json`
- Monitoring both TOF RAW data with the remote machine configuration 


## Useful information
Here are some various tips to run QC

### Multinode QC i.e. running QC with separate data merging
The multinode QC configuration is useful when running QC on EPN/FLP/DCS machines i.e. when several machines are observing different fractions of the TOF data.
- The local machines are machines that run the QC directly on data usuall there are more than one.
- The remote machine is responsible for the merging and checking of the data
- On the QCDB the merged output will be under the name of the LOCAL tasks!
- QC on local machines should have specified the name of the machine.
- It's very important to specify `--local --host localnode1` for local machines and `--remote` for remote machine. 
`localnode1` is the alias of the machine defined inside the configuration JSON.

An example is the following:
```bash
# First local QC process
o2-raw-file-reader-workflow -b --input-conf TOFraw.cfg --loop 1000 | \
    o2-tof-compressor -b --shm-segment-size 10000000000 | \
    o2-tof-reco-workflow -b --input-type raw --output-type digits | \
    o2-qc -b --config json://${$QUALITYCONTROL_ROOT}/toffull_multinode.json --local --host localnode1 &

# Second local QC process
o2-raw-file-reader-workflow -b --input-conf TOFraw.cfg --loop 1000 | \
    o2-tof-compressor -b --shm-segment-size 10000000000 | \
    o2-tof-reco-workflow -b --input-type raw --output-type digits | \
    o2-qc -b --config json://${$QUALITYCONTROL_ROOT}/toffull_multinode.json --local --host localnode2 &

# Third local QC process
o2-raw-file-reader-workflow -b --input-conf TOFraw.cfg --loop 1000 | \
    o2-tof-compressor -b --shm-segment-size 10000000000 | \
    o2-tof-reco-workflow -b --input-type raw --output-type digits | \
    o2-qc -b --config json://${$QUALITYCONTROL_ROOT}/toffull_multinode.json --local --host localnode3 &

# Remote QC process
o2-qc -b --config json://${$QUALITYCONTROL_ROOT}/toffull_multinode.json --remote
```

## Test
