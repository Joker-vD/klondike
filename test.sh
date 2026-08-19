#!/bin/bash -e

set -o pipefail

OUTPUT=$(bin/klondike <test.log | tail -n1)
EXPECTED=$'\aYou won!'

if [ "$?" != 0 ] ; then exit ; fi

if [ "$EXPECTED" != "$OUTPUT" ] ; then
    >&2 printf 'Expected: %q\n' "$EXPECTED"
    >&2 printf '  Actual: %q\n' "$OUTPUT"
    exit 1 ;
fi
