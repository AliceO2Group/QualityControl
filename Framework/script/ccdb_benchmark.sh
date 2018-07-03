#!/usr/bin/env bash
set -e ;# exit on error
set -u ;# exit when using undeclared variable
#set -x ;# debugging

### Notes
# FOR THE TIME BEING IT RUNS EVERYTHING LOCALLY
# One must have ssh keys to connect to all hosts.

### Define matrix of tests
NB_OF_TASKS=(20) ;#1 2 5 10 25 50 100);
NB_OF_OBJECTS=(20);# 10 100 1000);
#SIZE_OBJECTS=(1 10 100 500 1000 2500 5000);# 10 100 1000);# in kB
SIZE_OBJECTS=(2500 5000);# 10 100 1000);# in kB

### Misc variables
# The log prefix will be followed by the benchmark description, e.g. 1 task 1 checker... or an id or both
LOG_FILE_PREFIX=/tmp/logRepositoryBenchmark_
NUMBER_CYCLES=30 ;# ec per cycle -> # seconds
PAUSE_BTW_RUNS=120 ;# in seconds, pause between tests
DB_URL="ccdb-test.cern.ch:8080" ;#"aido2qc43:8080" ;#
DB_USERNAME=""
DB_PASSWORD=""
DB_NAME=""
DB_BACKEND="CCDB"
COMMAND_PREFIX="cd alice ; unset http_proxy ; unset https_proxy ; alienv setenv --no-refresh QualityControl/latest -c "
MONITORING_URL="influxdb-udp://aido2mon.cern.ch:8087" ;#"influxdb-udp://aido2mon-gpn.cern.ch:8087"
NODES=(
"ccdb@aido2qc10"
"ccdb@aido2qc40"
"ccdb@aido2qc11"
"ccdb@aido2qc12"
"ccdb@aido2fe05"
#"ccdb@localhost"
#"ccdb@barth-ccdb-606a6b90-1d83-48d5-8e46-ca72a63fc586"a
#"ccdb@barth-ccdb-6f859d93-034c-4151-a4c8-571be0fe90f5"
#"ccdb@barth-ccdb-c518e99c-c05c-4066-a5dd-63613755985f"
#"ccdb@barth-ccdb-1394cef1-a627-4cf0-944f-40bf63f62ce1"
#"ccdb@barth-ccdb-a53241f0-f9f5-4862-8bca-66c081f7bd14"
) ;# space delimited


### Utility functions
# Start a task in the background
# \param 1 : host
# \param 2 : name of the task
# \param 3 : log file suffix
# \param 4 : size of the objects
# \param 5 : number of objects
# \param 6 : number of tasks
function startTask {
  host=$1
  name=$2
  log_file_suffix=$3
  size_objects=$4
  number_objects=$5
  number_tasks=$6
  log_file_name=${LOG_FILE_PREFIX}${log_file_suffix}.log
  echo "Starting task ${name} on host ${host}, logs in ${log_file_name}"
    cmd="${COMMAND_PREFIX} repositoryBenchmark --max-iterations ${NUMBER_CYCLES} \
        --id ${name} --mq-config ~/alice/QualityControl/Framework/alfa.json --number-tasks ${number_tasks}\
        --delete 0 --control static --size-objects ${size_objects} --number-objects ${number_objects} \
        --monitoring-url ${MONITORING_URL} --task-name ${name} \
        --database-backend ${DB_BACKEND:-\"\"} \
        --database-name ${DB_NAME:-\"\"} \
        --database-username ${DB_USERNAME:-\"\"} \
        --database-password ${DB_PASSWORD:-\"\"} \
        --database-url ${DB_URL:-\"\"} \
        --monitoring-threaded 0 > ${log_file_name} 2>&1 "
  echo "ssh ${host} \"${cmd}\" &"
  ssh ${host} "${cmd}" &
  pidLastTask=$!
}

# Kill all processes named "name"
# \param 1 : name
# \param 2 : host
# \param 3 : extra flag for killall
function killAll {
    name=$1
    host=$2
    extra=${3:-""}
    echo "Killing all processes called $name on $host"
    ssh ${host} "killall ${extra} ${name}> /dev/null 2>&1 || true" ;# ignore errors
}

# Delete the database content
# \param 1 : number of tasks
function cleanDatabase {
  number_tasks=$1
  for (( task=0; task<$nb_tasks; task++ )); do
    name=benchmarkTask_${task}
    cmd="repositoryBenchmark --id test --mq-config ~/dev/alice/QualityControl/Framework/alfa.json --delete 1 --control static \
         --task-name ${name}" ;# > /dev/null 2>&1"
    echo ${cmd}
    eval ${cmd}
  done
}


### Benchmark starts here
# Loop through the matrix of tests
for nb_tasks in ${NB_OF_TASKS[@]}; do
  for nb_objects in ${NB_OF_OBJECTS[@]}; do
    for size_objects in ${SIZE_OBJECTS[@]}; do
      echo "*************************** $(date)
      Launching test for $nb_tasks tasks, $nb_objects objects, $size_objects kB objects"

      echo "Kill all old processes"
      for machine in ${NODES[@]}; do
        killAll "repositoryBenchmark" ${machine} "-9"
      done

#      echo "Delete database content"
#      cleanDatabase $nb_tasks

      echo "Now start the tasks"
      for (( task=0; task<$nb_tasks; task++ )); do
        echo "*** ~~~ *** Start task"

        # select a node for this task : task % #node -> index of node
        node_index=$((task%${#NODES[@]}))

        startTask "${NODES[$node_index]}" benchmarkTask_${task} \
                  "benchmarkTask_${task}_${nb_tasks}_${nb_objects}_${size_objects}" \
                  ${size_objects} ${nb_objects} ${nb_tasks}
        TASKS_PIDS+=($pidLastTask)
      done

      echo "Now wait for the tasks to finish "
      wait ${TASKS_PIDS[*]}

      sleep 5 # leave time to finish

      for machine in ${NODES[@]}; do
        killAll "repositoryBenchmark" ${machine} "-9"
      done

      sleep ${PAUSE_BTW_RUNS} # leave time to finish

      TASKS_PIDS=()

      echo "OK, ready for the next round !"

    done
  done
done

