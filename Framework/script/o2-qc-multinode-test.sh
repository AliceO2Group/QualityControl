#!/usr/bin/env bash
#set -e
set -x
set -u
set -m
# Arguments or expected variables
# UNIQUE_PORT_1 must be set; it is a candidate, and the script draws another
# pair if it turns out to be occupied. UNIQUE_PORT_2 is derived from it.
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
if [ -z "$JSON_DIR" ]
then
  echo "JSON_DIR must be set when calling o2-qc-multinode-test.sh"
  exit 1
fi

# UNIQUE_PORT_1 comes from string(RANDOM) at configure time, so it may be taken
# by the time the test runs -- and netstat can only look, not claim: binding is
# the only reliable check. Bind the pair for real, and draw a new even candidate
# if it is taken. Redraws come from BELOW the ephemeral range (32768+ on Linux,
# 49152+ on macOS), so the kernel's own outgoing connections cannot land on the
# port between our probe and FairMQ's bind.
function pick_port() {
  python3 - "$1" <<'EOF'
import random, socket, sys

def bindable(port):
    held = []
    try:
        for p in (port, port + 1):
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.bind(("", p))
            held.append(s)
        return True
    except OSError:
        return False
    finally:
        for s in held:
            s.close()

for port in [int(sys.argv[1])] + [random.randrange(20000, 30000, 2) for _ in range(50)]:
    if bindable(port):
        print(port)
        sys.exit(0)
sys.exit(1)
EOF
}

configured_port_1=$UNIQUE_PORT_1
UNIQUE_PORT_1=$(pick_port "$UNIQUE_PORT_1") || { echo "No free port pair found, exiting."; exit 1; }
if [ "$UNIQUE_PORT_1" != "$configured_port_1" ]; then
  echo "Port pair ${configured_port_1}/$((configured_port_1+1)) is in use, using ${UNIQUE_PORT_1}/$((UNIQUE_PORT_1+1))."
fi
UNIQUE_PORT_2=$((UNIQUE_PORT_1+1))
export UNIQUE_PORT_1 UNIQUE_PORT_2

# The port also names the QCDB objects (MNLTest$PORT) and the /tmp work area,
# keeping concurrent runs apart in the database as well as on the socket.
export UNIQUE_TEST_NAME="multinode-test-${UNIQUE_PORT_1}"

function delete_data() {
  curl -i -L ali-qcdb-test.cern.ch:8083/truncate/qc/TST/MO/MNLTest${UNIQUE_PORT_1}*
  curl -i -L ali-qcdb-test.cern.ch:8083/truncate/qc/TST/MO/MNRTest${UNIQUE_PORT_2}*
  curl -i -L ali-qcdb-test.cern.ch:8083/truncate/qc/TST/QO/MNLTest
  curl -i -L ali-qcdb-test.cern.ch:8083/truncate/qc/TST/QO/MNRTest

  cd /tmp
  # mv in /tmp is guaranteed to be atomic
  mv -f /tmp/${UNIQUE_TEST_NAME}{,.todelete}
  rm -rf /tmp/${UNIQUE_TEST_NAME}.todelete
}

delete_data
# mkdir in /tmp is guaranteed to be atomic
mkdir /tmp/${UNIQUE_TEST_NAME} || { echo "Concurrent usage of the same port ${UNIQUE_PORT_1} detected, exiting"; exit 1; }
pushd /tmp/${UNIQUE_TEST_NAME}

# The configure-time port is baked into multinode-test.json, in the bind ports
# and the task names. If we drew a different one, substitute it in a private
# copy; the file in JSON_DIR is shared by every run on this machine.
QC_CONFIG="${JSON_DIR}/multinode-test.json"
if [ "$UNIQUE_PORT_1" != "$configured_port_1" ]; then
  QC_CONFIG="/tmp/${UNIQUE_TEST_NAME}/multinode-test.json"
  python3 - "${JSON_DIR}/multinode-test.json" "$QC_CONFIG" "$configured_port_1" "$UNIQUE_PORT_1" <<'EOF' || { echo "Could not rewrite multinode-test.json."; exit 1; }
import re, sys
src, dst, old, new = sys.argv[1:5]
text = open(src).read()
for o, n in ((old, new), (str(int(old) + 1), str(int(new) + 1))):
    text = re.sub(r"(?<!\d)" + o + r"(?!\d)", n, text)
open(dst, "w").write(text)
EOF
fi

# make sure the CCDB is available otherwise we bail (no failure)
# we do not use ping because it will fail from outside CERN.
if curl --silent --connect-timeout 1 ali-qcdb-test.cern.ch:8083 > /dev/null 2>&1 ; then
  echo "CCDB is reachable."
else
  echo "CCDB not reachable, multinode test is cancelled."
  exit 0
fi

# store data
timeout -s INT 40s o2-qc --config json://${QC_CONFIG} -b --remote --run &
o2-qc-run-producer --producers 2 --message-amount 20  --message-rate 1 -b | timeout -s INT 35s o2-qc --config json://${QC_CONFIG} -b --local --host localhost --run &


# wait until the local QC quits before moving forward.
wait

# check MonitorObject
# first the return code must be 200
code=$(curl -L ali-qcdb-test.cern.ch:8083/qc/TST/MO/MNLTest${UNIQUE_PORT_1}/example/8000000 --write-out %{http_code} --silent --output /tmp/${UNIQUE_TEST_NAME}/multinode_test_obj${UNIQUE_PORT_1}.root)
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
code=$(curl -L ali-qcdb-test.cern.ch:8083/qc/TST/MO/MNRTest${UNIQUE_PORT_2}/example/8000000 --write-out %{http_code} --silent --output /tmp/${UNIQUE_TEST_NAME}/multinode_test_obj${UNIQUE_PORT_2}.root)
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
