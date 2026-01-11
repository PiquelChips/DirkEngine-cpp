check() {
  local missing_count=0

  for cmd in "$@"; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
      echo "Error: '$cmd' is not installed."
      missing_count=$((missing_count + 1))
    else
      echo "$cmd is installed."
    fi
  done

  return $missing_count
}

check "make" "g++" "go"
