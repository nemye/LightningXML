#!/usr/bin/env bash
set -euo pipefail

CLANG_FORMAT=${CLANG_FORMAT:-clang-format}

files=$(git diff --name-only HEAD -- '*.cc' '*.hh' '*.cpp' '*.h' '*.hpp' 2>/dev/null)

if [[ -z "$files" ]]; then
  echo "No changed C/C++ files to format."
  exit 0
fi

echo "$files" | xargs "$CLANG_FORMAT" --style=file -i
echo "Formatted:"
echo "$files" | sed 's/^/  /'
