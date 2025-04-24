#!/bin/bash

# Default values
PREFIX="o2/"
TMP_DIR="./consul_kv_backups"
mkdir -p "$TMP_DIR"
UPDATE=false

# --- Parse arguments ---
if [[ "$#" -eq 0 ]]; then
  echo "Use -h or --help for usage"
  exit 0
fi

while [[ "$#" -gt 0 ]]; do
  case "$1" in
  -u | --update)
    UPDATE=true
    ;;
  -p | --prefix)
    PREFIX="$2"
    shift
    ;;
  -h | --help)
    echo "Script to change any occurrence of \"PROTON-PROTON\" -> \"pp\", \"Pb-PROTON\" -> \"pPb\", \"Pb-Pb\" -> \"PbPb\""
    echo "Usage: $0 [-p|--prefix <consul_prefix>] [-u|--update]"
    echo
    echo "  -p, --prefix   Prefix to search in Consul KV (default: o2/)"
    echo "  -u, --update   Apply changes back to Consul"
    exit 0
    ;;
  *)
    echo "❌ Unknown option: $1"
    echo "Use -h or --help for usage"
    exit 1
    ;;
  esac
  shift
done

echo "🔍 Using Consul prefix: $PREFIX"
[[ "$UPDATE" == true ]] && echo "🚀 Update mode: ON (values will be written back)" || echo "🔒 Dry-run mode: changes only printed/saved"

# --- Define replacement logic ---
replace() {
  sed -e 's/\"PROTON-PROTON\"/"pp"/g' \
    -e 's/\"Pb-PROTON\"/"pPb"/g' \
    -e 's/\"Pb-Pb\"/"PbPb"/g'
}

# --- Fetch all keys ---
KEYS=$(consul kv get -recurse -keys "$PREFIX")

# --- Process each key ---
while IFS= read -r key; do
  VALUE=$(consul kv get "$key")
  MODIFIED=$(echo "$VALUE" | replace)

  if [[ "$VALUE" != "$MODIFIED" ]]; then
    SAFE_NAME=$(echo "$key" | sed 's|/|__|g')

    ORIG_FILE="$TMP_DIR/$SAFE_NAME.orig"
    NEW_FILE="$TMP_DIR/$SAFE_NAME.new"
    DIFF_FILE="$TMP_DIR/$SAFE_NAME.diff"

    echo "$VALUE" >"$ORIG_FILE"
    echo "$MODIFIED" >"$NEW_FILE"
    diff -u "$ORIG_FILE" "$NEW_FILE" >"$DIFF_FILE"

    echo "✅ Changed key: $key"
    echo "  📄 $ORIG_FILE"
    echo "  🆕 $NEW_FILE"
    echo "  📑 $DIFF_FILE"

    if [[ "$UPDATE" == true ]]; then
      echo "$MODIFIED" | consul kv put "$key" -
      echo "  🔁 Updated in Consul: $key"
    fi

    echo "---------------------------------------"
  fi
done <<<"$KEYS"
