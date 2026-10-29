#pragma once

#include <distance.h>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// QR codes (ISO/IEC 18004) made from text or bytes, written as SVG or PNG with no image library:
// a QR code is a grid of squares, so both are written directly (the PNG with zlib).
//
//     const sc::qr::code code{"https://example.com/invoice/123"};
//     code.save_svg("invoice.svg", sc::dist::mm(30));  // or a viewBox only, to scale where it's shown
//     code.save_png("invoice.png", 8);                 // 8 pixels a module
//
// The encoding is Project Nayuki's QR Code generator (MIT), compiled in.
//
// TODO: reading QR codes from images, as a module of its own (sc-qrdecode), so this one stays light.
namespace sc::qr {
    // How much of a code may be damaged and still read: about 7%, 15%, 25% or 30%.
    enum class correction { low, medium, quartile, high };

    class code {
    public:
        // Text, as UTF-8 (digits and upper-case letters are packed tighter). The smallest version
        // that holds it is chosen, then the highest correction that still fits, at least
        // at_least. Throws std::length_error when it doesn't fit version 40 (2953 bytes at low),
        // and std::invalid_argument for text holding a NUL (code::binary() takes any bytes).
        explicit code(std::string_view text, correction at_least = correction::medium);
        // Bytes as they are (byte mode), for data that isn't text.
        static code binary(std::string_view bytes, correction at_least = correction::medium);

        // 1 to 40.
        [[nodiscard]] int version() const { return version_; }
        // Modules a side, without the quiet zone: 21 to 177 (4 a version, plus 17).
        [[nodiscard]] int size() const { return size_; }
        [[nodiscard]] correction level() const { return level_; }
        // Whether a module is dark, counted from the top left; outside the code: false (light).
        [[nodiscard]] bool dark(int x, int y) const;

        // The dark modules as one path on a white square, with quiet_zone light modules around
        // (4, as the standard asks). size is the whole picture's width and height, quiet zone
        // included; without it the SVG has a viewBox only and takes the size it's shown at.
        // Throws std::invalid_argument for a size that isn't positive or a negative quiet zone.
        [[nodiscard]] std::string svg(std::optional<sc::distance> size = std::nullopt, int quiet_zone = 4) const;
        // Black and white, 1 bit a pixel, module_pixels (1 to 100) a module. Throws
        // std::invalid_argument for other sizes or a negative quiet zone.
        [[nodiscard]] std::string png(int module_pixels = 8, int quiet_zone = 4) const;
        // svg() and png() written to a file; throw std::runtime_error when it can't be written.
        void save_svg(const std::filesystem::path &file, std::optional<sc::distance> size = std::nullopt, int quiet_zone = 4) const;
        void save_png(const std::filesystem::path &file, int module_pixels = 8, int quiet_zone = 4) const;

        // For a terminal with light text on a dark background: two rows a line, the light modules
        // drawn with half blocks, so phones read it off the screen.
        [[nodiscard]] std::string text(int quiet_zone = 2) const;

    private:
        code() = default;

        int version_ = 0;
        int size_ = 0;
        correction level_ = correction::low;
        std::vector<bool> modules_; // row by row
    };
}
