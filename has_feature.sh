#!/bin/sh
tmpfile=$$.c
mkdir -p .test

echo "#ifndef FEATURES_H"
echo "#define FEATURES_H"

for i in features/*.feature;do
	cp "$i" .test/"$tmpfile"
	cd .test

	if ${CC:-c99} $LDFLAGS $CPPFLAGS $CFLAGS "$tmpfile" $LDLIBS 2>/dev/null 1>/dev/null;then
		echo "#define HAVE_`basename "$i" .feature | tr a-z A-Z` 1"
	fi
	rm -f "$tmpfile" "${tmpfile%.*}.o" "${tmpfile%.*}"
	cd "$OLDPWD"
done

echo "#endif"
