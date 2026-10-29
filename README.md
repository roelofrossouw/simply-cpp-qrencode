# simply-cpp-qrencode

`sc-qrencode` makes QR codes (ISO/IEC 18004) from text or bytes and writes them
as SVG or PNG, with no image library: a QR code is a grid of squares, so both
are written directly (the PNG with zlib). It stays light on purpose: sc-core and
zlib are all it needs.

**Documentation:** the [simply-cpp wiki](https://github.com/roelofrossouw/simply-cpp/wiki) has a [QR codes guide](https://github.com/roelofrossouw/simply-cpp/wiki/QR-Codes) and the [sc-qrencode reference](https://github.com/roelofrossouw/simply-cpp/wiki/Reference-sc-qrencode).

```cpp
#include <sc_qrencode.h>

const sc::qr::code code{"https://example.com/invoice/123"};
code.save_svg("invoice.svg", sc::dist::mm(30));    // 30 mm square, quiet zone included
code.save_svg("scales.svg");                       // a viewBox only: the size it's shown at
code.save_png("invoice.png", 8);                   // 8 pixels a module, 1 bit a pixel
const std::string svg = code.svg(), png = code.png();
std::cout << code.text();                          // in a terminal, readable by a phone

const auto bytes = sc::qr::code::binary(data, sc::qr::correction::high);
code.version(); code.size(); code.dark(x, y);      // 1 to 40, modules a side, one module
```

- Text is UTF-8; digits and upper-case letters are packed tighter. The smallest
  version that holds the data is chosen, then the highest correction level that
  still fits, at least the one asked for (`low`, `medium`, `quartile`, `high`:
  about 7, 15, 25 or 30% of the code may be damaged). Too much data (2953 bytes
  at `low`) throws `std::length_error`.
- The SVG is one path on a white square, with `shape-rendering="crispEdges"`;
  it's an `sc::drawing` too through sc-graphics' SVG reader, for a canvas or a PDF.
- The PNG is black and white at 1 bit a pixel, each module an exact block of
  pixels: a few hundred bytes for a link.
- Both keep a quiet zone of 4 light modules around the code, as the standard
  asks (a parameter, for tight layouts).

## Licences

sc-qrencode is Apache-2.0. The encoding is Project Nayuki's
[QR Code generator](https://github.com/nayuki/QR-Code-generator) (C++, MIT),
vendored in `third_party/qrcodegen` (commit `3c6d0b3cefb4`) and compiled in, its
namespace renamed so a program with its own copy doesn't clash. zlib (zlib
License) is linked from the system.

**TODO:** reading QR codes from images, as a module of its own (sc-qrdecode,
on OpenCV's QR reader or zxing-cpp), so this one stays light.

## Demo

`sc-qrencode-demo [TEXT] [--svg FILE [--size MM]] [--png FILE [--scale PIXELS]] [--correction LEVEL]`
shows a code in the terminal, or writes it as SVG or PNG.

<!-- sc-example: examples/sc-qrencode-demo.cpp -->
```cpp
const sc::qr::code code{args.as<std::string>("text"), level->second};
const auto svg = args.as<std::string>("svg"), png = args.as<std::string>("png");
if (!svg.empty()) {
    const auto mm = args.as<double>("size");
    code.save_svg(svg, mm > 0 ? std::optional<sc::distance>{sc::dist::mm(mm)} : std::nullopt);
}
if (!png.empty()) code.save_png(png, args.as<int>("scale"));
if (svg.empty() && png.empty()) std::cout << code.text();
std::cout << "Version " << code.version() << ": " << code.size() << " x " << code.size() << " modules\n";
```
<!-- /sc-example -->

## Building and testing

Run `bash scripts/install.sh` to configure, build, test, package and install.
The test (`qrencode`, label `unit`) checks the codes' structure, and every PNG
pixel against its module. Codes written by it were read back by OpenCV's QR
reader: text, links, digits, UTF-8 (with emoji), versions 1 to 25.
