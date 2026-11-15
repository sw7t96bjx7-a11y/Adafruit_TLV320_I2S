#!/usr/bin/env bash
# Builds API documentation without authentication or a gh-pages branch.
set -euo pipefail
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
cd -- "$script_dir/.."
mkdir -p docs-build
cat > docs-build/Doxyfile <<'EOF'
PROJECT_NAME = "Adafruit TLV320 I2S Arduino Library"
OUTPUT_DIRECTORY = docs-build
INPUT = Adafruit_TLV320DAC3100.h Adafruit_TLV320DAC3100_typedefs.h Adafruit_TLV320DAC3100.cpp
RECURSIVE = NO
EXTRACT_ALL = YES
GENERATE_HTML = YES
GENERATE_LATEX = NO
QUIET = YES
WARN_IF_UNDOCUMENTED = NO
WARN_LOGFILE = docs-build/warnings.log
HAVE_DOT = NO
EOF
doxygen docs-build/Doxyfile
test -s docs-build/html/index.html
