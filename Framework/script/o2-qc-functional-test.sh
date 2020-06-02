#!/usr/bin/env bash
set -e
set -x

# Arguments or expected variables
# UNIQUE_ID must be set.
# JSON_DIR must be set and point to the directory containing basic-functional.json.

if [ -z "$UNIQUE_ID" ]
then
  echo "UNIQUE_ID must be set when calling functional_test.sh"
  exit 1
fi
if [ -z "$JSON_DIR" ]
then
  echo "JSON_DIR must be set when calling functional_test.sh"
  exit 1
fi

# delete data
curl -i -L ccdb-test.cern.ch:8080/truncate/qc/TST/FctTest${UNIQUE_ID}*

# store data
DIR="$(dirname "${BASH_SOURCE[0]}")"  # get the directory name
DIR="$(realpath "${DIR}")"    # resolve its full path if need be
o2-qc-run-producer --message-amount 10 -b | o2-qc --config json://${JSON_DIR}/basic-functional.json -b --run

# check MonitorObject
# first the return code must be 200
code=$(curl -L ccdb-test.cern.ch:8080/qc/TST/FunctionalTest${UNIQUE_ID}/example/`date +%s`000 --write-out %{http_code} --silent --output /tmp/output${UNIQUE_ID}.root)
if (( $code != 200 )); then
  echo "Error, monitor object could not be found."
  exit 2
fi
# try to check that we got a valid root object
root -b -l -q -e 'TFile f("/tmp/output.root"); f.Print();'

# check QualityObject
# first the return code must be 200
code=$(curl -L ccdb-test.cern.ch:8080/qc/checks/TST/QcCheck${UNIQUE_ID}/`date +%s`000 --write-out %{http_code} --silent --output /tmp/output${UNIQUE_ID}.root)
if (( $code != 200 )); then
  echo "Error, data not found."
  exit 2
fi
# try to check that we got a valid root object
root -b -l -q -e 'TFile f("/tmp/output.root"); f.Print();'

# delete the data
curl -i -L ccdb-test.cern.ch:8080/truncate/qc/TST/FunctionalTest${UNIQUE_ID}*