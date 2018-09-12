#!/usr/bin/env bash
set -e ;# exit on error
set -u ;# exit when using undeclared variable
#set -x ;# debugging

DONOR=SkeletonDPL
DONOR_LC=skeleton_dpl
DONOR_TASK=SkeletonTaskDPL
DONOR_TASK_INCLUDE_GUARD=QC_MODULE_`echo ${DONOR} | tr a-z A-Z`_`echo ${DONOR_TASK} | tr a-z A-Z`_H
DONOR_CHECK=SkeletonCheckDPL
DONOR_CHECK_INCLUDE_GUARD=QC_MODULE_`echo ${DONOR} | tr a-z A-Z`_`echo ${DONOR_CHECK} | tr a-z A-Z`_H

# Checks if current pwd matches the location of this script, exits if it is not
function check_pwd {
  DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
  if [[ $DIR != $PWD ]] ; then
    echo 'Please execute this script while being in its directory'
    exit 1
  fi
}

# Create new empty module
# \param 1 : module_name
function create_module {
  if [ -d $1 ] ; then
    echo 'Module '$1' already exists.'
  else
    echo 'Module '$1' does not exist, generating...'
    if [ ! -d ${DONOR} ] ; then
      echo '> Donor template '${DONOR}' does not exist, exiting...'
      exit 1
    fi

    MODULE_LC=`echo $1 | tr A-Z a-z`
    # prepare folder structure
    mkdir $1 $1/src $1/include/ $1/include/$1 $1/test
    
    # prepare CMakeLists.txt
    sed 's/'${DONOR}'/'$1'/' ${DONOR}/.CMakeListsEmpty.txt > $1/CMakeLists.txt
    # prepare LinkDef.h
    sed '/#pragma link C++ class o2::quality_control_modules::'${DONOR_LC}'::/ d' ${DONOR}/include/${DONOR}/LinkDef.h > $1/include/$1/LinkDef.h
    # prepare test
    sed 's/.testEmpty/test'$1'/; s/'${DONOR_LC}'/'${MODULE_LC}'/' ${DONOR}/test/.testEmpty.cxx > $1'/test/test'$1'.cxx'
    sed -i '/set(TEST_SRCS/ a \ \ test/test'$1'.cxx' $1/CMakeLists.txt

    # add new module to the project
    echo 'add_subdirectory('$1')' >> CMakeLists.txt
    
    echo '> Module created.'
  fi
}

# Create new task in specified module
# \param 1 : module_name
# \param 2 : task_name
function create_task {
  echo 'Creating task '$2' in module '$1'.'
  if [ -f $1'/include/'$1'/'$2'.h' ] ; then
    echo '> Task '$2' already exists, returning...'
    return
  fi
  
  MODULE_LC=`echo $1 | tr A-Z a-z`
  INCLUDE_GUARD_NAME=QC_MODULE_`echo $1 | tr a-z A-Z`_`echo $2 | tr a-z A-Z`_H
  
  # add header
  sed 's/'${DONOR_TASK}'/'$2'/g; s/'${DONOR_LC}'/'${MODULE_LC}'/g; s/'${DONOR}'/'${MODULE}'/g; s/'${DONOR_TASK_INCLUDE_GUARD}'/'${INCLUDE_GUARD_NAME}'/g' ${DONOR}'/include/'${DONOR}'/'${DONOR_TASK}'.h' > $1'/include/'$1'/'$2'.h' 
  sed -i '/#endif/ i #pragma link C++ class o2::quality_control_modules::'${MODULE_LC}'::'$2'+;' $1/include/$1/LinkDef.h
  sed -i '/set(HEADERS/ a \ \ include/'$1'/'$2'.h' $1/CMakeLists.txt

  # add src
  sed 's/'${DONOR_TASK}'/'$2'/g; s/'${DONOR_LC}'/'${MODULE_LC}'/g; s/'${DONOR}'/'${MODULE}'/g' ${DONOR}'/src/'${DONOR_TASK}'.cxx' > $1'/src/'$2'.cxx' 
  sed -i '/set(SRCS/ a \ \ src/'$2'.cxx' $1/CMakeLists.txt
  echo '> Task created.'
}

# Create new check in specified module
# \param 1 : module_name
# \param 2 : check_name
function create_check {
  echo 'Creating check '$2' in module '$1'.'
  
  if [ -f $1'/include/'$1'/'$2'.h' ] ; then
    echo '> Check '$2' already exists, returning...'
    return
  fi
  
  MODULE_LC=`echo $1 | tr A-Z a-z`
  INCLUDE_GUARD_NAME=QC_MODULE_`echo $1 | tr a-z A-Z`_`echo $2 | tr a-z A-Z`_H
  
  # add header
  sed 's/'${DONOR_CHECK}'/'$2'/g; s/'${DONOR_LC}'/'${MODULE_LC}'/g; s/'${DONOR}'/'${MODULE}'/g; s/'${DONOR_CHECK_INCLUDE_GUARD}'/'${INCLUDE_GUARD_NAME}'/g' ${DONOR}'/include/'${DONOR}'/'${DONOR_CHECK}'.h' > $1'/include/'$1'/'$2'.h' 
  sed -i '/#endif/ i #pragma link C++ class o2::quality_control_modules::'${MODULE_LC}'::'$2'+;' $1/include/$1/LinkDef.h
  sed -i '/set(HEADERS/ a \ \ include/'$1'/'$2'.h' $1/CMakeLists.txt

  # add src
  sed 's/'${DONOR_CHECK}'/'$2'/g; s/'${DONOR_LC}'/'${MODULE_LC}'/g; s/'${DONOR}'/'${MODULE}'/g' ${DONOR}'/src/'${DONOR_CHECK}'.cxx' > $1'/src/'$2'.cxx' 
  sed -i '/set(SRCS/ a \ \ src/'$2'.cxx' $1/CMakeLists.txt
  echo '> Check created.'
}

function print_usage {
  echo "Usage: ./modulesHelper.sh -m MODULE_NAME [OPTION]

Generate template QC module and/or tasks, checks.
If a module with specified name already exists, new tasks and checks are inserted to the existing one.
Please follow UpperCamelCase convention for modules', tasks' and checks' names. 

Example:
# create new module and some task
./modulesHelper.sh -m MyModule -t SuperTask
# add one task and two checks
./modulesHelper.sh -m MyModule -t EvenBetterTask -c HistoUniformityCheck -c MeanTest

Options:
 -h               print this message
 -m MODULE_NAME   create module named MODULE_NAME or add there some task/checker
 -t TASK_NAME     create task named TASK_NAME
 -c CHECK_NAME    create check named CHECK_NAME
"
}

MODULE=
while getopts 'hm:t:c:' option; do
  case "${option}" in
    \?) print_usage
       exit 1;;
    h) print_usage
       exit 0;;
    m) check_pwd
       create_module ${OPTARG}
       MODULE=${OPTARG};;
    t) if [ -z ${MODULE} ] ; then
         echo 'Cannot add a task, module name not specified, exiting...'
         exit 1
       fi
       create_task ${MODULE} ${OPTARG};;
    c) if [ -z ${MODULE} ] ; then
         echo 'Cannot add a check, module name not specified, exiting...'
         exit 1
       fi
       create_check ${MODULE} ${OPTARG};;
  esac
done

# If no options are specified
if [ ${OPTIND} -eq 1 ]; then
  print_usage
fi