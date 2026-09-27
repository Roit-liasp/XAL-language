#!/bin/sh

OUT="${1:-build}"
fail=0

for n in xb_child xb_exit xb_pipe xb_parent; do
	if ! "$OUT/xalc" -o "$OUT/$n.xb" "tests/$n.xa"; then
		echo "FAIL compile tests/$n.xa"
		exit 1
	fi
done

cmp_out() {
	if [ "$2" = "$3" ]; then
		echo "ok   $1"
	else
		echo "FAIL $1"
		echo "--- expected"; echo "$2"
		echo "--- got"; echo "$3"
		fail=1
	fi
}

got=$("$OUT/xvm" "$OUT/xb_parent.xb" "$OUT/xb_child.xb" "$OUT/xb_exit.xb" "$OUT/xb_pipe.xb" 2>&1)
want='child argv0=child-name
child argv1=one
child argv2=two
child done
run_xb rc=42
child argv0=second
child done
second rc=42
child exiting
exit rc=7
parent alive
pipe ok
pipe rc=0'
cmp_out "run_xb: nested launch (argv, exit code, exit() in child)" "$want" "$got"

got=$(XVM_CAPS=-pipe "$OUT/xvm" "$OUT/xb_parent.xb" "$OUT/xb_child.xb" "$OUT/xb_exit.xb" "$OUT/xb_pipe.xb" 2>&1)
want='child argv0=child-name
child argv1=one
child argv2=two
child done
run_xb rc=42
child argv0=second
child done
second rc=42
child exiting
exit rc=7
parent alive
xvm: cannot run image: unsupported here: pipe
pipe rc=126'
cmp_out "run_xb: child refused when host lacks a capability" "$want" "$got"

got=$(XVM_CAPS=-xb "$OUT/xvm" "$OUT/xb_parent.xb" "$OUT/xb_child.xb" 2>&1; echo "rc=$?")
want='xvm: cannot run image: unsupported here: xb
rc=126'
cmp_out "caps: image refused at startup" "$want" "$got"

got=$(XVM_CAPS_STRICT=0 XVM_CAPS=-pipe "$OUT/xvm" "$OUT/xb_pipe.xb" 2>&1; echo "rc=$?")
want='xvm: warning: image needs unsupported features: pipe
xvm: pipe: not supported by this host
pipe unavailable
rc=0'
cmp_out "caps: non-strict mode warns and runs" "$want" "$got"

got=$("$OUT/xvm" --caps "$OUT/xb_pipe.xb" 2>&1; echo "rc=$?")
case "$got" in
	*"missing: pipe"*|*"ok: can run here"*) echo "ok   xvm --caps reports needs and host caps" ;;
	*) echo "FAIL xvm --caps"; echo "$got"; fail=1 ;;
esac

exit $fail
