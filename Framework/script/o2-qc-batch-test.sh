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

  rm -f /tmp/batch_test_merged${UNIQUE_ID}.root
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
  exit 1
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

# Run the Tasks 3 times, merge results into the file.
o2-qc-run-producer --message-amount 100 --message-rate 100 | o2-qc --config json:/${JSON_DIR}/batch-test.json --local-batch /tmp/batch_test_merged${UNIQUE_ID}.root --run
o2-qc-run-producer --message-amount 100 --message-rate 100 | o2-qc --config json:/${JSON_DIR}/batch-test.json --local-batch /tmp/batch_test_merged${UNIQUE_ID}.root --run
# Run Checks and Aggregators, publish results to QCDB
o2-qc --config json:/${JSON_DIR}/batch-test.json --remote-batch /tmp/batch_test_merged${UNIQUE_ID}.root --run

# check MonitorObject
# first the return code must be 200
code=$(curl -L ccdb-test.cern.ch:8080/qc/TST/MO/BatchTestTask${UNIQUE_ID}/example/`date +%s`999 --write-out %{http_code} --silent --output /tmp/batch_test_obj${UNIQUE_ID}.root)
if (( $code != 200 )); then
  echo "Error, monitor object of the QC Task could not be found."
  delete_data
  exit 2
fi
# try to check that we got a valid root object
root -b -l -q -e 'TFile f("/tmp/batch_test_obj${UNIQUE_ID}.root"); f.Print();'
if (( $? != 0 )); then
  echo "Error, monitor object of the QC Task is invalid."
  delete_data
  exit 2
fi
# try if it is a non empty histogram
entries=`root -b -l -q -e 'TFile f("/tmp/batch_test_obj${UNIQUE_ID}.root"); TH1F *h = (TH1F*)f.Get("ccdb_object"); cout << h->GetEntries() << endl;' | tail -n 1`
if [ $entries -lt 150 ] 2>/dev/null
then
  echo "The histogram of the QC Task has less than 150 (75%) of expected samples."
  delete_data
  exit 2
fi

# check QualityObject
# first the return code must be 200
code=$(curl -L ccdb-test.cern.ch:8080/qc/TST/QO/BatchTestCheck${UNIQUE_ID}/`date +%s`999 --write-out %{http_code} --silent --output /tmp/batch_test_check${UNIQUE_ID}.root)
if (( $code != 200 )); then
  echo "Error, quality object of the QC Task could not be found."
  delete_data
  exit 2
fi

echo "Batch test was passed."
delete_data
