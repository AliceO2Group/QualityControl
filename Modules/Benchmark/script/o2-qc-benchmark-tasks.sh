#!/usr/bin/env bash

#set -e ;# exit on error
set -u ;# exit when using undeclared variable
#set -x ;# debugging

trap kill_benchmark INT

function check_installed() {
  # very stupid but easy way to check if a package is installed
  $1 --version > /dev/null
  if [ $? -ne 0 ]; then
    echo "Please install the package "$1" before running the benchmark."
    exit 1;
  fi
}

echo 'Checking if all necessary packages are installed...'
check_installed tail
check_installed bc
check_installed pkill
check_installed sed
check_installed grep
echo '...all are there.'

function kill_benchmark() {
  echo "Killing any running benchmark workflows..."
  pkill -9 -f 'o2-qc '
  pkill -9 -f 'o2-qc-run-producer'
  exit 1
}

# Run the benchmark with given parameters
# \param 1 : number of producers array name
# \param 2 : payload sizes array name
# \param 3 : number of histograms array name
# \param 4 : number of bins array name
# \param 5 : cycle_seconds
# \param 6 : repetitions
# \param 7 : test_duration
# \param 8 : warm up cycles - how many first metrics should be ignored (they are sent each 10s)
# \param 9 : test name
# \param 10: fill - (yes/no) should write zeroes to the produced messages (prevents memory overcommitment)
# \param 11: max input data throughput
function benchmark() {

  local number_of_producers_name=$1[@]
  local payload_sizes_name=$2[@]
  local number_of_histograms_name=$3[@]
  local number_of_bins_name=$4[@]

  local number_of_producers=("${!number_of_producers_name}")
  local payload_sizes=("${!payload_sizes_name}")
  local number_of_histograms=("${!number_of_histograms_name}")
  local number_of_bins=("${!number_of_bins_name}")

  local cycle_seconds=$5
  local repetitions=$6
  local test_duration=$7
  local warm_up_cycles=$8
  local test_name=$9
  local fill=${10}
  # we assume that no QC Task might sustain more than this value of B/s
  # and we will trim down the data rates accordingly to be gentle with memory
  local max_data_input=${11}


  if [ ! -z $TESTS ] && [[ ! " ${TESTS[@]} " =~ " ${test_name} " ]]; then
    echo "Test '"$test_name"' ignored"
    return
  fi

  echo "======================================================================="
  echo "Running the test '"$test_name"'"

#  local test_duration_timeout=$((test_duration + 60))
  local repo_latest_commit=$(git rev-list --format=oneline --max-count=1 HEAD)
  local repo_branch=$(git rev-parse --abbrev-ref HEAD)
  local results_filename='qc-task-benchmark-'$(date +"%y-%m-%d_%H%M")'-'$test_name
  local test_date=$(date +"%y-%m-%d %H:%M:%S")
  local config_file_template='../etc/benchmarkTaskTemplate.json'
  local config_file_concrete='benchmarkTask.json'
  local run_log='run_log'

  printf "QC TASK RUNNER BENCHMARK RESULTS\n" > $results_filename
  printf "Date:                   %s\n" "$test_date" >> $results_filename
  printf "Latest commit:          %s\n" "$repo_latest_commit" >> $results_filename
  printf "Branch:                 %s\n" "$repo_branch" >> $results_filename
  printf "Repetitions:            %s\n" "$repetitions" >> $results_filename
  printf "Test duration [s]:      %s\n" "$test_duration" >> $results_filename
  printf "Warm up cycles:         %s\n" "$warm_up_cycles" >> $results_filename
  echo "nb_producers, payload_sizes, nb_histograms,      nb_bins, msgs_per_second, data_per_second, objs_published_per_second" >> $results_filename
  local qc_common_args="--run -b --shm-segment-size 50000000000 --infologger-severity info --config json:/"`pwd`'/'$config_file_concrete
  local producer_common_args="-b"
  if [[ $fill == "no" ]]; then
    producer_common_args=$producer_common_args' --empty'
  fi

  for nb_producers in ${number_of_producers[@]}; do
    for payload_size in ${payload_sizes[@]}; do
      for nb_histograms in ${number_of_histograms[@]}; do
        for nb_bins in ${number_of_bins[@]}; do
          echo "************************************************************"
          echo "Launching the test for payload size $payload_size bytes, $nb_producers producers, $nb_histograms histograms, $nb_bins bins"

          echo "Creating a config file..."
          rm -f $config_file_concrete
          sed 's/__CYCLE_SECONDS__/'$cycle_seconds'/; s/__NUMBER_OF_HISTOGRAMS__/'$nb_histograms'/; s/__NUMBER_OF_BINS__/'$nb_bins'/' $config_file_template > $config_file_concrete
          echo "...created."

          # calculating parameters
          (( message_rate = max_data_input / payload_size / nb_producers))

          # preparing the command
          command_producer="o2-qc-run-producer "$producer_common_args" --min-size "$payload_size" --max-size "$payload_size" --timepipeline "$nb_producers" --message-rate "$message_rate
          command_qc="o2-qc "$qc_common_args
          command=$command_producer" | "$command_qc

          for ((rep=0;rep<repetitions;rep++)); do

            echo "*****************"
            echo "Repetition: "$rep


            printf "%12s," "$nb_producers" >> $results_filename
            printf "%14s," "$payload_size" >> $results_filename
            printf "%14s," "$nb_histograms" >> $results_filename
            printf "%13s," "$nb_bins" >> $results_filename

            messages_per_second=
            state=
            while [ "$state" == 'error' ] || [ -z "$state" ]; do
              if [ "$state" == 'error' ]; then
                echo "Retrying the test because of an error"
                mv run_log run_log_error"$(date +"%y-%m-%d-%H:%M:%S")"
              fi
              state='ok'

              # Running the benchmark
              rm -f $run_log
              echo "Used command:"
              echo "timeout -k 5s $test_duration $command > $run_log"
              timeout -k 5s $test_duration bash -c "$command > $run_log"

              # Cleaning up potentially leftover processes. Notice the space after o2-qc to avoid suicide
              pkill -9 -f 'o2-qc '
              pkill -9 -f 'o2-qc-run-producer'

              # Check the log in case of errors, repeat then
              if grep 'ERROR\|segmentation violation' $run_log; then
                state='error'
                echo 'Error in the test:'
                echo `grep 'ERROR\|segmentation' $run_log`
                continue
              fi

              mapfile -t metrics_messages_in_cycle < <(grep -a 'qc_data_received' $run_log | grep -o -e 'messages_in_cycle=[0-9]\{1,\}' | sed -e 's/messages_in_cycle=//' | tail -n +$warm_up_cycles)
              mapfile -t metrics_data_in_cycle < <(grep -a 'qc_data_received' $run_log | grep -o -e 'data_in_cycle=[0-9]\{1,\}' | sed -e 's/data_in_cycle=//' | tail -n +$warm_up_cycles)
              mapfile -t metrics_objects_published_in_cycle < <(grep -a 'qc_objects_published' $run_log | grep -o -e 'in_cycle=[0-9]\{1,\}' | sed -e 's/in_cycle=//' | tail -n +$warm_up_cycles)
              mapfile -t metrics_cycle_duration < <(grep -a 'qc_duration' $run_log | grep -o -e 'module_cycle=[0-9]\{1,\}.[0-9]\{1,\}' | sed -e 's/module_cycle=//' | tail -n +$warm_up_cycles)

              if [ ${#metrics_messages_in_cycle[@]} -ge 2 ] && [ ${#metrics_cycle_duration[@]} -ge 2 ] && [ ${#metrics_objects_published_in_cycle[@]} -ge 2 ] && [ ${#metrics_data_in_cycle[@]} -ge 2 ]; then

                total_messages=0
                for ((i = 0; i < ${#metrics_messages_in_cycle[@]}; i++))
                do
                  (( total_messages+=metrics_messages_in_cycle[i] ))
                done

                total_data=0
                for ((i = 0; i < ${#metrics_data_in_cycle[@]}; i++))
                do
                  (( total_data+=metrics_data_in_cycle[i] ))
                done

                total_objects_published=0
                for ((i = 0; i < ${#metrics_objects_published_in_cycle[@]}; i++))
                do
                  (( total_objects_published+=metrics_objects_published_in_cycle[i] ))
                done

                total_time=0.0
                for ((i = 0; i < ${#metrics_cycle_duration[@]}; i++))
                do
                  total_time=`echo "scale=3; $total_time+${metrics_cycle_duration[i]}" | bc -l`
                done

                messages_per_second=`echo "scale=3; $total_messages/$total_time" | bc -l`
                data_per_second=`echo "scale=3; $total_data/$total_time" | bc -l`
                objects_per_second=`echo "scale=3; $total_objects_published/$total_time" | bc -l`
              else
                state='error'
              fi
            done

            printf "%16s," "$messages_per_second" >> $results_filename
            printf "%16s," "$data_per_second" >> $results_filename
            printf "%16s" "$objects_per_second" >> $results_filename
            printf "\n" >> $results_filename

          done
        done
      done
    done
  done

  # cleanups
  rm -f $config_file_concrete
  rm -f $run_log
}

function print_usage() {
  echo "Usage: ./o2-qc-benchmark-tasks.sh [-f] [-m MEMORY_USAGE] [-t TEST]

Run Task Runners Benchmark and create report files in the directory. Execute from within the same directory, after
having entered the QualityControl environment.

Options:
 -h               Print this message
 -t TEST          Test name to be run. Use it multiple times to run multiple tests. If not specified, all are run. See
                  the last part of this script to find the available tests.
 -f               Fill QC Task input messages. It slows down producers, but prevents from overcommitting memory.
"
}


FILL=no
TESTS=

while getopts 'hfm:t:' option; do
  case "${option}" in
  \?)
    print_usage
    exit 1
    ;;
  h)
    print_usage
    exit 0
    ;;
  f)
    FILL=yes
    ;;
  m)
    MEMORY_USAGE=$OPTARG
    ;;
  t)
    TESTS=("$OPTARG")
    printf '%s\n' "${TESTS[@]}"
    ;;
  esac
done

REPETITIONS=5;
TEST_DURATION=300;
WARM_UP_CYCLES=5;

NB_PRODUCERS=(1 2 4 8 16);
PAYLOAD_SIZE=(256 2000000);
NB_HISTOGRAMS=(1);
NB_BINS=(100);
CYCLE_SECONDS=10
MAX_INPUT_DATA_THROUGHPUT=5000000000
TEST_NAME='producers'

benchmark NB_PRODUCERS PAYLOAD_SIZE NB_HISTOGRAMS NB_BINS $CYCLE_SECONDS $REPETITIONS $TEST_DURATION $WARM_UP_CYCLES $TEST_NAME $FILL $MAX_INPUT_DATA_THROUGHPUT

NB_PRODUCERS=(8);
PAYLOAD_SIZE=(1 256 1024 4096 16384 65536 262144 1048576 4194304 16777216 67108864 268435456);
NB_HISTOGRAMS=(1);
NB_BINS=(100);
CYCLE_SECONDS=10;
MAX_INPUT_DATA_THROUGHPUT=5000000000
TEST_NAME='payload-sizes'

benchmark NB_PRODUCERS PAYLOAD_SIZE NB_HISTOGRAMS NB_BINS $CYCLE_SECONDS $REPETITIONS $TEST_DURATION $WARM_UP_CYCLES $TEST_NAME $FILL $MAX_INPUT_DATA_THROUGHPUT

NB_PRODUCERS=(1);
PAYLOAD_SIZE=(256);
NB_HISTOGRAMS=(100);
NB_BINS=(1 10 100 1000 10000 100000 1000000);
CYCLE_SECONDS=1;
MAX_INPUT_DATA_THROUGHPUT=500
TEST_NAME='obj-size'

benchmark NB_PRODUCERS PAYLOAD_SIZE NB_HISTOGRAMS NB_BINS $CYCLE_SECONDS $REPETITIONS $TEST_DURATION $WARM_UP_CYCLES $TEST_NAME $FILL $MAX_INPUT_DATA_THROUGHPUT

NB_PRODUCERS=(1);
PAYLOAD_SIZE=(256);
NB_HISTOGRAMS=(1 4 16 64 256 1024);
NB_BINS=(64000);
CYCLE_SECONDS=1;
MAX_INPUT_DATA_THROUGHPUT=500
TEST_NAME='obj-amount'

benchmark NB_PRODUCERS PAYLOAD_SIZE NB_HISTOGRAMS NB_BINS $CYCLE_SECONDS $REPETITIONS $TEST_DURATION $WARM_UP_CYCLES $TEST_NAME $FILL $MAX_INPUT_DATA_THROUGHPUT
