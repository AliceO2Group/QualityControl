#!/usr/bin/env bash
set -e ;# exit on error
set -u ;# exit when using undeclared variable
set -x

# Parameters
if [ "$#" -ne 1 ]; then
    echo "A single parameter must be passed: the version to patch. $# provided."
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
read -p "Do we continue ? [y|n]" -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    exit 3
fi


# checkout last tagged version
cd /tmp
echo "cloning in /tmp/QualityControl if not already there"
if [[ ! -d "/tmp/QualityControl" ]];
then
  git clone https://github.com/AliceO2Group/QualityControl.git
fi
cd QualityControl
read -p "Github user ? " user
git remote rename origin upstream
git remote add origin https://github.com/user/QualityControl.git


echo "branch"
git checkout $currentVersion
git checkout -b branch_$patchVersion


echo "cherry-pick the commit from master"
read -p "Hash to cherry-pick? " hash
echo
git cherry-pick $hash


echo "change version in CMakeLists and commit"
patchVersionNumbers="${patchVersion:1}"
echo "patchVersionNumbers: $patchVersionNumbers"
case "$(uname -s)" in
   Darwin)
     sed -E -i '' 's/VERSION [0-9]+\.[0-9]+\.[0-9]+/VERSION '"${patchVersionNumbers}"'/g' CMakeLists.txt
     ;;

   Linux)
     sed -r -i 's/VERSION [0-9]+\.[0-9]+\.[0-9]+/VERSION '"${patchVersionNumbers}"'/g' CMakeLists.txt
     ;;

    *)
     echo 'Unknown OS'
     exit 4
     ;;
esac
git add CMakeLists.txt
git commit -m "$patchVersion"


echo "push the branch upstream"
git push upstream -u branch_$patchVersion


echo "tag"
git tag -a $patchVersion -m "$patchVersion"


echo "push the tag upstream"
git push upstream $patchVersion

echo "Go to Github https://github.com/AliceO2Group/QualityControl/releases/new?tag=v${newVersion}&title=v${newVersion}"

read -p "Fill in the release notes and create the new release in GitHub" -n 1 -r
echo

read -p "A PR is automatically made in alidist, go check here: https://github.com/alisw/alidist/pulls"

echo "We are now done."