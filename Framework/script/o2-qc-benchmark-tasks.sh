#!/usr/bin/env bash

#set -e ;# exit on error
set -u ;# exit when using undeclared variable
#set -x ;# debugging


MESSAGE_RATE=(1 10 100 1000 10000);
MESSAGE_SIZE=(1 1000 10000 100000 1000000 10000000 100000000);
NB_HISTOS=(1 10 100 1000);
#NB_DISPATCHERS=(1 2 4 8);
NB_BINS=(100);

echo "message_rate        , message_size        , nb_histos           , nb_bins      " > data-sampling-benchmark

for message_rate in ${MESSAGE_RATE[@]}; do
  for message_size in ${MESSAGE_SIZE[@]}; do
    for nb_histos in ${NB_HISTOS[@]}; do
      for nb_bins in ${NB_BINS[@]}; do

        echo "***************************
        Launching test for message rate $message_rate per second, message_size bytes each, number of histos: $nb_histos, number of bins: $nb_bins"

        printf "%20s," "message_rate" >> data-sampling-benchmark
        printf "%21s," "message_size" >> data-sampling-benchmark
        printf "%21s," "nb_histos" >> data-sampling-benchmark
        printf "%21s," "nb_bins" >> data-sampling-benchmark

        #timeout -k 60s 8m dataSamplingBenchmark -q -b --monitoring-backend no-op:// --infologger-severity info --payload-size $payload_size --producers $nb_producers --dispatchers $nb_dispatchers --usleep $usleep_time
#        timeout -k 60s 8m dataSamplingBenchmark -q -b --infologger-severity info --payload-size $payload_size --producers $nb_producers --dispatchers $nb_dispatchers --usleep $usleep_time
#        pkill -9 -f dataSamplingBenchmark
        printf "\n" >> data-sampling-benchmark

      done
    done
  done
done