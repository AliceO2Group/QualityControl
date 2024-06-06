#!/usr/bin/env bash
set -e # exit on error
set -u # exit when using undeclared variable
#set -x ;# debugging

DONOR=Skeleton
DONOR_LC=skeleton

OS=`uname`

function inplace_sed() {
  sed -ibck "$1" $2 && rm $2bck
}

# Checks if current pwd matches the location of this script, exits if it is not
function check_pwd() {
  DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
  if [[ $DIR != $PWD ]]; then
    echo 'Please execute this script while being in its directory'
    exit 1
  fi
}

# Create new empty module
# \param 1 : module_name
function create_module() {
  if [ -d $1 ]; then
    echo 'Module '$1' already exists.'
  else
    echo 'Module '$1' does not exist, generating...'
    if [ ! -d ${DONOR} ]; then
      echo '> Donor template '${DONOR}' does not exist, exiting...'
      exit 1
    fi

    MODULE_LC=$(echo $1 | tr A-Z a-z)
    # prepare folder structure
    mkdir $1 $1/src $1/include/ $1/include/$1 $1/test

    # prepare CMakeLists.txt
    sed 's/'${DONOR}'/'$1'/' ${DONOR}/.CMakeListsEmpty.txt >$1/CMakeLists.txt
    # prepare LinkDef.h
    sed '/#pragma link C++ class o2::quality_control_modules::'${DONOR_LC}'::/ d' ${DONOR}/include/${DONOR}/LinkDef.h >$1/include/$1/LinkDef.h
    # prepare test
    sed 's/.testEmpty/test'$1'/; s/'${DONOR_LC}'/'${MODULE_LC}'/' ${DONOR}/test/.testEmpty.cxx >$1'/test/testQc'$1'.cxx'

    inplace_sed '/testQcSkeleton/s/Skeleton/'$1'/g' $1/CMakeLists.txt

    # add new module to the project
    if ! grep -c $1 CMakeLists.txt; then
      echo 'add_subdirectory('$1')' >>CMakeLists.txt
    fi

    echo '> Module created.'
  fi
}

function cmake_format() {
  if command cmake-format >/dev/null 2>&1; then
    cmake-format -i $1 -c ../.cmake-format.py
  fi
}

# Return the name of the include guard
function include_guard() {
  local modulename=$1
  local classname=$2
  echo QC_MODULE_$(echo $modulename | tr a-z A-Z)_$(echo $modulename$classname | tr a-z A-Z)_H
}

# Return the name of the include file
function include_file() {
  local modulename=$1
  local classname=$2
  echo $modulename'/include/'$modulename'/'$classname'.h'
}

# Create new Task or Check in specified module
function create_class() {

  local modulename=$1
  local classname=$2
  local typename=$3

  if [ "$typename" != "Task" ] && [ "$typename" != "Check" ] && [ "$typename" != "PostProcessing" ] && [ "$typename" != "Aggregator" ]; then
    echo "3rd parameter can only be Task, Check, Aggregator or PostProcessing"
    return
  fi

  INCLUDE_FILENAME=$(include_file $modulename $classname)

  echo 'Creating '$typename' '$classname' in module '$modulename'.'
  if [ -f $INCLUDE_FILENAME ]; then
    echo '> '$typename' '$classname' already exists, returning...'
    return
  fi

  MODULE_LC=$(echo $modulename | tr A-Z a-z)
  DONOR_INCLUDE_GUARD=$(include_guard $DONOR $typename)
  INCLUDE_GUARD=$(include_guard $modulename $classname)
  DONOR_INCLUDE_FILENAME=$(include_file $DONOR ${DONOR}${typename})

  # add header
  sed '
  s/'${DONOR}${typename}'/'${classname}'/g;
  s/'${DONOR_LC}'/'${MODULE_LC}'/g;
  s/'${DONOR}'/'${MODULE}'/g;
  s/'${DONOR_INCLUDE_GUARD}'/'${INCLUDE_GUARD}'/g;
  ' $DONOR_INCLUDE_FILENAME >$INCLUDE_FILENAME

  # add LinkDef.h
  if [[ $OS == Linux ]] ; then
    sed -i '/#endif/ i #pragma link C++ class o2::quality_control_modules::'${MODULE_LC}'::'$classname'+;' \
      $modulename/include/$modulename/LinkDef.h
    sed -i '/HEADERS/ a \ \ include/'$modulename'/'$classname'.h' $modulename/CMakeLists.txt
  else #Darwin/BSD
    inplace_sed '/#endif/ i\
      #pragma link C++ class o2::quality_control_modules::'${MODULE_LC}'::'$classname'+;\
      \
      ' $modulename/include/$modulename/LinkDef.h
    inplace_sed '/LINKDEF include/ i\
      include/'$modulename'/'$classname'.h\
      ' $modulename/CMakeLists.txt
  fi

  # add src
  sed '
  s/'${DONOR}${typename}'/'${classname}'/g;
  s/'${DONOR_LC}'/'${MODULE_LC}'/g;
  s/'${DONOR}'/'${MODULE}'/g;
  ' ${DONOR}'/src/'${DONOR}${typename}'.cxx' >$modulename'/src/'$classname'.cxx'

  # the sources are on the same line as the PRIVATE
  inplace_sed '/target_sources(O2Qc'$modulename' PRIVATE/ s_PRIVATE_PRIVATE src/'$classname'.cxx _' $modulename/CMakeLists.txt

  # the PRIVATE is on its own line (because there are more sources than
  # what fits on a single line)
  inplace_sed '/target_sources(O2Qc'$modulename'$/ {
  N
  /PRIVATE/ {
    s_PRIVATE_PRIVATE src/'$classname'.cxx _
  }
  }' $modulename/CMakeLists.txt

  echo '> '$typename' created.'

  cmake_format $modulename/CMakeLists.txt
}

function print_usage() {
  echo "Usage: ./o2-qc-module-configurator.sh -m MODULE_NAME [OPTION]

Generate template QC module and/or tasks, checks, aggregators and postprocessing.
If a module with specified name already exists, new tasks, checks, aggregators and postprocessing are inserted to the existing module.
Please follow UpperCamelCase convention for modules', tasks' and checks' names.

Example:
# create new module and some task
./o2-qc-module-configurator.sh -m MyModule -t SuperTask
# add one task and two checks
./o2-qc-module-configurator.sh -m MyModule -t EvenBetterTask -c HistoUniformityCheck -c MeanTest

Options:
 -h               print this message
 -m MODULE_NAME   create a module named MODULE_NAME or add there some task/checker
 -t TASK_NAME     create a task named TASK_NAME
 -c CHECK_NAME    create a check named CHECK_NAME
 -p PP_NAME       create a postprocessing task named PP_NAME
 -a AGG_NAME      create an aggregator named AGG_NAME
"
}

MODULE=
while getopts 'hm:t:c:p:a:' option; do
  case "${option}" in
  \?)
    print_usage
    exit 1
    ;;
  h)
    print_usage
    exit 0
    ;;
  m)
    check_pwd
    create_module ${OPTARG}
    MODULE=${OPTARG}
    ;;
  t)
    if [ -z ${MODULE} ]; then
      echo 'Cannot add a task, module name not specified, exiting...'
      exit 1
    fi
    create_class ${MODULE} ${OPTARG} Task
    ;;
  c)
    if [ -z ${MODULE} ]; then
      echo 'Cannot add a check, module name not specified, exiting...'
      exit 1
    fi
    create_class ${MODULE} ${OPTARG} Check
    ;;
  p)
    if [ -z ${MODULE} ]; then
      echo 'Cannot add a postprocessing task, module name not specified, exiting...'
      exit 1
    fi
    create_class ${MODULE} ${OPTARG} PostProcessing
    ;;
  a)
    if [ -z ${MODULE} ]; then
      echo 'Cannot add an aggregator, module name not specified, exiting...'
      exit 1
    fi
    create_class ${MODULE} ${OPTARG} Aggregator
    ;;
  esac
done

# If no options are specified
if [ ${OPTIND} -eq 1 ]; then
  print_usage
fi
