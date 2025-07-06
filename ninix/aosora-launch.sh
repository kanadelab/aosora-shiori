#!/bin/sh
AOSORA_DEBUG_BOOTSTRAP=1 ninix --ghost "$2" --exit-if-not-found
exit $?
