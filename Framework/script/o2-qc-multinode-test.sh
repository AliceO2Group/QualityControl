#!/usr/bin/env bash
#set -e
set -x
set -u
set -m
# Arguments or expected variables
# UNIQUE_PORT_1 and UNIQUE_PORT_2 must be set and not occupied by another process
# JSON_DIR must be set and point to the directory containing multinode-test.json.

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
trap cleanup EXIT

if [ -z "$UNIQUE_PORT_1" ]
then
  echo "UNIQUE_PORT_1 must be set when calling o2-qc-multinode-test.sh"
  exit 1
fi
export UNIQUE_TEST_NAME="multinode-test-${UNIQUE_PORT_1}"

function check_if_port_in_use() {
  OS=`uname`
  if [[ $OS == Linux ]] ; then
    PORT_PRESENT="$(netstat -tulpn 2>/dev/null | grep LISTEN | grep -w $1)"
  else #Darwin/BSD
    PORT_PRESENT="$(netstat -an -ptcp | grep LISTEN | grep -w $1)"
  fi

  if [[ ! -z "$PORT_PRESENT" ]]; then
    echo 'Port '$1' is in use, exiting.'
    echo 'If this port is always used in the build machines, ping the QC developers please'
    exit 1
  fi
}

function delete_data() {
  curl -i -L ccdb-test.cern.ch:8080/truncate/qc/TST/MO/MNLTest${UNIQUE_PORT_1}*
  curl -i -L ccdb-test.cern.ch:8080/truncate/qc/TST/MO/MNRTest${UNIQUE_PORT_2}*
  curl -i -L ccdb-test.cern.ch:8080/truncate/qc/TST/QO/MNLTest
  curl -i -L ccdb-test.cern.ch:8080/truncate/qc/TST/QO/MNRTest

  cd /tmp
  # mv in /tmp is guaranteed to be atomic
  mv -f /tmp/${UNIQUE_TEST_NAME}{,.todelete}
  rm -rf /tmp/${UNIQUE_TEST_NAME}.todelete
}

delete_data
# mkdir in /tmp is guaranteed to be atomic
mkdir /tmp/${UNIQUE_TEST_NAME} || { echo "Concurrent usage of the same port ${UNIQUE_PORT_1} detected, exiting"; exit 1; }
pushd /tmp/${UNIQUE_TEST_NAME}

UNIQUE_PORT_2=$((UNIQUE_PORT_1+1))

check_if_port_in_use $UNIQUE_PORT_1
check_if_port_in_use $UNIQUE_PORT_2
if [ -z "$JSON_DIR" ]
then
  echo "JSON_DIR must be set when calling o2-qc-multinode-test.sh"
  exit 1
fi

# make sure the CCDB is available otherwise we bail (no failure)
# we do not use ping because it will fail from outside CERN.
if curl --silent --connect-timeout 1 ccdb-test.cern.ch:8080 > /dev/null 2>&1 ; then
  echo "CCDB is reachable."
else
  echo "CCDB not reachable, multinode test is cancelled."
  exit 0
fi

# store data
timeout -s INT 40s o2-qc --config json://${JSON_DIR}/multinode-test.json -b --remote --run &
o2-qc-run-producer --producers 2 --message-amount 20  --message-rate 1 -b | timeout -s INT 35s o2-qc --config json://${JSON_DIR}/multinode-test.json -b --local --host localhost --run &


# wait until the local QC quits before moving forward.
wait

# check MonitorObject
# first the return code must be 200
code=$(curl -L ccdb-test.cern.ch:8080/qc/TST/MO/MNLTest${UNIQUE_PORT_1}/example/`date +%s`999 --write-out %{http_code} --silent --output /tmp/${UNIQUE_TEST_NAME}/multinode_test_obj${UNIQUE_PORT_1}.root)
if (( $code != 200 )); then
  echo "Error, monitor object of the local QC Task could not be found."
  delete_data
  exit 2
fi
# try to check that we got a valid root object
root -b -l -q -e 'TFile f("/tmp/${UNIQUE_TEST_NAME}/multinode_test_obj${UNIQUE_PORT_1}.root"); f.Print();'
if (( $? != 0 )); then
  echo "Error, monitor object of the local QC Task is invalid."
  delete_data
  exit 2
fi
# try if it is a non empty histogram
entries=`root -b -l -q -e 'TFile f("/tmp/${UNIQUE_TEST_NAME}/multinode_test_obj${UNIQUE_PORT_1}.root"); TH1F *h = (TH1F*)f.Get("ccdb_object"); cout << h->GetEntries() << endl;' | tail -n 1`
if ! [ $entries -gt 0 ] 2>/dev/null
then
  echo "The histogram of the local QC Task is empty or the object is not a histogram."
  delete_data
  exit 2
fi

# check MonitorObject
# first the return code must be 200
code=$(curl -L ccdb-test.cern.ch:8080/qc/TST/MO/MNRTest${UNIQUE_PORT_2}/example/`date +%s`999 --write-out %{http_code} --silent --output /tmp/${UNIQUE_TEST_NAME}/multinode_test_obj${UNIQUE_PORT_2}.root)
if (( $code != 200 )); then
  echo "Error, monitor object of the remote QC Task could not be found."
  delete_data
  exit 2
fi
# try to check that we got a valid root object
root -b -l -q -e 'TFile f("/tmp/${UNIQUE_TEST_NAME}/multinode_test_obj${UNIQUE_PORT_2}.root"); f.Print();'
if (( $? != 0 )); then
  echo "Error, monitor object of the remote QC Task is invalid."
  delete_data
  exit 2
fi
# try if it is a non empty histogram
entries=`root -b -l -q -e 'TFile f("/tmp/${UNIQUE_TEST_NAME}/multinode_test_obj${UNIQUE_PORT_2}.root"); TH1F *h = (TH1F*)f.Get("ccdb_object"); cout << h->GetEntries() << endl;' | tail -n 1`
if ! [ $entries -gt 0 ] 2>/dev/null
then
  echo "The histogram of the remote QC Task is empty or the object is not a histogram."
  delete_data
  exit 2
fi

delete_data
