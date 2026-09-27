#!/bin/sh

in="$1"; out="$2"
{
	echo "const unsigned char prog_xb[] = {"
	od -An -v -tu1 "$in" | tr -s ' ' '\n' | grep -v '^$' | sed 's/$/,/'
	echo "};"
	echo "const unsigned long prog_xb_len = sizeof prog_xb;"
} > "$out"
