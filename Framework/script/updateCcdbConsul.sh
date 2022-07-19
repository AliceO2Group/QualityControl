# This script updates the ccdb host in all config files in all head nodes.
# It is ad hoc and specific but it can easily be modified for similar purpose.

# set -x

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
#  alio2-cr1-mvs01
  barth-test-cc7.cern.ch
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

  # for each file
  for file in "${array_files[@]}"; do
    echo "file: $file"
    # download
    consul kv get "$file" >/tmp/consul.json

    # if we need to check the value before modifying :
#    current=$(cat /tmp/consul.json | jq  '.qc.config.infologger.filterDiscardLevel')
#    current=$(echo $current| tr -d '"') # remove quotes
#    echo "current: $current"
#    unset new_content
#    if (( $current != null && $current < 11 )); then
#      # modify
#      new_content=$(cat /tmp/consul.json | jq  '.qc.config.infologger.filterDiscardLevel |= "11"')
#      echo $new_content
#      consul kv put "$file" "$new_content"
#    fi

    # or simply modify :
    new_content=$(cat /tmp/consul.json | jq  '.qc.config.infologger.filterDiscardLevel |= "21"')
    # upload (uncomment)
#    consul kv put "$file" "$new_content"
  done
done
