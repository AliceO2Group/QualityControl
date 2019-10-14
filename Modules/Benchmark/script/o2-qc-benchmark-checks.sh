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

function inplace_sed() {
  sed -ibck "$1" $2 && rm $2bck
}

# Run the benchmark with given parameters
# \param 1 : number of checks
# \param 2 : number of histograms array name
# \param 3 : number of bins array name
# \param 4 : cycle_seconds
# \param 5 : repetitions
# \param 6 : test_duration
# \param 7 : warm up cycles - how many first metrics should be ignored (they are sent each 10s)
# \param 8 : test name
# \param 9: fill - (yes/no) should write zeroes to the produced messages (prevents memory overcommitting)
function benchmark() {

  local number_of_checks_name=$1[@]
  local number_of_histograms_name=$2[@]
  local number_of_bins_name=$3[@]

  local number_of_checks=("${!number_of_checks_name}")
  local number_of_histograms=("${!number_of_histograms_name}")
  local number_of_bins=("${!number_of_bins_name}")

  local cycle_seconds=$4
  local repetitions=$5
  local test_duration=$6
  local warm_up_cycles=$7
  local test_name=$8
  local fill=${9}

  # we assume that no QC Task might sustain more than this value of B/s
  # and we will trim down the data rates accordingly to be gentle with memory
  local max_data_input=500

  if [ ! -z $TESTS ] && [[ ! " ${TESTS[@]} " =~ " ${test_name} " ]]; then
    echo "Test '"$test_name"' ignored"
    return
  fi

  echo "======================================================================="
  echo "Running the test '"$test_name"'"

#  local test_duration_timeout=$((test_duration + 60))
  local repo_latest_commit=$(git rev-list --format=oneline --max-count=1 HEAD)
  local repo_branch=$(git rev-parse --abbrev-ref HEAD)
  local results_filename='qc-check-benchmark-'$(date +"%y-%m-%d_%H%M")'-'$test_name
  local test_date=$(date +"%y-%m-%d %H:%M:%S")
  local config_file_template='../etc/benchmarkCheckTemplate.json'
  local config_file_concrete='benchmarkCheck.json'
  local run_log='run_log'
  local check_config='\"AlwaysGoodCheck__CHECK_NO__\":{\"active\":\"true\",\"className\":\"o2::quality_control_modules::benchmark::AlwaysGoodCheck\",\"moduleName\":\"QcBenchmark\",\"policy\":\"OnAny\",\"detectorName\":\"TST\",\"dataSource\":[{\"type\":\"Task\",\"name\":\"BenchmarkTask\"}]},__CHECKS__'

  printf "QC CHECK RUNNER BENCHMARK RESULTS\n" > $results_filename
  printf "Date:                   %s\n" "$test_date" >> $results_filename
  printf "Latest commit:          %s\n" "$repo_latest_commit" >> $results_filename
  printf "Branch:                 %s\n" "$repo_branch" >> $results_filename
  printf "Repetitions:            %s\n" "$repetitions" >> $results_filename
  printf "Test duration [s]:      %s\n" "$test_duration" >> $results_filename
  printf "Warm up cycles:         %s\n" "$warm_up_cycles" >> $results_filename
  echo "nb_checks   , nb_histograms,      nb_bins, objs_per_second, checks_per_second" >> $results_filename
  local qc_common_args="--run -b --shm-segment-size 50000000000 --infologger-severity info --config json:/"`pwd`'/'$config_file_concrete
  local producer_common_args="-b"
  if [[ $fill == "no" ]]; then
    producer_common_args=$producer_common_args' --empty'
  fi

  for nb_checks in ${number_of_checks[@]}; do
    for nb_histograms in ${number_of_histograms[@]}; do
      for nb_bins in ${number_of_bins[@]}; do
        echo "************************************************************"
        echo "Launching the test for $nb_checks checks per object, $nb_histograms histograms, $nb_bins bins"

        echo "Creating a config file..."
        rm -f $config_file_concrete

        sed 's/__CYCLE_SECONDS__/'$cycle_seconds'/; s/__NUMBER_OF_HISTOGRAMS__/'$nb_histograms'/; s/__NUMBER_OF_BINS__/'$nb_bins'/' $config_file_template > $config_file_concrete

        # create a json string with all check configurations
        total_checks_config=
        for ((check_no=0;check_no<nb_checks;check_no++)); do
          new_check=`echo ${check_config} | sed 's/__CHECK_NO__/'$check_no'/g'`
          inplace_sed 's/__CHECKS__/\n'$new_check'/g' $config_file_concrete
        done
        # cut last comma and tag
        inplace_sed 's/,__CHECKS__//g' $config_file_concrete

        echo "...created."

        # preparing the command
        command_producer="o2-qc-run-producer "$producer_common_args" --min-size 100 --max-size 100 --producers 1 --message-rate 1"
        command_qc="o2-qc "$qc_common_args
        command=$command_producer" | "$command_qc

        for ((rep=0;rep<repetitions;rep++)); do

          echo "*****************"
          echo "Repetition: "$rep


          printf "%12s," "$nb_checks" >> $results_filename
          printf "%14s," "$nb_histograms" >> $results_filename
          printf "%13s," "$nb_bins" >> $results_filename

          checks_per_second=
          state=
          while [ "$state" == 'error' ] || [ -z "$state" ]; do
            if [ "$state" == 'error' ]; then
              echo "Retrying the test because of an error"
              mv run_log run_log_error_"$(date +"%y-%m-%d-%H:%M:%S")"
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

            mapfile -t metrics_checks_executed < <(grep -a 'qc_checks_executed' $run_log | grep -o -e 'value=[0-9]\{1,\}' | sed -e 's/value=//' | tail -n +$warm_up_cycles)
            mapfile -t metrics_objects_received < <(grep -a 'qc_objects_received' $run_log | grep -o -e 'value=[0-9]\{1,\}' | sed -e 's/value=//' | tail -n +$warm_up_cycles)
            mapfile -t metrics_test_duration < <(grep -a 'qc_checks_executed' $run_log | grep -o -e '[0-9]\{1,\} hostname' | sed -e 's/ hostname//' | tail -n +$warm_up_cycles)

            if [ ${#metrics_checks_executed[@]} -ge 2 ] && [ ${#metrics_test_duration[@]} -ge 2 ] && [ ${#metrics_objects_received[@]} -ge 2 ]; then

              (( total_checks_executed = metrics_checks_executed[-1] - metrics_checks_executed[0] ))
              (( total_objects_received = metrics_objects_received[-1] - metrics_objects_received[0] ))
              (( total_test_duration_ms = metrics_test_duration[-1] - metrics_test_duration[0] ))

              checks_per_second=`echo "scale=3; $total_checks_executed*1000/$total_test_duration_ms" | bc -l`
              objects_per_second=`echo "scale=3; $total_objects_received*1000/$total_test_duration_ms" | bc -l`
            else
              state='error'
            fi
          done

          printf "%16s," "$objects_per_second" >> $results_filename
          printf "%16s" "$checks_per_second" >> $results_filename
          printf "\n" >> $results_filename

        done
      done
    done
  done

  # cleanups
  rm -f $config_file_concrete
  rm -f $run_log
}

function print_usage() {
  echo "Usage: ./o2-qc-benchmark-checks.sh [-f] [-m MEMORY_USAGE] [-t TEST]

Run Check Runners Benchmark and create report files in the directory. Execute from within the same directory, after
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

# global parameters
REPETITIONS=5;
TEST_DURATION=300;
WARM_UP_CYCLES=5;

# test-specific parameters
NB_CHECKS=(1);
PAYLOAD_SIZE=(256);
NB_HISTOGRAMS=(1 4 16 64 256 1024 4096);
NB_BINS=(64000);
CYCLE_SECONDS=1
TEST_NAME='objects'

benchmark NB_CHECKS NB_HISTOGRAMS NB_BINS $CYCLE_SECONDS $REPETITIONS $TEST_DURATION $WARM_UP_CYCLES $TEST_NAME $FILL

NB_CHECKS=(1 4 16 64 256);
PAYLOAD_SIZE=(256);
NB_HISTOGRAMS=(1 100);
NB_BINS=(64000);
CYCLE_SECONDS=1
TEST_NAME='checks'

benchmark NB_CHECKS NB_HISTOGRAMS NB_BINS $CYCLE_SECONDS $REPETITIONS $TEST_DURATION $WARM_UP_CYCLES $TEST_NAME $FILL

NB_CHECKS=(1 100);
PAYLOAD_SIZE=(256);
NB_HISTOGRAMS=(1 100);
NB_BINS=(1 10 100 1000 10000 100000 1000000);
CYCLE_SECONDS=1
TEST_NAME='object-size'

benchmark NB_CHECKS NB_HISTOGRAMS NB_BINS $CYCLE_SECONDS $REPETITIONS $TEST_DURATION $WARM_UP_CYCLES $TEST_NAME $FILL
