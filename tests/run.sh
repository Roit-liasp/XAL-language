#!/bin/sh

OUT="${1:-build}"
fail=0
for src in tests/t*.xa; do
	exp="${src%.xa}.expected"
	[ -f "$exp" ] || continue
	bin="$OUT/$(basename "${src%.xa}").xb"
	if ! "$OUT/xalc" -o "$bin" "$src"; then echo "FAIL compile $src"; fail=1; continue; fi
	got=$("$OUT/xvm" "$bin" 2>&1; echo "rc=$?")
	if [ "$got" = "$(cat "$exp")" ]; then echo "ok   $src"; else
		echo "FAIL $src"; echo "--- expected"; cat "$exp"; echo "--- got"; echo "$got"; fail=1; fi
done

head -c 40 "$OUT/t2_basic.xb" > "$OUT/trunc.xb"
if "$OUT/xvm" "$OUT/trunc.xb" 2>/dev/null; then echo "FAIL truncated image accepted"; fail=1; else echo "ok   truncated image rejected"; fi
exit $fail
