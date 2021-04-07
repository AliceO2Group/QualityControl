#!/usr/bin/env bash
set -e ;# exit on error
set -u ;# exit when using undeclared variable


# Parameter: if there we check that it is a proper version number, if not we propose a new one
if [ "$#" -gt 0 ]; then
    newVersion=$1
    if [[ ! $newVersion =~ ^[0-9]+(\.[0-9]+){2,2}$ ]];
    then
        echo "The version number must be provided in the form x.y.z"
        exit 1
    fi
else
    # create the patch version: take the current version x.y.z and do y+=1 and z=0
    echo "No version number provided, we use the CMakeLists to build one."
    oldVersion=$(awk '/^ *VERSION/{print $2}' CMakeLists.txt)
    delimiter=.
    array=($(echo "$oldVersion" | tr $delimiter '\n'))
    array[1]=$((array[1]+1))
    array[2]=0
    newVersion=$(IFS=$delimiter ; echo "${array[*]}")
fi
echo "Version to release: $newVersion";


#Get the script dir
fullScriptName=$0
scriptName=`basename $0`
scriptDir=`echo $fullScriptName | sed 's:'/$scriptName'::'`


# check
read -p "Do we continue ? (y|n)" -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    exit 3
fi


echo "Cloning in /tmp/QualityControl"
cd /tmp
if [[ ! -d "/tmp/QualityControl" ]];
then
  git clone https://github.com/AliceO2Group/QualityControl.git
else
  echo "/tmp/QualityControl already exists, please [re]move it"
  exit 5
fi
cd QualityControl


echo "Update version number in CMakeLists"
case "$(uname -s)" in
   Darwin)
     sed -E -i '' 's/VERSION [0-9]+\.[0-9]+\.[0-9]+/VERSION '"${newVersion}"'/g' CMakeLists.txt
     ;;

   Linux)
     sed -r -i 's/VERSION [0-9]+\.[0-9]+\.[0-9]+/VERSION '"${newVersion}"'/g' CMakeLists.txt
     ;;

    *)
     echo 'Unknown OS'
     exit 4
     ;;
esac
echo "Commit it to master"
git add CMakeLists.txt
git commit -m "${newVersion}"


read -p "Ready to push, please confirm [y|n]" -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    exit 3
fi
#git push


read -p "Go to JIRA and release this version : https://alice.its.cern.ch/jira/projects/QC?selectedItem=com.atlassian.jira.jira-projects-plugin%3Arelease-page&status=released-unreleased
Press any key to continue" -n 1 -r
echo


echo "Go to Github https://github.com/AliceO2Group/QualityControl/releases/new?tag=v${newVersion}&title=v${newVersion}&body=$(cat ${scriptDir}/../../doc/ReleaseNotesTemplate.md |jq -sRr @uri)"


read -p "Fill in the release notes and create the new release in GitHub" -n 1 -r
echo


read -p "A PR is automatically made in alidist, go check here: https://github.com/alisw/alidist/pulls"


echo "We are now done.
Once the PR is merged, send an email to alice-o2-wp7@cern.ch, alice-o2-qc-contact@cern.ch and alice-dpg-qa-tools@cern.ch to announce the new release. Use the email from the previous release as a template."
