dnl From FIND_MOTIF and ACX_PTHREAD, without much understanding
dnl
dnl FIND_ZIP[ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]]
dnl ------------------------------------------------
dnl
dnl Find ZIP libraries and headers
dnl
dnl Put includes stuff in ZIP_INCLUDES
dnl Put link stuff in ZIP_LIBS
dnl
dnl Default ACTION-IF-FOUND defines HAVE_ZIP
dnl
AC_DEFUN([FIND_ZIP], [
AC_REQUIRE([AC_PATH_XTRA])

ZIP_INCLUDES=""
ZIP_LIBS=""

AC_ARG_WITH(zip, 
[  --without-zip			do not use libz])
# Treat --without-zip like --without-zip-includes --without-zip-libraries.
if test "$with_zip" = "no"; then
	ZIP_INCLUDES=no
	ZIP_LIBS=no
fi

AC_ARG_WITH(zip-includes,
[  --with-zip-includes=DIR	ZIP include files are in DIR],
ZIP_INCLUDES="-I$withval")
AC_ARG_WITH(zip-libraries,
[  --with-zip-libraries=DIR	ZIP libraries are in DIR],
ZIP_LIBS="-L$withval -lz")

AC_MSG_CHECKING(for ZIP)

# Look for zlib.h 
if test "$ZIP_INCLUDES" = ""; then
	zip_save_LIBS="$LIBS"

	LIBS="-lz $LIBS"

	# Check the standard search path
	AC_TRY_COMPILE([#include <zlib.h>],[int a;],[
		ZIP_INCLUDES=""
	], [
		# zlib.h is not in the standard search path.

		# A whole bunch of guesses
		for dir in \
			"${prefix}"/*/include /usr/*/include \
			/usr/local/*/include \
			"${prefix}"/include/* \
			/usr/include/* /usr/local/include/* /*/include; do
			if test -f "$dir/zlib.h"; then
				ZIP_INCLUDES="-I$dir"
				break
			fi
		done

		if test "$ZIP_INCLUDES" = ""; then
			ZIP_INCLUDES=no
		fi
	])

	LIBS="$zip_save_LIBS"
fi

# Now for the libraries
if test "$ZIP_LIBS" = ""; then
	zip_save_LIBS="$LIBS"
	zip_save_INCLUDES="$INCLUDES"

	LIBS="-lz $LIBS"
	INCLUDES="$ZIP_INCLUDES $INCLUDES"

	# Try the standard search path first
	AC_TRY_LINK([#include <zlib.h>],[zlibVersion()], [
		ZIP_LIBS="-lz"
	], [
		# libz is not in the standard search path.

		# A whole bunch of guesses
		for dir in \
			"${prefix}"/*/lib /usr/*/lib /usr/local/*/lib \
			"${prefix}"/lib/* /usr/lib/* \
			/usr/local/lib/* /*/lib; do
			if test -d "$dir" && test "`ls $dir/libz.* 2> /dev/null`" != ""; then
				ZIP_LIBS="-L$dir -lz"
				break
			fi
		done

		if test "$ZIP_LIBS" = ""; then
			ZIP_LIBS=no
		fi
	])

	LIBS="$zip_save_LIBS"
	INCLUDES="$zip_save_INCLUDES"
fi

AC_SUBST(ZIP_LIBS)
AC_SUBST(ZIP_INCLUDES)

# Print a helpful message
zip_libraries_result="$ZIP_LIBS"
zip_includes_result="$ZIP_INCLUDES"

if test x"$zip_libraries_result" = x""; then
	zip_libraries_result="in default path"
fi
if test x"$zip_includes_result" = x""; then
	zip_includes_result="in default path"
fi

if test "$zip_libraries_result" = "no"; then
	zip_libraries_result="(none)"
fi
if test "$zip_includes_result" = "no"; then
	zip_includes_result="(none)"
fi

AC_MSG_RESULT(
  [libraries $zip_libraries_result, headers $zip_includes_result])

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test "$ZIP_INCLUDES" != "no" && test "$ZIP_LIBS" != "no"; then
        ifelse([$1],,AC_DEFINE(HAVE_ZIP,1,[Define if you have libz libraries and header files.]),[$1])
        :
else
	ZIP_LIBS=""
	ZIP_INCLUDES=""
        $2
fi

])dnl

dnl From FIND_MOTIF and ACX_PTHREAD, without much understanding
dnl
dnl FIND_TIFF[ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]]
dnl ------------------------------------------------
dnl
dnl Find TIFF libraries and headers
dnl
dnl Put compile stuff in TIFF_INCLUDES
dnl Put link stuff in TIFF_LIBS
dnl
dnl Default ACTION-IF-FOUND defines HAVE_TIFF
dnl
AC_DEFUN([FIND_TIFF], [
AC_REQUIRE([AC_PATH_XTRA])

TIFF_INCLUDES=""
TIFF_LIBS=""

AC_ARG_WITH(tiff, 
[  --without-tiff		do not use libtiff])
# Treat --without-tiff like --without-tiff-includes --without-tiff-libraries.
if test "$with_tiff" = "no"; then
	TIFF_INCLUDES=no
	TIFF_LIBS=no
fi

AC_ARG_WITH(tiff-includes,
[  --with-tiff-includes=DIR	TIFF include files are in DIR],
TIFF_INCLUDES="-I$withval")
AC_ARG_WITH(tiff-libraries,
[  --with-tiff-libraries=DIR	TIFF libraries are in DIR],
TIFF_LIBS="-L$withval -ltiff")

AC_MSG_CHECKING(for TIFF)

# Look for tiff.h 
if test "$TIFF_INCLUDES" = ""; then
	# Check the standard search path
	AC_TRY_COMPILE([#include <tiff.h>],[int a;],[
		TIFF_INCLUDES=""
	], [
		# tiff.h is not in the standard search path.

		# A whole bunch of guesses
		for dir in \
			"${prefix}"/*/include /usr/*/include \
			/usr/local/*/include "${prefix}"/include/* \
			/usr/include/* /usr/local/include/* \
			/opt/include /opt/*/include /*/include; do
			if test -f "$dir/tiff.h"; then
				TIFF_INCLUDES="-I$dir"
				break
			fi
		done

		if test "$TIFF_INCLUDES" = ""; then
			TIFF_INCLUDES=no
		fi
	])
fi

# Now for the libraries
if test "$TIFF_LIBS" = ""; then
	tiff_save_LIBS="$LIBS"
	tiff_save_INCLUDES="$INCLUDES"

	LIBS="-ltiff -lm $LIBS"
	INCLUDES="$TIFF_INCLUDES $INCLUDES"

	# Try the standard search path first
	AC_TRY_LINK([#include <tiff.h>],[TIFFGetVersion();], [
		TIFF_LIBS="-ltiff"
	], [
		# libtiff is not in the standard search path.

		# A whole bunch of guesses
		for dir in \
			"${prefix}"/*/lib /usr/*/lib /usr/local/*/lib \
			"${prefix}"/lib/* /usr/lib/* \
			/usr/local/lib/* \
			/opt/lib /opt/*/lib /*/lib; do
			if test -d "$dir" && test "`ls $dir/libtiff.* 2> /dev/null`" != ""; then
				TIFF_LIBS="-L$dir -ltiff"
				break
			fi
		done

		if test "$TIFF_LIBS" = ""; then
			TIFF_LIBS=no
		fi
	])

	LIBS="$tiff_save_LIBS"
	INCLUDES="$tiff_save_INCLUDES"
fi

AC_SUBST(TIFF_LIBS)
AC_SUBST(TIFF_INCLUDES)

# Print a helpful message
tiff_libraries_result="$TIFF_LIBS"
tiff_includes_result="$TIFF_INCLUDES"

if test x"$tiff_libraries_result" = x""; then
	tiff_libraries_result="in default path"
fi
if test x"$tiff_includes_result" = x""; then
	tiff_includes_result="in default path"
fi

if test "$tiff_libraries_result" = "no"; then
	tiff_libraries_result="(none)"
fi
if test "$tiff_includes_result" = "no"; then
	tiff_includes_result="(none)"
fi

AC_MSG_RESULT(
  [libraries $tiff_libraries_result, headers $tiff_includes_result])

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test "$TIFF_INCLUDES" != "no" && test "$TIFF_LIBS" != "no"; then
        ifelse([$1],,AC_DEFINE(HAVE_TIFF,1,[Define if you have tiff libraries and header files.]),[$1])
        :
else
	TIFF_INCLUDES=""
	TIFF_LIBS=""
        $2
fi

])dnl

dnl From FIND_MOTIF and ACX_PTHREAD, without much understanding
dnl
dnl FIND_JPEG[ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]]
dnl ------------------------------------------------
dnl
dnl Find JPEG libraries and headers
dnl
dnl Put compile stuff in JPEG_INCLUDES
dnl Put link stuff in JPEG_LIBS
dnl
dnl Default ACTION-IF-FOUND defines HAVE_JPEG
dnl
AC_DEFUN([FIND_JPEG], [
AC_REQUIRE([AC_PATH_XTRA])

JPEG_INCLUDES=""
JPEG_LIBS=""

AC_ARG_WITH(jpeg, 
[  --without-jpeg		do not use libjpeg])
# Treat --without-jpeg like --without-jpeg-includes --without-jpeg-libraries.
if test "$with_jpeg" = "no"; then
	JPEG_INCLUDES=no
	JPEG_LIBS=no
fi

AC_ARG_WITH(jpeg-includes,
[  --with-jpeg-includes=DIR	JPEG include files are in DIR],
JPEG_INCLUDES="-I$withval")
AC_ARG_WITH(jpeg-libraries,
[  --with-jpeg-libraries=DIR	JPEG libraries are in DIR],
JPEG_LIBS="-L$withval -ljpeg")

AC_MSG_CHECKING(for JPEG)

# Look for jpeg.h 
if test "$JPEG_INCLUDES" = ""; then
	jpeg_save_LIBS="$LIBS"

	LIBS="-ljpeg $LIBS"

	# Check the standard search path
	AC_TRY_COMPILE([
		#include <stdio.h>
		#include <jpeglib.h>],[int a],[
		JPEG_INCLUDES=""
	], [
		# jpeg.h is not in the standard search path.

		# A whole bunch of guesses
		for dir in \
			"${prefix}"/*/include \
			/usr/local/include \
			/usr/*/include \
			/usr/local/*/include /usr/*/include \
			"${prefix}"/include/* \
			/usr/include/* /usr/local/include/* \
			/opt/include /opt/*/include /*/include; do
			if test -f "$dir/jpeglib.h"; then
				JPEG_INCLUDES="-I$dir"
				break
			fi
		done

		if test "$JPEG_INCLUDES" = ""; then
			JPEG_INCLUDES=no
		fi
	])

	LIBS="$jpeg_save_LIBS"
fi

# Now for the libraries
if test "$JPEG_LIBS" = ""; then
	jpeg_save_LIBS="$LIBS"
	jpeg_save_INCLUDES="$INCLUDES"

	LIBS="-ljpeg $LIBS"
	INCLUDES="$JPEG_INCLUDES $INCLUDES"

	# Try the standard search path first
	AC_TRY_LINK([
		#include <stdio.h>
		#include <jpeglib.h>],[jpeg_abort((void*)0)], [
		JPEG_LIBS="-ljpeg"
	], [
		# libjpeg is not in the standard search path.

		# A whole bunch of guesses
		for dir in \
			"${prefix}"/*/lib \
			/usr/local/lib \
			/usr/*/lib \
			"${prefix}"/lib/* /usr/lib/* \
			/usr/local/lib/* \
			/opt/lib /opt/*/lib /*/lib; do
			if test -d "$dir" && test "`ls $dir/libjpeg.* 2> /dev/null`" != ""; then
				JPEG_LIBS="-L$dir -ljpeg"
				break
			fi
		done

		if test "$JPEG_LIBS" = ""; then
			JPEG_LIBS=no
		fi
	])

	LIBS="$jpeg_save_LIBS"
	INCLUDES="$jpeg_save_INCLUDES"
fi

AC_SUBST(JPEG_LIBS)
AC_SUBST(JPEG_INCLUDES)

# Print a helpful message
jpeg_libraries_result="$JPEG_LIBS"
jpeg_includes_result="$JPEG_INCLUDES"

if test x"$jpeg_libraries_result" = x""; then
	jpeg_libraries_result="in default path"
fi
if test x"$jpeg_includes_result" = x""; then
	jpeg_includes_result="in default path"
fi

if test "$jpeg_libraries_result" = "no"; then
	jpeg_libraries_result="(none)"
fi
if test "$jpeg_includes_result" = "no"; then
	jpeg_includes_result="(none)"
fi

AC_MSG_RESULT(
  [libraries $jpeg_libraries_result, headers $jpeg_includes_result])

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test "$JPEG_INCLUDES" != "no" && test "$JPEG_LIBS" != "no"; then
        ifelse([$1],,AC_DEFINE(HAVE_JPEG,1,[Define if you have jpeg libraries and header files.]),[$1])
        :
else
	JPEG_INCLUDES=""
	JPEG_LIBS=""
        $2
fi

])dnl

dnl From FIND_MOTIF and ACX_PTHREAD, without much understanding
dnl
dnl FIND_PNG[ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]]
dnl ------------------------------------------------
dnl
dnl Find PNG libraries and headers
dnl
dnl Put compile stuff in PNG_INCLUDES
dnl Put link stuff in PNG_LIBS
dnl
dnl Default ACTION-IF-FOUND defines HAVE_PNG
dnl
AC_DEFUN([FIND_PNG], [
AC_REQUIRE([AC_PATH_XTRA])

PNG_INCLUDES=""
PNG_LIBS=""

AC_ARG_WITH(png, 
[  --without-png        		do not use libpng])
# Treat --without-png like --without-png-includes --without-png-libraries.
if test "$with_png" = "no"; then
	PNG_INCLUDES=no
	PNG_LIBS=no
fi

AC_ARG_WITH(png-includes,
[  --with-png-includes=DIR	PNG include files are in DIR],
PNG_INCLUDES="-I$withval")
AC_ARG_WITH(png-libraries,
[  --with-png-libraries=DIR	PNG libraries are in DIR],
PNG_LIBS="-L$withval -lpng")

AC_MSG_CHECKING(for PNG)

# Look for png.h 
if test "$PNG_INCLUDES" = ""; then
	png_save_LIBS="$LIBS"

	LIBS="-lpng $LIBS"

	# Check the standard search path
	AC_TRY_COMPILE([
		#include <png.h>],[int a],[
		PNG_INCLUDES=""
	], [
		# png.h is not in the standard search path.

		# A whole bunch of guesses
		for dir in \
			"${prefix}"/*/include \
			/usr/local/include \
			/usr/*/include \
			/usr/local/*/include /usr/*/include \
			"${prefix}"/include/* \
			/usr/include/* /usr/local/include/* /*/include; do
			if test -f "$dir/png.h"; then
				PNG_INCLUDES="-I$dir"
				break
			fi
		done

		if test "$PNG_INCLUDES" = ""; then
			PNG_INCLUDES=no
		fi
	])

	LIBS="$png_save_LIBS"
fi

# Now for the libraries
if test "$PNG_LIBS" = ""; then
	png_save_LIBS="$LIBS"
	png_save_INCLUDES="$INCLUDES"

	LIBS="-lpng $LIBS"
	INCLUDES="$PNG_INCLUDES $INCLUDES"

	# Try the standard search path first
	AC_TRY_LINK([
		#include <png.h>],[png_access_version_number()], [
		PNG_LIBS="-lpng"
	], [
		# libpng is not in the standard search path.

		# A whole bunch of guesses
		for dir in \
			"${prefix}"/*/lib \
			/usr/local/lib \
			/usr/*/lib \
			"${prefix}"/lib/* /usr/lib/* \
			/usr/local/lib/* /*/lib; do
			if test -d "$dir" && test "`ls $dir/libpng.* 2> /dev/null`" != ""; then
				PNG_LIBS="-L$dir -lpng"
				break
			fi
		done

		if test "$PNG_LIBS" = ""; then
			PNG_LIBS=no
		fi
	])

	LIBS="$png_save_LIBS"
	INCLUDES="$png_save_INCLUDES"
fi

AC_SUBST(PNG_LIBS)
AC_SUBST(PNG_INCLUDES)

# Print a helpful message
png_libraries_result="$PNG_LIBS"
png_includes_result="$PNG_INCLUDES"

if test x"$png_libraries_result" = x""; then
	png_libraries_result="in default path"
fi
if test x"$png_includes_result" = x""; then
	png_includes_result="in default path"
fi

if test "$png_libraries_result" = "no"; then
	png_libraries_result="(none)"
fi
if test "$png_includes_result" = "no"; then
	png_includes_result="(none)"
fi

AC_MSG_RESULT(
  [libraries $png_libraries_result, headers $png_includes_result])

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test "$PNG_INCLUDES" != "no" && test "$PNG_LIBS" != "no"; then
        ifelse([$1],,AC_DEFINE(HAVE_PNG,1,[Define if you have png libraries and header files.]),[$1])
        :
else
	PNG_INCLUDES=""
	PNG_LIBS=""
        $2
fi

])dnl

dnl a macro to check for ability to create python extensions
dnl  AM_CHECK_PYTHON_HEADERS([ACTION-IF-POSSIBLE], [ACTION-IF-NOT-POSSIBLE])
dnl function also defines PYTHON_INCLUDES
AC_DEFUN([AM_CHECK_PYTHON_HEADERS],
[AC_REQUIRE([AM_PATH_PYTHON])
AC_MSG_CHECKING(for headers required to compile python extensions)
dnl deduce PYTHON_INCLUDES
py_prefix=`$PYTHON -c "import sys; print sys.prefix"`
py_exec_prefix=`$PYTHON -c "import sys; print sys.exec_prefix"`
PYTHON_INCLUDES="-I${py_prefix}/include/python${PYTHON_VERSION}"
if test "$py_prefix" != "$py_exec_prefix"; then
  PYTHON_INCLUDES="$PYTHON_INCLUDES -I${py_exec_prefix}/include/python${PYTHON_VERSION}"
fi
AC_SUBST(PYTHON_INCLUDES)
dnl check if the headers exist:
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $PYTHON_INCLUDES"
AC_TRY_CPP([#include <Python.h>],dnl
[AC_MSG_RESULT(found)
$1],dnl
[AC_MSG_RESULT(not found)
$2])
CPPFLAGS="$save_CPPFLAGS"
])

