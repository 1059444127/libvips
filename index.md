---
---

[![Build Status](https://travis-ci.org/jcupitt/libvips.svg?branch=master)](https://travis-ci.org/jcupitt/libvips)
[![Coverity Status](https://scan.coverity.com/projects/6503/badge.svg)](https://scan.coverity.com/projects/jcupitt-libvips)

libvips is a 2D image processing library. Compared to
similar libraries, [libvips runs quickly and uses little
memory](https://github.com/jcupitt/libvips/wiki/Speed-and-memory-use).
libvips is licensed under the LGPL 2.1+.

It has around 300 operations covering arithmetic, histograms, convolutions,
morphological operations, frequency filtering, colour, resampling, statistics
and others. It supports a large range of numeric formats, from 8-bit int to
128-bit complex. Images can have any number of bands. It supports a good
range of image formats, including JPEG, TIFF, PNG, WebP, FITS, Matlab,
OpenEXR, PDF, SVG, HDR, PPM, CSV, GIF, Analyze, DeepZoom, and OpenSlide.
It can also load images via ImageMagick or GraphicsMagick.

It has APIs for [C](API/current/using-from-c.html) and [C++](API/current/using-from-cpp.html)
and comes with a [Python binding](API/current/using-from-python.html) and a 
[command-line interface](API/current/using-cli.html). Bindings are available for 
[Ruby](https://rubygems.org/gems/ruby-vips), 
[PHP](https://github.com/jcupitt/php-vips), and
[Go](https://github.com/davidbyttow/govips).
libvips is used as an image processing engine by
[sharp (on node.js)](https://www.npmjs.org/package/sharp),
[bimg](https://github.com/h2non/bimg), 
[sharp for Go](https://github.com/DAddYE/vips),
[carrierwave-vips](https://github.com/eltiare/carrierwave-vips),
[mediawiki](http://www.mediawiki.org/wiki/Extension:VipsScaler),
[nip2](https://github.com/jcupitt/nip2), 
[PhotoFlow](https://github.com/aferrero2707/PhotoFlow) and others. 

## Cite

Martinez, K. and Cupitt, J. (2005) [VIPS -- a highly tuned image processing
software architecture](http://eprints.ecs.soton.ac.uk/12371). In Proceedings
of IEEE International Conference on Image Processing 2, pp. 574-577, Genova.

Cupitt, J. and Martinez, K. (1996) [VIPS: An image processing system
for large images](http://eprints.soton.ac.uk/252227/1/vipsspie96a.pdf),
Proc. SPIE, vol.  2663, pp. 19--28.

## News

<ul class="blog-index">
  {% for post in site.posts %}
    <li>
      <span class="date">{{ post.date }}</span>
      <h3><a href="{{ site.baseurl }}{{ post.url }}">{{ post.title }}</a></h3>
      {{ post.excerpt }}
    </li>
  {% endfor %}
</ul>
