#!/usr/bin/env bash
set -e ;# exit on error
set -u ;# exit when using undeclared variable
#set -x ;# debugging

### Notes
# Most of the configuration is in the config file we distribute.
# The list of task addresses is defined there as well and we pick
# it up and parse it to know where to launch tasks.
# In the config file, the ports in the variable tasksAddresses must
# match the ones in the benchmark_task definitions.
# TODO the point above could be improved
# One must have ssh keys to connect to all hosts.

### Define matrix of tests
NB_OF_TASKS=(1);# 2 4) ;# 5 10 20 30)
NB_OF_CHECKERS=(1);# 2) ;# 2 5 10)
NB_OF_HISTOS_PER_CYCLE_PER_TASK=(1) ;# 10 100 1000 10000)
NB_OF_CHECKS_PER_CHECKER=(1) ;# 10 100 1000)


### Misc variables
# The log prefix will be followed by the benchmark description, e.g. 1 task 1 checker... or an id or both
LOG_FILE_PREFIX=/tmp/logQcBenchmark_
NUMBER_CYCLES=10 ;# 180 ;# 1 sec per cycle -> ~ 3 minutes + the publication time
TIMEOUT_DURATION=$(awk -v m="$NUMBER_CYCLES" 'BEGIN { print m * 2 }');# 100% margin
ORIGINAL_CONFIG_FILE_NAME=example.ini
MODIFIED_CONFIG_FILE_NAME=newconfig.ini
TASKS_FULL_ADDRESSES="\
tcp://localhost:5556,\
tcp://localhost:5557\
" ;# comma delimited, no space
NODES_CHECKER=("localhost" "localhost")
USER=bvonhall
TASKS_PIDS=()


### Compute addresses of task nodes for our different usages
# Remove the "tcp://" from the addresses, needed for ssh, and store in NODES_TASKS
echo ${TASKS_FULL_ADDRESSES} | sed -n -e 's/tcp:\/\///gp' > /tmp/sed.temp.2
addresses_string=`cat /tmp/sed.temp.2 | sed -n -e 's/:[0-9]*//gp'`
IFS=',' read -r -a NODES_TASKS <<< "$addresses_string" ;# echo "${array[0]}"
# Replace the "localhost" with "*" in the addresses, needed for the tasks config,
# and store in TASKS_ADDRESSES_FOR_CONFIG
echo ${TASKS_FULL_ADDRESSES} | sed -n -e 's/tcp:\/\/[^:]*/tcp:\/\/\*/gp' > /tmp/sed.temp.3
addresses_string=`cat /tmp/sed.temp.3`
IFS=',' read -r -a TASKS_ADDRESSES_FOR_CONFIG <<< "$addresses_string" ;# echo "${array[0]}"


### Utility functions
# Prepare the config file
# \param 1 : number of histos per cycle per task
# \param 2 : number of checks
# \param 3 : number of tasks
# \param 4 : number of checkers
function prepareConfigFile {
  number_histos=$1
  number_checks=$2
  number_tasks=$3
  number_checkers=$4

  # Update the general checkers section
  sed "s/tasksAddresses=.*$/tasksAddresses="${TASKS_FULL_ADDRESSES//\//\\/}"/g" < ${ORIGINAL_CONFIG_FILE_NAME} > ${MODIFIED_CONFIG_FILE_NAME}
  sed -i "s/numberCheckers=.*$/numberCheckers="${number_checkers}"/" ${MODIFIED_CONFIG_FILE_NAME}
  sed -i "s/numberTasks=.*$/numberTasks="${number_tasks}"/" ${MODIFIED_CONFIG_FILE_NAME}

  # Generate tasks
  for (( task=0; task<${number_tasks}; task++ )); do
    echo "
          [benchmarkTask_$task]
          taskDefinition=benchmark
          address=${TASKS_ADDRESSES_FOR_CONFIG[$task]}
          " >> ${MODIFIED_CONFIG_FILE_NAME}
  done

  # Generate checkers
  for (( checker=0; checker<${number_checkers}; checker++ )); do
    echo "
          [checker_$checker]
          id=$checker
          " >> ${MODIFIED_CONFIG_FILE_NAME}
  done
}
# Send the config file to host
# \param 1 : host
function sendConfigFile {
  host=$1
  scp ${MODIFIED_CONFIG_FILE_NAME} ${USER}@${host}:~/${ORIGINAL_CONFIG_FILE_NAME}
}
# Start a task
# \param 1 : host
# \param 2 : name of the task
# \param 3 : log file suffix
function startTask {
  host=$1
  name=$2
  log_file_suffix=$3
  log_file_name=${LOG_FILE_PREFIX}${log_file_suffix}.log
  echo "Starting task ${name} on host ${host}, logs in ${log_file_name}"
  echo "qcTaskLauncher -c file:${ORIGINAL_CONFIG_FILE_NAME} -n ${name} -C ${NUMBER_CYCLES} > ${log_file_name} 2>&1"
  ssh ${USER}@${host} "qcTaskLauncher -c file:${ORIGINAL_CONFIG_FILE_NAME} -n ${name} -C ${NUMBER_CYCLES} > ${log_file_name} 2>&1" &
  pidLastTask=$!
}
# Start a checker
# \param 1 : host
# \param 2 : name of the checker
# \param 3 : log file suffix
function startChecker {
  host=$1
  name=$2
  log_file_suffix=$3
  log_file_name=${LOG_FILE_PREFIX}${log_file_suffix}.log
  echo "Starting checker ${name} on host ${host}, logs in ${log_file_name}"
  echo "qcCheckerLauncher -c file:${ORIGINAL_CONFIG_FILE_NAME} -n ${name} \
                > ${log_file_name} 2>&1"
  ssh ${USER}@${host} "qcCheckerLauncher -c file:${ORIGINAL_CONFIG_FILE_NAME} -n ${name}  \
                > ${log_file_name} 2>&1" &
  pidLastChecker=$!
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


### Benchmark starts here
# Loop through the matrix of tests
for nb_tasks in ${NB_OF_TASKS[@]}; do
  for nb_checkers in ${NB_OF_CHECKERS[@]}; do
    for nb_histos in ${NB_OF_HISTOS_PER_CYCLE_PER_TASK[@]}; do
      for nb_checks in ${NB_OF_CHECKS_PER_CHECKER[@]}; do
        echo "***************************
        Launching test for $nb_tasks tasks, $nb_checkers checkers, $nb_histos histos, $nb_checks checks"

        if (( $nb_tasks < nb_checkers ))
        then
          echo "number tasks is less than number checkers, skip"
          continue;
        fi

        prepareConfigFile $nb_histos $nb_checks $nb_tasks $nb_checkers

	      echo "Kill all old processes"
        for (( checker=0; checker<$nb_checkers; checker++ )); do
	        killAll "qcCheckerLauncher" ${NODES_CHECKER[${checker}]} "-9"
	      done
        for (( task=0; task<$nb_tasks; task++ )); do
	        killAll "qcTaskLauncher" ${NODES_TASKS[${task}]} "-9"
	      done

        for (( checker=0; checker<$nb_checkers; checker++ )); do
          sendConfigFile ${NODES_CHECKER[${checker}]}
          startChecker ${NODES_CHECKER[${checker}]} checker_${checker} \
                    "checker_${checker}_${nb_tasks}_${nb_checkers}_${nb_histos}_${nb_checks}"
        done

        echo "Let the checkers start up... wait 2 sec."
        sleep 2
        echo "Now start the tasks"

        for (( task=0; task<$nb_tasks; task++ )); do
          sendConfigFile ${NODES_TASKS[${task}]}
          startTask ${NODES_TASKS[${task}]} benchmarkTask_${task} \
                    "benchmarkTask_${task}_${nb_tasks}_${nb_checkers}_${nb_histos}_${nb_checks}"
          TASKS_PIDS+=($pidLastTask)
        done

        echo "Now wait for the tasks to finish"
        wait ${TASKS_PIDS[*]};# the checker never stops, we can't just wait

        for (( checker=0; checker<$nb_checkers; checker++ )); do
	        killAll "qcCheckerLauncher" ${NODES_CHECKER[${checker}]}
	      done
        for (( task=0; task<$nb_tasks; task++ )); do
	        killAll "qcTaskLauncher" ${NODES_TASKS[${task}]}
	      done
	      sleep 10 # leave time to finish

	      TASKS_PIDS=()

        echo "OK, ready for the next round !"

      done
    done
  done
done

