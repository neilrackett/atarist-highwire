#!/bin/sh -eu
# Stage the archive that "make dist" just built under the names the release
# carries.  Two of them: a versioned one so the release history is readable,
# and a constant one so that
#   .../releases/latest/download/highwire-latest.zip
# keeps working -- that URL resolves by file name, so the name must not change
# between releases.

BUILT="hw`date +%y%m%d`.zip"
OUT="${DEPLOY_DIR:-/tmp/highwire-deploy}"

if [ ! -f "$BUILT" ]; then
	echo "package.sh: $BUILT not found -- did make dist run?" >&2
	exit 1
fi

mkdir -p "$OUT"
cp "$BUILT" "$OUT/${PROJECT_NAME}-${PROJECT_VERSION}-${LONG_ID}.zip"
cp "$BUILT" "$OUT/${PROJECT_NAME}-latest.zip"
ls -l "$OUT"
