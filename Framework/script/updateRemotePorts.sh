# This script updates the ""remotePort" values in QC config files according to the mapping in remotePortMapping.csv
# The mapping is between className and remotePort. Thus, there are a few exceptions, where the same class is used
# for different tasks. The corresponding corrections are included in this script.
# It is ad hoc and specific but it can easily be modified for similar purpose.
#
# KNOWN ISSUES:
# - for very large configuration files, the consul upload fails

#set -x

HEAD_NODES=(
#  alio2-cr1-flp162
#  alio2-cr1-flp146
#  alio2-cr1-flp160
#  alio2-cr1-flp187
#  alio2-cr1-flp148
#  alio2-cr1-flp182
#  alio2-cr1-flp159
#  alio2-cr1-flp164
#  alio2-cr1-flp178
#  alio2-cr1-hv-head01
#  alio2-cr1-flp166
#  alio2-cr1-flp181
#  alio2-cr1-mvs03
#  barth-test-cc7.cern.ch
#ali-consul.cern.ch
#  ali-staging-consul.cern.ch
)
echo "Number of nodes: ${#HEAD_NODES[@]}"

# Check that we have `jq`
if ! command -v jq &> /dev/null
then
    echo "jq could not be found, please install it."
    exit
fi

# read csv file into an associative array
declare -A classPortMap
while IFS=, read -r class port
do
    classPortMap["$class"]="$port"
done < remotePortMapping.csv

# for each node
for ((nodeIndex = 0; nodeIndex < ${#HEAD_NODES[@]}; nodeIndex++)); do

  node=${HEAD_NODES[${nodeIndex}]}
  echo "node: $node"
  export CONSUL_HTTP_ADDR=${node}:8500
  echo $CONSUL_HTTP_ADDR

  # Get the list of config files for qc
  list_files=$(curl -s ${node}:8500/v1/kv/o2/components/qc/ANY/any?keys=true | jq -c -r '.[]')
  IFS=$'\n' read -rd '' -a array_files <<<"$list_files"

  # backup folder
  backup_dir_name="backup-consul-`date +%Y.%m.%d`"
  mkdir $backup_dir_name
  cd $backup_dir_name

  # for each file
  for file in "${array_files[@]}"; do
    echo "file: $file"

    # download
    local_file=$(basename $file)
    consul kv get "$file" >$local_file

    for key in "${!classPortMap[@]}"; do
      className="$key"
      remotePort="${classPortMap[$key]}"
      jq --arg className "$className" --arg remotePort "$remotePort" \
      '(.qc.tasks[]? | select(.className == $className and has("remotePort")?) .remotePort) = $remotePort' \
      $local_file > temp && mv temp $local_file
    done
    # special cases
    jq '(if .qc.tasks.MCHFRofs | has("remotePort") then .qc.tasks.MCHFRofs.remotePort = "29512" else . end)' $local_file > temp && mv temp $local_file
    jq '(if .qc.tasks.MergeMETOFwTRD | has("remotePort") then .qc.tasks.MergeMETOFwTRD.remotePort = "29753" else . end)' $local_file > temp && mv temp $local_file
    jq '(if .qc.tasks.ExpertPedestalsOnFLP | has("remotePort") then .qc.tasks.ExpertPedestalsOnFLP.remotePort = "29002" else . end)' $local_file > temp && mv temp $local_file

    # or simply modify :
    #new_content=$(sed 's/http:\/\/localhost:8084/o2-ccdb.internal/g' $local_file)
    new_content=$(cat $local_file)
    # echo "new_content: $new_content"
    echo local_file: $local_file
    echo new_content: $new_content
    # upload (uncomment)
    # consul kv put "$file" "$new_content"
  done
done

