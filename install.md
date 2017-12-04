---
---

Most unix-like operating systems have libvips packages, check your package 
manager. For macOS, there are packages in Homebrew, MacPorts and Fink. For
Windows, there are pre-compiled binaries in the [Download area]({{
site.github.releases_url }}).

## Installing on macOS with homebrew

Install [homebrew](https://brew.sh/), then enter:

	brew install vips

That will install vips with most optional add-ons included. It won't include
ImageMagick/GraphicsMagick support (used for loading things like BMP), you have
to enable that if you need it:

	brew install vips --with-imagemagick

or:

	brew install vips --with-graphicsmagick

It also won't include things like openslide, cfitsio and matlab support. You
must install those packages first, then install libvips with:

	brew install vips --env=std

Meaning, pick up packages from the environment. 

## Installing the Windows binary

Download `vips-dev-w64-web-x.y.z.zip` from the [Download area]({{ 
site.github.releases_url }}) and unzip somewhere. At the command-prompt, `cd`
to `vips-x.y.z/bin` and run (for example):

	vips.exe invert some/input/file.jpg some/output/file.jpg

If you want to run `vips.exe` from some other directory on your PC, 
you'll need to set your `PATH`.

The zipfile includes all the libraries and headers for developing with C with
any compiler. For C++, you must build with `g++`, or rebuild the C++ API 
with your compiler, or just use the C API. 

The `vips-dev-w64-web-x.y.z.zip` is built with a small set of relatively secure
file format readers and can be used in a potentially hostile environment. The
much larger `vips-dev-w64-all-x.y.z.zip` includes all the file format readers
that libvips supports and care should be taken before public deployment.

The Windows binary is built
by [build-win64](https://github.com/jcupitt/build-win64). This is a
containerised mingw build system: on any host, install Docker, 
clone the project, and type `./build.sh 8.5`. The README has notes.

## Building libvips from a source tarball

If the packaged version is too old, you might need to build from source. 

Download `vips-x.y.z.tar.gz` from the [Download area]({{
site.github.releases_url }}), then:

	$ tar xf vips-x.y.z.tar.gz
	$ cd vips-x.y.z
	$ ./configure

Check the summary at the end of `configure` carefully.  libvips must have
`build-essential`, `pkg-config`, `glib2.0-dev`, `libexpat1-dev`.

You'll need the dev packages for the file format support you want. For basic
jpeg and tiff support, you'll need `libtiff5-dev`, `libjpeg-turbo8-dev`,
and `libgsf-1-dev`.  See the **Dependencies** section below for a full list
of the things that libvips can be configured to use.

Once `configure` is looking OK, compile and install with the usual:

	$ make
	$ sudo make install
	$ sudo ldconfig

By default this will install files to `/usr/local`.

We have detailed guides on the wiki for [building for
Windows](https://github.com/jcupitt/libvips/wiki/Build-for-Windows) and
[building for macOS](https://github.com/jcupitt/libvips/wiki/Build-for-macOS).

## Building libvips from git

Checkout the latest sources with:

	$ git clone git://github.com/jcupitt/libvips.git

Building from git needs more packages, you'll need at least `gtk-doc` 
and `gobject-introspection`, see the dependencies section below. 

Then:

	$ ./autogen.sh
	$ make
	$ sudo make install
	$ sudo ldconfig

## Dependencies 

libvips has to have `glib2.0-dev`. Other dependencies are optional, see below.

## Optional dependencies

If suitable versions are found, libvips will add support for the following
libraries automatically. See `./configure --help` for a set of flags to
control library detection. Packages are generally found with `pkg-config`,
so make sure that is working.

libtiff, giflib and libjpeg do not usually use `pkg-config` so libvips looks for
them in the default path and in `$prefix`. If you have installed your own
versions of these libraries in a different location, libvips will not see
them. Use switches to libvips configure like:

	./configure --prefix=/Users/john/vips \
		--with-giflib-includes=/opt/local/include \
		--with-giflib-libraries=/opt/local/lib \
		--with-tiff-includes=/opt/local/include \
		--with-tiff-libraries=/opt/local/lib \
		--with-jpeg-includes=/opt/local/include \
		--with-jpeg-libraries=/opt/local/lib

or perhaps:

	CFLAGS="-g -Wall -I/opt/local/include -L/opt/local/lib" \
		CXXFLAGS="-g -Wall -I/opt/local/include -L/opt/local/lib" \
		./configure --without-python --prefix=/Users/john/vips 

to get libvips to see your builds.

### vips8 Python binding

If `gobject-introspection`, `python-gi-dev`, and `libgirepository1.0-dev` are
available, libvips will install the deprecated vips8 Python binding. 

This has been replaced by the compatible `pyvips` binding in pip. Only enable it
if you must have it. 

### libjpeg

The IJG JPEG library. Use the `-turbo` version if you can. 

### libexif

If available, libvips adds support for EXIF metadata in JPEG files.

### giflib

The standard gif loader. If this is not present, vips will try to load gifs
via imagemagick instead.

### librsvg

The usual SVG loader. If this is not present, vips will try to load SVGs
via imagemagick instead.

### libpoppler

The usual PDF loader. If this is not present, vips will try to load PDFs
via imagemagick instead.

### libgsf-1

If available, libvips adds support for creating image pyramids with `dzsave`. 

### libtiff

The TIFF library. It needs to be built with support for JPEG and
ZIP compression. 3.4b037 and later are known to be OK. 

### fftw3

If libvips finds this library, it uses it for fourier transforms. 

### lcms2, lcms

If present, `vips_icc_import()`, `vips_icc_export()` and `vips_icc_transform()`
are available for transforming images with ICC profiles. If `lcms2` is 
available it is used in preference to `lcms`, since it is faster.

### Large files

libvips uses the standard autoconf tests to work out how to support
large files (>2GB) on your system. Any reasonably recent unix should
be OK.

### libpng

If present, libvips can load and save png files. 

### ImageMagick, or optionally GraphicsMagick

If available, libvips adds support for loading all libMagick-supported
image file types. Use `--with-magickpackage=GraphicsMagick` to build against 
graphicsmagick instead.

Imagemagick 6.9+ needs to have been built with `--with-modules`. Most packaged
IMs are, I think, but if you are rolling your own, you'll need to pass
this flag to configure. 

If you are going to be using libvips with untrusted images, perhaps in a
web-server, for example, you should consider the security implications of
using a package with such a large attack surface. You might prefer not to
enable Magick support. 

libvips also supports ImageMagick7. 

### pangoft2

If available, libvips adds support for text rendering. You need the
package pangoft2 in `pkg-config --list-all`.

### orc-0.4

If available, vips will accelerate some operations with this run-time
compiler.

### matio

If available, vips can load images from Matlab save files.

### cfitsio

If available, vips can load FITS images.

### libwebp

If available, vips can load and save WebP images.

### OpenEXR

If available, libvips will directly read (but not write, sadly)
OpenEXR images.

### OpenSlide

If available, libvips can load OpenSlide-supported virtual slide
files: Aperio, Hamamatsu, Leica, MIRAX, Sakura, Trestle, and Ventana.

### swig, python, python-dev

If available, we build the old vips7 python binding.
