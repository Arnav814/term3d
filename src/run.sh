#!/usr/bin/env bash
./play3d "$@" 2>errors.log
STAT=$?
if [[ STAT -ne 0 ]]; then
	stty sane
fi
cat errors.log
exit $STAT

