# This script updates the ccdb host in all config files in all head nodes.
# It is ad hoc and specific but it can easily be modified for similar purpose.

# set -x

HEAD_NODES=(
  ali-staging-consul.cern.ch
#ali-consul.cern.ch
)
echo "Number of nodes: ${#HEAD_NODES[@]}"

# Check that we have `jq`
if ! command -v jq &> /dev/null
then
    echo "jq could not be found, please install it."
    exit
fi

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
    # avoid touching the repo cleaner file 
    if [[ $file == *repoCleanerConfig.yaml* ]]; then
      echo "   it is the repo cleaner config file, we skip it"
      continue
    fi

    if [[ "$file" == */ ]]; then
        echo "   path finishes with / and is directory"
        continue
    fi

    # example how to use jq to do the job. 
    new_content=$(jq '
      if has("qc") and .qc?.config?.bookkeeping?.url == null then
          .qc.config.bookkeeping.url = "alio2-cr1-hv-mvs00.cern.ch:4001"
      else
        .
      end
    ' $local_file)
    if [ $? -eq 0 ]; then
        echo "jq succeeded"
        cat  $local_file
        echo $new_content | jq .
        #      consul kv put "$file" "$new_content"
    else
        echo "jq failed"
    fi

    # download
    local_file=$(basename $file)
    consul kv get "$file" >$local_file

    # if we need to check the value before modifying :
    current=$(cat $local_file | jq  '.qc.config.conditionDB.url')
    current=$(echo $current| tr -d '"') # remove quotes
    echo "current: $current"
#    unset new_content
#    if (( $current != null && $current < 11 )); then
#      # modify
#      new_content=$(cat $local_file | jq  '.qc.config.conditionDB.url |= "11"')
#      echo $new_content
#      consul kv put "$file" "$new_content"
#    fi

    # or simply modify :
    #new_content=$(sed 's/http:\/\/localhost:8084/o2-ccdb.internal/g' $local_file)
    new_content=$(cat $local_file | jq  '.qc.config.infologger.filterDiscardLevel |= "21"')
    echo "new_content: $new_content"
    # upload (uncomment)
#    consul kv put "$file" "$new_content"
  done
done
