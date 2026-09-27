#!/bin/sh

OUT="${1:-build}"
fail=0

if [ ! -x /bin/sh ]; then
	echo "skip proc tests (no /bin/sh)"
	exit 0
fi

if ! "$OUT/xalc" -o "$OUT/proc.xb" tests/proc.xa; then
	echo "FAIL compile tests/proc.xa"
	exit 1
fi

got=$("$OUT/xvm" "$OUT/proc.xb" 2>&1)
echo "$got"
case "$got" in
	*BAD*) echo "FAIL proc: see BAD lines above"; fail=1 ;;
	*) echo "ok   processes (spawn/wait/waitpid/kill/pipe/dup2)" ;;
esac

exit $fail
