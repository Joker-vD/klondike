#!/bin/bash -e

set -o pipefail

DECK=QDJH6S6H4C3SAS10S4DQH5HJC8C2S5C10DKSAH7S10H9D9CKC7C3D8DACJS2H5D5SADKH2D10C3H7H8SJDQC3CKD7D6C6D8H4S2C9S4HQS9H
OUTPUT=$(bin/klondike --deck "$DECK" <test.log | tail -n1)
EXPECTED='You won!'

if [ "$?" != 0 ] ; then exit ; fi

if [ "$EXPECTED" != "$OUTPUT" ] ; then
    >&2 printf 'Expected: %q\n' "$EXPECTED"
    >&2 printf '  Actual: %q\n' "$OUTPUT"
    exit 1 ;
fi
