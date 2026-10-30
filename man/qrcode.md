# EXIT STATUS
0 when the code was shown or written, 1 when it couldn't be (no data, an invalid type, a file that can't be read or
written), and 2 for a mistake in the options.

# EXAMPLES
A link, shown in the terminal:
```
qrcode "https://example.com"
```
From a program, shown in the terminal:
```
echo -n "WIFI:S:Home;T:WPA;P:secret;;" | qrcode
```
A link as a PNG, 8 pixels a module:
```
qrcode -o invoice.png -s 8 "https://example.com/invoice/123"
```
An SVG of a file's contents, with the highest error correction:
```
qrcode -l H -r contact.vcf -o contact.svg
```
A PNG to standard output, for a web server:
```
qrcode -t PNG -o - "https://example.com" > code.png
```

# NOTES
Run without anything to encode (no STRING, no `-r`, nothing piped in), it shows its help. The code goes to the terminal
as text unless `-o` names a file, whose name gives the type: `.png` or `.svg`; any other name is an invalid type unless
`-t` says which.

It takes libqrencode's qrencode options, by the same letters, for what sc-qrencode can do: `-o`, `-r`, `-s`, `-l`, `-m`, `-d`, `-t`, `-8` and
the data as an argument or on standard input, with the same defaults (level L, 3 pixels a module, a margin of 4, 72 dpi).
Unlike libqrencode's, it shows the code in the terminal by default instead of needing `-o` for a PNG. Its other options (`-v`, `-S`, `-M`, `-k`, `-i`, `--foreground`, `--background`, `--rle`) and its EPS, XPM and ANSI
types aren't there: they are reported as unknown, as is `-V`: use `--version`.

The encoding chooses the smallest version that holds the data and raises the error correction when a higher level still
fits. A PNG is black and white at 1 bit a pixel, each module an exact square of pixels; `-d` only sets an SVG's printed
size (the PNG has no resolution in it). An SVG's dark modules are one path, so it scales cleanly.

It is called qrcode so that it can be installed beside libqrencode's qrencode (Ubuntu's qrencode package).

For C++ and PHP programs, sc-qrencode's sc::qr::code and sc-phplib's SC\QrCode do the same without starting a program.

# SEE ALSO
https://github.com/roelofrossouw/simply-cpp-qrencode
