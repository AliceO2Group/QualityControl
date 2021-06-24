# This script updates the ccdb host in all config files in all head nodes.
# It is ad hoc and specific but it can easily be modified for similar purpose.

# set -x

HEAD_NODES=(
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
    # modify
    new_content=$(cat /tmp/consul.json | jq  '.qc.config.database.host |= "qcdb.cern.ch:8083"')
    # upload
    consul kv put "$file" "$new_content"
  done
done
