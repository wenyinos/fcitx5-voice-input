#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="dist/aur"

# Skip if prepare.sh didn't produce files (non-tag push)
if [[ ! -f "${OUT_DIR}/PKGBUILD" ]]; then
  echo "No PKGBUILD found, skipping AUR build"
  echo "pkgfile=" >> "${GITHUB_OUTPUT}"
  exit 0
fi

PKG="fcitx5-voice-input"
WORK_DIR="dist/aur/pkg"

mkdir -p "${WORK_DIR}"

# Feed PKGBUILD + source tarball into build directory
cp "${OUT_DIR}/PKGBUILD" "${WORK_DIR}/"
cp "${OUT_DIR}"/src/*.tar.gz "${WORK_DIR}/"

# Build Docker image
docker build -t arch-builder "$(dirname "$0")"

# Run build (entrypoint runs makepkg inside /build)
# shellcheck disable=SC2086
docker run --rm -v "${PWD}/${WORK_DIR}:/build" arch-builder

# Extract result
PKGFILE=$(ls "${WORK_DIR}"/*.pkg.tar.zst 2>/dev/null | head -1)
if [ -n "${PKGFILE}" ]; then
  cp "${PKGFILE}" "${OUT_DIR}/"
  echo "pkgfile=${PKGFILE}" >> "${GITHUB_OUTPUT}"
else
  echo "pkgfile=" >> "${GITHUB_OUTPUT}"
fi
