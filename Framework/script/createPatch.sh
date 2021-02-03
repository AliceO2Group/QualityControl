#!/usr/bin/env bash
set -e ;# exit on error
set -u ;# exit when using undeclared variable


# Parameters
if [ "$#" -ne 1 ]; then
    echo "A single parameter must be passed. $# provided."
    exit 1
fi
currentVersion=$1
if [[ ! $currentVersion =~ ^v[0-9]+(\.[0-9]+){2,2}$ ]];
then
    echo "The parameter must be provided in the form vx.y.z"
    exit 2
fi
echo "Version to patch: $currentVersion";


# create the patch version: take the current version x.y.z and do z+1
delimiter=.
array=($(echo "$currentVersion" | tr $delimiter '\n'))
array[2]=$((array[2]+1))
patchVersion=$(IFS=$delimiter ; echo "${array[*]}")
echo "Patched version: $patchVersion";


# check
read -p "Are you sure? " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    exit 3
fi


# checkout last tagged version
cd /tmp
if [[ ! -d "/tmp/QualityControl" ]];
then
  git clone https://github.com/AliceO2Group/QualityControl.git
fi
cd QualityControl
git checkout $currentVersion


# branch
git checkout -b branch_$patchVersion


# cherry-pick the commit from master
read -p "Hash to cherry-pick? " hash
echo

git cherry-pick $hash

# change version in CMakeLists and commit
# push the branch upstream, e.g. git push upstream -u branch_v0.26.2
# tag, e.g. git tag -a v0.26.2 -m "v0.26.2"
# push the tag upstream, e.g. git push upstream v0.26.2
# Release in github using this tag