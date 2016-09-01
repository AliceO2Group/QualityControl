#!/usr/bin/env bash
set -e ;# exit on error
set -u ;# exit when using undeclared variable
#set -x ;# debugging

# Parameter 1 : csv file to which we will write. default : ./output.txt
# Parameter 2 : path to folder containing the results. default : .

OUTPUT_FILE=${1:-"output.txt"}
LOG_FOLDER=${2:-"."}
METRICS=(
        QC_checker_Mean_pcpu_whole_run
        QC_checker_Mean_pmem_whole_run
        QC_checker_Mean_processing_time_per_event
        QC_checker_Rate_objects_treated_per_second_whole_run
        QC_checker_Total_number_histos_treated
        QC_checker_Time_between_first_and_last_objects_received
        QC_task_Mean_pmem_whole_run
        QC_task_Mean_pcpu_whole_run
        QC_task_Rate_objects_published_per_second_whole_run
        QC_task_Total_duration_activity_whole_run
        QC_task_Total_objects_published_whole_run
        )

# prepare header
header_line="type_of_process, number_tasks, number_checkers, number_histos, number_checks, "
for metric in ${METRICS[@]}; do
  header_line="${header_line}${metric}, "
done
echo ${header_line:-2} > ${OUTPUT_FILE}

# TODO
# For each file like logQcBenchmark_*_0_2_1_10_1000.log
for f in $LOG_FOLDER/logQcBenchmark_*.log;
do
  echo file : ${f}
#   Extract the number of tasks, checkers, checks and histos -> into file comma separated
  type_of_process=`echo ${f} | sed 's/.*logQcBenchmark_\([^_]*\)_[^_]*_\([^_]*\)_\([^_]*\)_\([^_]*\)_\([^_]*\)\.log/\1/'`
  echo $type_of_process
  number_tasks=`echo ${f} | sed 's/.*logQcBenchmark_\([^_]*\)_[^_]*_\([^_]*\)_\([^_]*\)_\([^_]*\)_\([^_]*\)\.log/\2/'`
  echo tasks : $number_tasks
  number_checkers=`echo ${f} | sed 's/.*logQcBenchmark_\([^_]*\)_[^_]*_\([^_]*\)_\([^_]*\)_\([^_]*\)_\([^_]*\)\.log/\3/'`
  echo checkers : $number_checkers
  number_histos=`echo ${f} | sed 's/.*logQcBenchmark_\([^_]*\)_[^_]*_\([^_]*\)_\([^_]*\)_\([^_]*\)_\([^_]*\)\.log/\4/'`
  echo histos : $number_histos
  number_checks=`echo ${f} | sed 's/.*logQcBenchmark_\([^_]*\)_[^_]*_\([^_]*\)_\([^_]*\)_\([^_]*\)_\([^_]*\)\.log/\5/'`
  echo checks : $number_checks

  data_line="$type_of_process, $number_tasks, $number_checkers, $number_histos, $number_checks, "
  for metric in ${METRICS[@]}; do
    value=`tac $f | grep -m 1 "${metric}" | sed "s/.*${metric}, \([^,]*\),.*/\1/"`
    data_line="${data_line}${value}, "
  done
#   And set them in the result file comma separated
  echo ${data_line:-2} >> ${OUTPUT_FILE}
done
