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
NB_OF_TASKS=(1) ;# 5 10 20 30)
NB_OF_CHECKERS=(1) ;# 2 5 10)
NB_OF_HISTOS_PER_CYCLE_PER_TASK=(1) ;# 10 100 1000 10000)
NB_OF_CHECKS_PER_CHECKER=(1) ;# 10 100 1000)


### Misc variables
# The log prefix will be followed by the benchmark description, e.g. 1 task 1 checker... or an id or both
LOG_FILE_PREFIX=/tmp/logQcBenchmark_
NUMBER_CYCLES=3 ;# 180 ;# 1 sec per cycle -> ~ 3 minutes + the publication time
ORIGINAL_CONFIG_FILE_NAME=example.ini
MODIFIED_CONFIG_FILE_NAME=newconfig.ini
TASKS_FULL_ADDRESSES="tcp://localhost:5555,tcp://localhost:5556,tcp://localhost:5557,tcp://localhost:5558,tcp://localhost:5559" ;# comma delimited, no space
NODES_CHECKER=("localhost" "localhost" "localhost" "localhost" "localhost" "localhost" "localhost" "localhost" "localhost" "localhost")


### Compute addresses of task nodes for our different usages
# Remove the "tcp://" from the addresses, needed for ssh, and store in NODES_TASKS
echo ${TASKS_FULL_ADDRESSES} | sed -n -e 's/tcp:\/\///gp' > /tmp/sed.temp.2
addresses_string=`cat /tmp/sed.temp.2 | sed -n -e 's/:[0-9]*//gp'`
IFS=',' read -r -a NODES_TASKS <<< "$addresses_string" ;# echo "${array[0]}"
# Replace the "localhost" with "*" in the addresses, needed for the tasks config,
# and store in TASKS_ADDRESSES_FOR_CONFIG
addresses_string=${TASKS_FULL_ADDRESSES//localhost/*}
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
  scp ${MODIFIED_CONFIG_FILE_NAME} ${host}:~
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
  ssh ${host} "qcTaskLauncher -c file:${MODIFIED_CONFIG_FILE_NAME} -n ${name} -C ${NUMBER_CYCLES} \
                > ${log_file_name} 2>&1" &
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
  ssh ${host} "qcTaskLauncher -c file:${MODIFIED_CONFIG_FILE_NAME} -n ${name} -C ${NUMBER_CYCLES} \
                > ${log_file_name} 2>&1" &
}


### Benchmark starts here
# Loop through the matrix of tests
for nb_tasks in ${NB_OF_TASKS[@]}; do
  for nb_checkers in ${NB_OF_CHECKERS[@]}; do
    for nb_histos in ${NB_OF_HISTOS_PER_CYCLE_PER_TASK[@]}; do
      for nb_checks in ${NB_OF_CHECKS_PER_CHECKER[@]}; do
        echo "Launching test for $nb_tasks tasks, $nb_checkers checkers, $nb_histos histos, $nb_checks checks"

        prepareConfigFile $nb_histos $nb_checks $nb_tasks $nb_checkers

        for (( task=0; task<$nb_tasks; task++ )); do
          sendConfigFile ${NODES_TASKS[${task}]}
          startTask ${NODES_TASKS[${task}]} benchmarkTask_${task} \
                    "benchmarkTask_${task}_${nb_tasks}_${nb_checkers}_${nb_histos}_${nb_checks}"
        done

        for (( checker=0; checker<$nb_checkers; checker++ )); do
          sendConfigFile ${NODES_CHECKER[${checker}]}
          startChecker ${NODES_CHECKER[${checker}]} checker_${checker} \
                    "checker_${checker}_${nb_tasks}_${nb_checkers}_${nb_histos}_${nb_checks}"
        done

        echo "Now wait..."
        wait

      done
    done
  done
done



#
#
#
#TASKSET="taskset -c ${TASKSET}"
#
#LIMIT=$((1000000000000 / EVENT))
#
#APP_SERVER=${SERVER}
##APP_SERVER=192.168.1.12
#
#### START
#if [ "${LIB}" = "zeromq" ]
#then
#	echo "Running ZeroMQ benchmark..."
#	ssh ${SERVER} "${TASKSET} /opt/zeromq/local_thr tcp://${APP_SERVER}:5555 ${EVENT} ${LIMIT} > /dev/null 2>&1" &
#	ssh ${CLIENT} "${TASKSET} /opt/zeromq/remote_thr tcp://${APP_SERVER}:5555 ${EVENT} ${LIMIT} > /dev/null 2>&1" &
#elif [ "${LIB}" = "nanomsg" ]
#then
#	echo "Running nanomsg benchmark..."
#        ssh ${SERVER} "${TASKSET} /opt/nanomsg/perf/local_thr tcp://${APP_SERVER}:5555 ${EVENT} ${LIMIT} > /dev/null 2>&1" &
#        ssh ${CLIENT} "${TASKSET} /opt/nanomsg/perf/remote_thr tcp://${APP_SERVER}:5555 ${EVENT} ${LIMIT} > /dev/null 2>&1" &
#elif [ "${LIB}" = "nanomsg10" ]
#then
#        echo "Running nanomsg v1.0.0 benchmark..."
#        ssh ${SERVER} "${TASKSET} /opt/nanomsg-1.0.0/build/local_thr tcp://${APP_SERVER}:5555 ${EVENT} ${LIMIT} > /dev/null 2>&1" &
#        ssh ${CLIENT} "${TASKSET} /opt/nanomsg-1.0.0/build/remote_thr tcp://${APP_SERVER}:5555 ${EVENT} ${LIMIT} > /dev/null 2>&1" &
#elif [ "${LIB}" = "asio" ]
#then
#	MEM=$((10*${EVENT}))
#	echo "Running Boost Asio benchmark..."
#	ssh ${SERVER} "${TASKSET} /opt/asio/epn ${EVENT} > /dev/null 2>&1" &
#	sleep 0.5
#        ssh ${CLIENT} "${TASKSET} /opt/asio/flp ${APP_SERVER} ${EVENT} ${MEM} > /dev/null 2>&1" &
#elif [ "${LIB}" = "fairmq" ]
#then
#        echo "Running FaiRoot benchmark..."
#        echo ssh ${SERVER} "${TASKSET} /opt/FairRoot_install/bin/bsampler --id bsampler1 --event-rate 0 --config-json-file /opt/FairRoot.cfg --event-size ${EVENT} > /dev/null 2>&1" &
#	echo ssh ${CLIENT} "${TASKSET} /opt/FairRoot_install/bin/sink --config-json-file /opt/FairRoot.cfg --id sink1  > /dev/null 2>&1" &
#elif [ "${LIB}" = "aliceo2" ]
#then
#        echo "Running AliceO2 benchmark..."
#        ssh ${SERVER} "${TASKSET} /opt/AliceO2/build_o2/bin/testEPN --input-method bind --input-address tcp://${APP_SERVER}:5555 --id arg 102 --input-buff-size 10 --input-socket-type pull" &
#        ssh ${CLIENT} "${TASKSET} /opt/AliceO2/build_o2/bin/testFLP --id arg 11 --output-socket-type push --output-method connect --output-address tcp://${APP_SERVER}:5555 --output-buff-size 10 --event-size ${EVENT}" &
#else
#	echo "Wrong -l|--lib parameter"
#	exit 1
#fi
#
#sleep ${TIME}
#
#
#### STOP
#if [ "${LIB}" = "zeromq" ]
#then
#        ssh ${SERVER} "pkill _thr"
#        ssh ${CLIENT} "pkill _thr"
#        echo "ZeroMQ benchmark finished"
#elif [ "${LIB}" = "nanomsg" ]
#then
#        ssh ${SERVER} "pkill _thr"
#        ssh ${CLIENT} "pkill _thr"
#        echo "nanomsg benchmark finished"
#elif [ "${LIB}" = "nanomsg10" ]
#then
#        ssh ${SERVER} "pkill _thr"
#        ssh ${CLIENT} "pkill _thr"
#        echo "nanomsg v1.0.0 benchmark finished"
#elif [ "${LIB}" = "asio" ]
#then
#        ssh ${SERVER} "pkill epn"
#        ssh ${CLIENT} "pkill flp"
#        echo "Boost Asio benchmark finished"
#elif [ "${LIB}" = "fairmq" ]
#then
#        ssh ${SERVER} "pkill bsampler"
#        ssh ${CLIENT} "pkill sink"
#        echo "FairRoot benchmark finished"
#elif [ "${LIB}" = "aliceo2" ]
#then
#        ssh ${SERVER} "pkill testEPN"
#        ssh ${CLIENT} "pkill testFLP"
#        echo "AliceO2 benchmark finished"
#
#fi
