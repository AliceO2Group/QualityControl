#!/usr/bin/env bash
set -e
set -x
set -u
set -m
# Arguments or expected variables
# UNIQUE_ID must be set.
# JSON_DIR must be set and point to the directory containing batch-test.json.

# this is to make sure that we do not leave child processes behind
# https://unix.stackexchange.com/questions/240723/exit-trap-in-dash-vs-ksh-and-bash/240736#240736
cleanup() {
    # kill all processes whose parent is this process
    pkill -P $$
}
for sig in INT QUIT HUP TERM; do
  trap "
    cleanup
    trap - $sig EXIT
    kill -s $sig "'"$$"' "$sig"
done
#trap cleanup EXIT

function delete_data() {
  curl -i -L ccdb-test.cern.ch:8080/truncate/qc/TST/MO/BatchTestTask${UNIQUE_ID}*
  curl -i -L ccdb-test.cern.ch:8080/truncate/qc/TST/QO/BatchTestCheck${UNIQUE_ID}*

  rm -f /tmp/batch_test_mergedA${UNIQUE_ID}.root
  rm -f /tmp/batch_test_mergedB${UNIQUE_ID}.root
  rm -f /tmp/batch_test_mergedC${UNIQUE_ID}.root
  rm -f /tmp/batch_test_obj${UNIQUE_ID}.root
  rm -f /tmp/batch_test_check${UNIQUE_ID}.root
}

if [ -z "$UNIQUE_ID" ]
then
  echo "UNIQUE_ID must be set when calling o2-qc-batch-test.sh"
  exit 1
fi
if [ -z "$JSON_DIR" ]
then
  echo "JSON_DIR must be set when calling o2-qc-batch-test.sh"
  exit 2
fi

# make sure the CCDB is available otherwise we bail (no failure)
# we do not use ping because it will fail from outside CERN.
if curl --silent --connect-timeout 1 ccdb-test.cern.ch:8080 > /dev/null 2>&1 ; then
  echo "CCDB is reachable."
else
  echo "CCDB not reachable, batch test is cancelled."
  exit 0
fi

delete_data

export O2_DPL_DEPLOYMENT_MODE=Grid
# Run the Tasks 3 times, including twice with the same file.
o2-qc-run-producer --message-amount 100 --message-rate 100 | o2-qc --config json:/${JSON_DIR}/batch-test.json --local-batch /tmp/batch_test_mergedA${UNIQUE_ID}.root --run
o2-qc-run-producer --message-amount 100 --message-rate 100 | o2-qc --config json:/${JSON_DIR}/batch-test.json --local-batch /tmp/batch_test_mergedA${UNIQUE_ID}.root --run
o2-qc-run-producer --message-amount 100 --message-rate 100 | o2-qc --config json:/${JSON_DIR}/batch-test.json --local-batch /tmp/batch_test_mergedB${UNIQUE_ID}.root --run
# Run the file merger to produce the complete result
o2-qc-file-merger --input-files /tmp/batch_test_mergedA${UNIQUE_ID}.root /tmp/batch_test_mergedB${UNIQUE_ID}.root --output-file /tmp/batch_test_mergedC${UNIQUE_ID}.root
# Run Checks and Aggregators, publish results to QCDB
o2-qc --config json:/${JSON_DIR}/batch-test.json --remote-batch /tmp/batch_test_mergedC${UNIQUE_ID}.root --run

# check MonitorObject
# first the return code must be 200
code=$(curl -L ccdb-test.cern.ch:8080/qc/TST/MO/BatchTestTask${UNIQUE_ID}/example/`date +%s`999 --write-out %{http_code} --silent --output /tmp/batch_test_obj${UNIQUE_ID}.root)
if (( $code != 200 )); then
  echo "Error, monitor object of the QC Task could not be found."
  delete_data
  exit 3
fi
# try to check that we got a valid root object
root -b -l -q -e 'TFile f("/tmp/batch_test_obj${UNIQUE_ID}.root"); f.Print();'
if (( $? != 0 )); then
  echo "Error, monitor object of the QC Task is invalid."
  delete_data
  exit 4
fi
# try if it is a non empty histogram
entries=`root -b -l -q -e 'TFile f("/tmp/batch_test_obj${UNIQUE_ID}.root"); TH1F *h = (TH1F*)f.Get("ccdb_object"); cout << h->GetEntries() << endl;' | tail -n 1`
if [ $entries -lt 225 ] 2>/dev/null
then
  echo "The histogram of the QC Task has less than 225 (75%) of expected samples."
  delete_data
  exit 5
fi

# check QualityObject
# first the return code must be 200
code=$(curl -L ccdb-test.cern.ch:8080/qc/TST/QO/BatchTestCheck${UNIQUE_ID}/`date +%s`999 --write-out %{http_code} --silent --output /tmp/batch_test_check${UNIQUE_ID}.root)
if (( $code != 200 )); then
  echo "Error, quality object of the QC Task could not be found."
  delete_data
  exit 6
fi

echo "Batch test passed."
delete_data
