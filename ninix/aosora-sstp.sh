#!/bin/sh

export SAORI_FALLBACK_ALWAYS=1
export SAORI_FALLBACK_PATH=/opt/ninix-kagari/lib/saori

exec aosora-sstp $@
