#!/usr/bin/env bash
set -e ;# exit on error
set -u ;# exit when using undeclared variable
#set -x ;# debugging

### Notes
# FOR THE TIME BEING IT RUNS EVERYTHING LOCALLY
# One must have ssh keys to connect to all hosts.

### Define matrix of tests
NB_OF_TASKS=(1);# 5 10 25 50);
NB_OF_OBJECTS=(1);# 10 100 1000);
SIZE_OBJECTS=(1);# 10 100 1000);# in kB

### Misc variables
# The log prefix will be followed by the benchmark description, e.g. 1 task 1 checker... or an id or both
LOG_FILE_PREFIX=/tmp/logCcdbBenchmark_
NUMBER_CYCLES=10 ;# 180 ;# 1 sec per cycle -> ~ 3 minutes
USER=benchmarkCCDB
TASKS_FULL_ADDRESSES="\
pcald02a.cern.ch,\
" ;# comma delimited, no space


### Utility functions
# Start a task in the background
# \param 1 : host
# \param 2 : name of the task
# \param 3 : log file suffix
# \param 4 : size of the objects
# \param 5 : number of objects
function startTask {
  host=$1
  name=$2
  log_file_suffix=$3
  size_objects=$4
  number_objects=$5
  log_file_name=${LOG_FILE_PREFIX}${log_file_suffix}.log
  echo "Starting task ${name} on host ${host}, logs in ${log_file_name}"
  cmd="ccdbBenchmark --max-iterations ${NUMBER_CYCLES} --id ${name} --mq-config ~/dev/alice/QualityControl/Framework/alfa.json \
        --delete 0 --control static --size-objects ${size_objects} --number-objects ${number_objects} \
        --task-name ${name} > ${log_file_name} 2>&1 &"
  echo ${cmd}
  eval ${cmd}
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
    ssh ${USER}@${host} "killall ${extra} ${name}  > /dev/null 2>&1" &
}

# Delete the database content
# \param 1 : number
function cleanDatabase {
  name=$1
  cmd="ccdbBenchmark --id test --mq-config ~/dev/alice/QualityControl/Framework/alfa.json --delete 1 --control static \
       --task-name ${name} > /dev/null 2>&1"
  echo ${cmd}
  eval ${cmd}
}


### Benchmark starts here
# Loop through the matrix of tests
for nb_tasks in ${NB_OF_TASKS[@]}; do
  for nb_objects in ${NB_OF_OBJECTS[@]}; do
    for size_objects in ${SIZE_OBJECTS[@]}; do
      echo "*************************** $(date)
      Launching test for $nb_tasks tasks, $nb_objects objects, $size_objects kB objects"

      echo "Kill all old processes"
      #for (( task=0; task<$nb_tasks; task++ )); do
        killall -9 ccdbBenchmark || true ;# ignore errors
#        killAll "ccdbBenchmark" ${NODES_TASKS[${task}]} "-9"
      #done

      echo "Delete database content"
      cleanDatabase $nb_tasks

      echo "Now start the tasks"

      for (( task=0; task<$nb_tasks; task++ )); do
        echo "*** ~~~ *** Start task"
        startTask "localhost" benchmarkTask_${task} \
                  "benchmarkTask_${task}_${nb_tasks}_${nb_objects}_${size_objects}" \
                  ${size_objects} ${nb_objects}
        TASKS_PIDS+=($pidLastTask)
      done

      echo "Now wait for the tasks to finish "
      wait ${TASKS_PIDS[*]};# the checker never stops, we can't just wait

      sleep 5 # leave time to finish

      #for (( task=0; task<$nb_tasks; task++ )); do
        killall -9 ccdbBenchmark || true ;# ignore errors
        #killAll "qcTaskLauncher" ${NODES_TASKS[${task}]}
     # done

      sleep 5 # leave time to finish

      TASKS_PIDS=()

      echo "OK, ready for the next round !"

    done
  done
done

