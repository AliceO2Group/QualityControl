# This script updates the qc nodes references in the config files and replace them with the staging qc nodes.

# Usage: ./updateStagingNodes.sh  # run but do not upload. See what changes are done in the local backup folder. 
# Usage: ./updateStagingNodes.sh -x # debug mode
# Usage: ./updateStagingNodes.sh -u # upload changes

updateFiles()
{
  # Replaces first argument by second argument in all the files in the current directory.
  replaceThis="$1"
  byThis="$2"
  filesChanged=$(grep -l "$replaceThis" ./*)
  if [ ! -z "$filesChanged" ]; then
    echo "
updated files for ($replaceThis --> $byThis) :
$filesChanged"
    sed -i "s/$replaceThis/$byThis/g" ./*
  else
    echo "
no files to update for ($replaceThis --> $byThis)"
  fi
}

while getopts xu flag
do
    case "${flag}" in
        u) upload=true;;
        x) echo "debug mode" && set -x;;
        *) echo "invalid flag"
    esac
done
echo "Upload enabled: $upload";

staging_consul_node="ali-staging-consul:8500"
echo "staging_consul_node: staging_consul_node"
export CONSUL_HTTP_ADDR=${staging_consul_node}

# Get the list of config files for qc
list_files=$(curl -s ${CONSUL_HTTP_ADDR}/v1/kv/o2/components/qc/ANY/any?keys=true | jq -c -r '.[]')
IFS=$'\n' read -rd '' -a array_files <<<"$list_files"

# backup folder
backup_dir_name="backup-consul-$(date +%Y.%m.%d)"
mkdir "$backup_dir_name"
cd $backup_dir_name

# download all files
for file in "${array_files[@]}"; do
  echo "Downloading $file"
  local_file=$(basename "$file")
  consul kv get "$file" >"$local_file"
done

# modify all files with proper qc nodes
echo "Apply the modifications"
updateFiles "alio2-cr1-qts01" "alio2-cr1-qc04"
updateFiles "alio2-cr1-qts02" "alio2-cr1-qc04"
updateFiles "alio2-cr1-qts03" "alio2-cr1-qc04"
updateFiles "alio2-cr1-qc01" "alio2-cr1-qc06"
updateFiles "alio2-cr1-qc02" "alio2-cr1-qc04"
updateFiles "alio2-cr1-qc03" "alio2-cr1-qc06"
updateFiles "alio2-cr1-qc05" "alio2-cr1-qc04"
updateFiles "alio2-cr1-qc07" "alio2-cr1-qc06"
updateFiles "alio2-cr1-qc08" "alio2-cr1-qc04"
updateFiles "alio2-cr1-qc09" "alio2-cr1-qc06"
updateFiles "alio2-cr1-qc10" "alio2-cr1-qc04"
updateFiles "alio2-cr1-qc11" "alio2-cr1-qc06"
updateFiles "alio2-cr1-qc12" "alio2-cr1-qc04"
updateFiles "alio2-cr1-qme01" "alio2-cr1-qc06"
updateFiles "alio2-cr1-qme02" "alio2-cr1-qc04"
updateFiles "alio2-cr1-qme03" "alio2-cr1-qc06"
updateFiles "alio2-cr1-qme04" "alio2-cr1-qc04"
updateFiles "alio2-cr1-qme05" "alio2-cr1-qc06"
updateFiles "alio2-cr1-qme06" "alio2-cr1-qc04"
updateFiles "alio2-cr1-qme07" "alio2-cr1-qc06"
updateFiles "alio2-cr1-qme08" "alio2-cr1-qc04"
updateFiles "alio2-cr1-qme09" "alio2-cr1-qc06"

# reupload files
if [[ $upload ]]; then
  echo "upload enabled"
  for file in "${array_files[@]}"; do
    local_file=$(basename "$file")
    content=$(cat $local_file)

    echo "uploading $file"
    consul kv put "$file" "$content"
  done
else
  echo "upload disabled"
fi
