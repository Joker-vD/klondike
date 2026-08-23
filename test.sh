#!/bin/bash -e

DECK=QDJH6S6H4C3SAS10S4DQH5HJC8C2S5C10DKSAH7S10H9D9CKC7C3D8DACJS2H5D5SADKH2D10C3H7H8SJDQC3CKD7D6C6D8H4S2C9S4HQS9H
EXPECTED='You won!'

do_test() {
    TEST_NAME=${1:-unnamed}
    shift || true
    OUTPUT=$(bin/klondike --deck "$DECK" "$@" <test.log)
    OUTPUT=$(tail -n 1 <<<"$OUTPUT")

    if [ "$EXPECTED" != "$OUTPUT" ] ; then
        >&2 printf 'Test "%s"\n' "$TEST_NAME"
        >&2 printf '\tExpected: %q\n' "$EXPECTED"
        >&2 printf '\t  Actual: %q\n' "$OUTPUT"
        return 1
    fi
}

ERRORS=

if ! do_test 'small cards' --small ; then ERRORS="${ERRORS}x" ; fi
if ! do_test 'normal cards' --normal ; then ERRORS="${ERRORS}x" ; fi

if [ -n "$ERRORS" ]
then
    exit 1
fi
