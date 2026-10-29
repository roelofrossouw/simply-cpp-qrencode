# EXAMPLES
A link as a PNG, 8 pixels a module:
```
qrencode -o invoice.png -s 8 "https://example.com/invoice/123"
```
An SVG of a file's contents, with the highest error correction:
```
qrencode -t SVG -l H -r contact.vcf -o contact.svg
```
From a program, shown in the terminal:
```
echo -n "WIFI:S:Home;T:WPA;P:secret;;" | qrencode -t UTF8
```
A PNG to standard output, for a web server:
```
qrencode -o - "https://example.com" > code.png
```

# NOTES
It takes libqrencode's qrencode options for what sc-qrencode can do: `-o`, `-r`, `-s`, `-l`, `-m`, `-d`, `-t`, `-8` and
the data as an argument or on standard input, with the same defaults (level L, 3 pixels a module, a margin of 4, 72 dpi,
PNG). Its other options (`-v`, `-S`, `-M`, `-k`, `-i`, `--foreground`, `--background`, `--rle`) and its EPS, XPM and ANSI
types aren't there: they are reported as unknown, as is `-V`: use `--version`.

The encoding chooses the smallest version that holds the data and raises the error correction when a higher level still
fits. A PNG is black and white at 1 bit a pixel, each module an exact square of pixels; `-d` only sets an SVG's printed
size (the PNG has no resolution in it). An SVG's dark modules are one path, so it scales cleanly.

For C++ and PHP programs, sc-qrencode's sc::qr::code and sc-phplib's SC\QrCode do the same without starting a program.

# SEE ALSO
https://github.com/roelofrossouw/simply-cpp-qrencode
