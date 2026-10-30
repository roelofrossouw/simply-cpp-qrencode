// QR codes: versions and correction chosen, the finder patterns, limits, the SVG's path and size,
// the PNG's structure and every one of its pixels against the modules, the terminal text.
#include <sc_qrencode.h>
#include <sc_test.h>

#include <zlib.h>

#include <algorithm>
#include <cstdint>
#include <string>

namespace {
    std::uint32_t big_endian(const std::string &bytes, const std::size_t at) {
        std::uint32_t value = 0;
        for (std::size_t i = 0; i < 4; ++i) value = value << 8 | static_cast<unsigned char>(bytes[at + i]);
        return value;
    }

    // The PNG's IDAT data, inflated: rows of a filter byte and 1-bit pixels.
    std::string pixels_of(const std::string &png, const std::uint32_t side) {
        std::string packed;
        for (std::size_t at = 8; at + 8 <= png.size();) {
            const auto length = big_endian(png, at);
            if (png.compare(at + 4, 4, "IDAT") == 0) packed += png.substr(at + 8, length);
            at += 12 + length;
        }
        std::string raw(static_cast<std::size_t>(1 + (side + 7) / 8) * side, '\0');
        auto raw_size = static_cast<uLongf>(raw.size());
        if (uncompress(reinterpret_cast<Bytef *>(raw.data()), &raw_size, reinterpret_cast<const Bytef *>(packed.data()),
                       static_cast<uLong>(packed.size())) != Z_OK) return {};
        raw.resize(raw_size);
        return raw;
    }
}

int main() {
    using sc::qr::code;
    using sc::qr::correction;
    using std::string;

    SECTION("Version and correction: the smallest version, then the best correction that fits");
    {
        const code hello{"HELLO WORLD", correction::quartile};
        CHECK_EQ(hello.version(), 1);
        CHECK_EQ(hello.size(), 21);
        CHECK(hello.level() == correction::quartile); // high holds only 10 letters in version 1
        CHECK(code("1", correction::low).level() == correction::high); // raised: it still fits
        const code url{"https://example.com/invoice/123"};
        CHECK_EQ(url.size(), url.version() * 4 + 17);
        CHECK(url.level() >= correction::medium);
    }

    SECTION("Modules: the three finder patterns, nothing outside");
    {
        const code url{"https://example.com/invoice/123"};
        const auto last = url.size() - 1;
        for (int i = 0; i < 7; ++i) {
            CHECK(url.dark(i, 0) && url.dark(0, i));             // top left
            CHECK(url.dark(last - i, 0) && url.dark(last, i));   // top right
            CHECK(url.dark(0, last - i) && url.dark(i, last));   // bottom left
        }
        CHECK(!url.dark(7, 0) && !url.dark(0, 7) && !url.dark(1, 1)); // separator, finder ring
        CHECK(!url.dark(-1, 0) && !url.dark(0, url.size()));
    }

    SECTION("Limits: version 40 at most; NUL only as bytes");
    {
        const auto most = code::binary(string(2953, 'x'), correction::low);
        CHECK_EQ(most.version(), 40);
        CHECK_EQ(most.size(), 177);
        CHECK_THROWS_AS((void) code::binary(string(2954, 'x'), correction::low), std::length_error);
        CHECK_THROWS_AS((void) code(string(3000, 'x')), std::length_error);
        CHECK_THROWS_AS((void) code(string("a\0b", 3)), std::invalid_argument);
        CHECK(code::binary(string("a\0b", 3)).version() >= 1);
        CHECK(code{"same"}.svg() == code{"same"}.svg()); // the same data, the same code
    }

    SECTION("SVG: one path in a viewBox of modules, or a given size");
    {
        const code hello{"HELLO WORLD", correction::quartile};
        const auto plain = hello.svg();
        CHECK(plain.starts_with("<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 29 29\""));
        CHECK(plain.substr(0, plain.find('>')).find("width=") == string::npos); // no size of its own
        CHECK(plain.find("<path fill=\"#000\" d=\"M4,4h7v1h-7z") != string::npos); // the top-left finder's first row
        CHECK(plain.ends_with("</svg>\n"));
        CHECK(plain.find("<rect width=\"29\" height=\"29\" fill=\"#fff\"/>") != string::npos); // the viewBox's size, not 100%
        const auto sized = hello.svg(sc::dist::mm(30), 2);
        CHECK(sized.find("viewBox=\"0 0 25 25\" width=\"30mm\" height=\"30mm\"") != string::npos);
        CHECK(hello.svg(sc::dist::inch(1)).find("width=\"25.4mm\"") != string::npos);
        CHECK_THROWS_AS((void) hello.svg(sc::distance{0}), std::invalid_argument);
        CHECK_THROWS_AS((void) hello.svg(std::nullopt, -1), std::invalid_argument);
    }

    SECTION("PNG: a 1-bit greyscale picture, each pixel its module's");
    {
        const code url{"https://example.com/invoice/123"};
        const int scale = 3, zone = 4;
        const auto png = url.png(scale, zone);
        const auto side = static_cast<std::uint32_t>((url.size() + 2 * zone) * scale);
        CHECK(png.starts_with(string{"\x89PNG\r\n\x1a\n", 8}));
        CHECK_EQ(png.compare(12, 4, "IHDR"), 0);
        CHECK_EQ(big_endian(png, 16), side);
        CHECK_EQ(big_endian(png, 20), side);
        CHECK_EQ(static_cast<int>(png[24]), 1); // bit depth
        CHECK_EQ(static_cast<int>(png[25]), 0); // greyscale
        CHECK(png.ends_with(string{"IEND\xae\x42\x60\x82", 8}));
        const auto raw = pixels_of(png, side);
        const auto row = 1 + (side + 7) / 8;
        CHECK_EQ(raw.size(), static_cast<std::size_t>(row) * side);
        bool same = !raw.empty();
        for (std::uint32_t y = 0; same && y < side; ++y) {
            same = raw[y * row] == '\0';
            for (std::uint32_t x = 0; same && x < side; ++x) {
                const bool white = (static_cast<unsigned char>(raw[y * row + 1 + x / 8]) >> (7 - x % 8)) & 1;
                same = white != url.dark(static_cast<int>(x) / scale - zone, static_cast<int>(y) / scale - zone);
            }
        }
        CHECK(same);
        CHECK_THROWS_AS((void) url.png(0), std::invalid_argument);
        CHECK_THROWS_AS((void) url.png(101), std::invalid_argument);
        CHECK(url.png(1, 0).size() < 1000);
    }

    SECTION("Text: two rows a line, the quiet zone around");
    {
        const code hello{"HELLO WORLD", correction::quartile};
        const auto text = hello.text();
        CHECK_EQ(static_cast<int>(std::count(text.begin(), text.end(), '\n')), (21 + 4 + 1) / 2);
        const auto first = text.substr(0, text.find('\n'));
        string light;
        for (int i = 0; i < 25; ++i) light += "█";
        CHECK_EQ(first, light); // the quiet zone's two rows: light
        CHECK(text.substr(first.size() + 1).starts_with("██ ")); // the quiet zone, then the finder's dark corner
    }

    TEST_SUMMARY();
}
