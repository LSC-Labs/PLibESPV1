#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

DOXYGEN_BIN="${DOXYGEN_BIN:-doxygen}"
DOXYFILE="${PROJECT_ROOT}/doc/runtime/Doxyfile"
XML_DIR="${PROJECT_ROOT}/doc/runtime/xml"
MARKDOWN_FILE="${PROJECT_ROOT}/doc/runtime/markdown/PLibESPV1_Runtime_Library.md"
PDF_FILE="${PROJECT_ROOT}/doc/runtime/pdf/PLibESPV1_Runtime_Library.pdf"
DOC_CONVERTER="${PROJECT_ROOT}/doc/runtime/tools/doxygen_xml_to_docs.py"

if ! command -v "${DOXYGEN_BIN}" >/dev/null 2>&1; then
  echo "error: doxygen not found. Install Doxygen or set DOXYGEN_BIN=/path/to/doxygen." >&2
  exit 1
fi

if [[ ! -f "${DOXYFILE}" ]]; then
  echo "error: Doxyfile not found: ${DOXYFILE}" >&2
  exit 1
fi

if [[ ! -f "${DOC_CONVERTER}" ]]; then
  echo "error: Markdown/PDF converter not found: ${DOC_CONVERTER}" >&2
  exit 1
fi

cd "${PROJECT_ROOT}"

echo "Building Doxygen HTML/XML documentation..."
"${DOXYGEN_BIN}" "${DOXYFILE}"

echo "Building Markdown and PDF documentation..."
python3 "${DOC_CONVERTER}" \
  --xml "${XML_DIR}" \
  --markdown "${MARKDOWN_FILE}" \
  --pdf "${PDF_FILE}"

echo "Documentation generated:"
echo "  HTML:     ${PROJECT_ROOT}/doc/runtime/html/index.html"
echo "  Markdown: ${MARKDOWN_FILE}"
echo "  PDF:      ${PDF_FILE}"
