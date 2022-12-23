#!/bin/sh -
# This script runs the .ed scripts generated by mkscripts.sh
# and compares their output against the .r files, which contain
# the correct output

PATH="/bin:/usr/bin:/usr/local/bin/:."
[ X"$ED" = X ] && ED="../ed"

# Run the *-err.ed scripts first, since these don't generate output;
# rename then to *-err.ed~; they exit with non-zero status
for i in *-err.ed; do
	echo $i~
	if $i; then
		echo "*** The script $i~ exited abnormally  ***"
	fi
	mv $i $i~
done >errs.o 2>&1

# Run the remainding scripts; they exit with zero status
for i in *.ed; do
	base=`echo $i | sed 's/\..*//'`
#	base=`$ED - <<-EOF
#	r !echo $i
#	s/\..*
#	EOF`
	if $base.ed; then
		if cmp -s $base.o $base.r; then :; else
			echo "*** Output $base.o of script $i is incorrect ***"
		fi
	else
		echo "*** The script $i exited abnormally ***"
	fi
done >scripts.o 2>&1

grep -h '\*\*\*' errs.o scripts.o
