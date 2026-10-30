#include "sc_qrencode.h"

#include "qrcodegen.hpp" // its namespace renamed sc_qrcodegen by the build, so another copy can't clash

#include <zlib.h>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <stdexcept>

namespace sc::qr {
    namespace {
        using qrcodegen::QrCode;

        QrCode::Ecc ecc(const correction level) {
            switch (level) {
                case correction::low: return QrCode::Ecc::LOW;
                case correction::medium: return QrCode::Ecc::MEDIUM;
                case correction::quartile: return QrCode::Ecc::QUARTILE;
                case correction::high: return QrCode::Ecc::HIGH;
            }
            throw std::invalid_argument("QR: unknown correction level");
        }

        correction level_of(const QrCode::Ecc level) {
            switch (level) {
                case QrCode::Ecc::LOW: return correction::low;
                case QrCode::Ecc::MEDIUM: return correction::medium;
                case QrCode::Ecc::QUARTILE: return correction::quartile;
                case QrCode::Ecc::HIGH: return correction::high;
            }
            return correction::low;
        }

        void check_quiet_zone(const int quiet_zone) {
            if (quiet_zone < 0) throw std::invalid_argument("QR: the quiet zone can't be negative");
        }

        // Up to 3 decimals, without trailing zeros: "30", "25.4".
        std::string number(const double value) {
            char text[32];
            std::snprintf(text, sizeof text, "%.3f", value);
            std::string out{text};
            out.erase(out.find_last_not_of('0') + 1);
            if (out.back() == '.') out.pop_back();
            return out;
        }

        void big_endian(std::string &out, const std::uint32_t value) {
            for (int shift = 24; shift >= 0; shift -= 8) out += static_cast<char>((value >> shift) & 0xff);
        }

        void chunk(std::string &png, const char *type, const std::string &data) {
            big_endian(png, static_cast<std::uint32_t>(data.size()));
            const std::string typed = type + data;
            png += typed;
            big_endian(png, static_cast<std::uint32_t>(
                    crc32(0, reinterpret_cast<const Bytef *>(typed.data()), static_cast<uInt>(typed.size()))));
        }

        void save(const std::filesystem::path &file, const std::string &bytes) {
            std::ofstream out{file, std::ios::binary | std::ios::trunc};
            out << bytes;
            if (!out) throw std::runtime_error("QR: can't write " + file.string());
        }

        // What a code keeps of the encoder's: its modules row by row.
        struct parts {
            int version, size;
            correction level;
            std::vector<bool> modules;
        };

        parts parts_of(const QrCode &made) {
            parts out{made.getVersion(), made.getSize(), level_of(made.getErrorCorrectionLevel()), {}};
            out.modules.reserve(static_cast<std::size_t>(out.size) * out.size);
            for (int y = 0; y < out.size; ++y)
                for (int x = 0; x < out.size; ++x) out.modules.push_back(made.getModule(x, y));
            return out;
        }
    }

    code::code(const std::string_view text, const correction at_least) {
        if (text.find('\0') != std::string_view::npos) throw std::invalid_argument("QR: text can't hold a NUL; use code::binary()");
        auto made = parts_of(QrCode::encodeText(std::string{text}.c_str(), ecc(at_least)));
        version_ = made.version;
        size_ = made.size;
        level_ = made.level;
        modules_ = std::move(made.modules);
    }

    code code::binary(const std::string_view bytes, const correction at_least) {
        auto made = parts_of(QrCode::encodeBinary(std::vector<std::uint8_t>(bytes.begin(), bytes.end()), ecc(at_least)));
        code out;
        out.version_ = made.version;
        out.size_ = made.size;
        out.level_ = made.level;
        out.modules_ = std::move(made.modules);
        return out;
    }

    bool code::dark(const int x, const int y) const {
        if (x < 0 || y < 0 || x >= size_ || y >= size_) return false;
        return modules_[static_cast<std::size_t>(y) * size_ + x];
    }

    std::string code::svg(const std::optional<sc::distance> size, const int quiet_zone) const {
        check_quiet_zone(quiet_zone);
        if (size && size->points() <= 0) throw std::invalid_argument("QR: an SVG's size must be positive");
        const auto whole = size_ + 2 * quiet_zone;
        std::string out = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 )" + std::to_string(whole) + " " + std::to_string(whole) + '"';
        if (size) {
            const auto length = number(size->millimetres()) + "mm";
            out += R"( width=")" + length + R"(" height=")" + length + '"';
        }
        // The light square in the viewBox's own units, not 100%: readers that don't resolve percentages
        // (sc::drawing's) would take it as 100 units.
        out += R"( shape-rendering="crispEdges"><rect width=")" + std::to_string(whole) + R"(" height=")" + std::to_string(whole) +
               R"(" fill="#fff"/><path fill="#000" d=")";
        // Each row's runs of dark modules as rectangles: "M4,4h7v1h-7z".
        for (int y = 0; y < size_; ++y) {
            for (int x = 0; x < size_;) {
                if (!dark(x, y)) {
                    ++x;
                    continue;
                }
                int run = 0;
                while (dark(x + run, y)) ++run;
                out += "M" + std::to_string(x + quiet_zone) + "," + std::to_string(y + quiet_zone) + "h" + std::to_string(run) + "v1h-" +
                       std::to_string(run) + "z";
                x += run;
            }
        }
        out += "\"/></svg>\n";
        return out;
    }

    std::string code::png(const int module_pixels, const int quiet_zone) const {
        check_quiet_zone(quiet_zone);
        if (module_pixels < 1 || module_pixels > 100) throw std::invalid_argument("QR: a module is 1 to 100 pixels");
        const auto side = static_cast<std::uint32_t>((size_ + 2 * quiet_zone) * module_pixels);
        // Rows of 1-bit greyscale (0 black, 1 white), each after its filter byte (0: none).
        const auto row_bytes = 1 + (side + 7) / 8;
        std::string raw;
        raw.reserve(static_cast<std::size_t>(row_bytes) * side);
        for (std::uint32_t py = 0; py < side; ++py) {
            const auto y = static_cast<int>(py) / module_pixels - quiet_zone;
            raw += '\0';
            for (std::uint32_t byte = 0; byte < row_bytes - 1; ++byte) {
                unsigned bits = 0;
                for (int bit = 0; bit < 8; ++bit) {
                    const auto px = byte * 8 + static_cast<std::uint32_t>(bit);
                    const bool white = px >= side || !dark(static_cast<int>(px) / module_pixels - quiet_zone, y);
                    if (white) bits |= 0x80u >> bit;
                }
                raw += static_cast<char>(bits);
            }
        }
        std::string packed(compressBound(static_cast<uLong>(raw.size())), '\0');
        auto packed_size = static_cast<uLongf>(packed.size());
        if (compress2(reinterpret_cast<Bytef *>(packed.data()), &packed_size, reinterpret_cast<const Bytef *>(raw.data()),
                      static_cast<uLong>(raw.size()), Z_BEST_COMPRESSION) != Z_OK) {
            throw std::runtime_error("QR: zlib couldn't compress the picture");
        }
        packed.resize(packed_size);

        std::string out{"\x89PNG\r\n\x1a\n", 8};
        std::string header;
        big_endian(header, side);
        big_endian(header, side);
        header += std::string{"\x01\x00\x00\x00\x00", 5}; // bit depth 1, greyscale, deflate, adaptive filters, not interlaced
        chunk(out, "IHDR", header);
        chunk(out, "IDAT", packed);
        chunk(out, "IEND", "");
        return out;
    }

    void code::save_svg(const std::filesystem::path &file, const std::optional<sc::distance> size, const int quiet_zone) const {
        save(file, svg(size, quiet_zone));
    }

    void code::save_png(const std::filesystem::path &file, const int module_pixels, const int quiet_zone) const {
        save(file, png(module_pixels, quiet_zone));
    }

    std::string code::text(const int quiet_zone) const {
        check_quiet_zone(quiet_zone);
        // Light modules lit: on a dark terminal they show as the code's white.
        const auto light = [&](const int x, const int y) { return !dark(x, y); };
        std::string out;
        for (int y = -quiet_zone; y < size_ + quiet_zone; y += 2) {
            for (int x = -quiet_zone; x < size_ + quiet_zone; ++x) {
                const bool top = light(x, y), bottom = y + 1 < size_ + quiet_zone && light(x, y + 1);
                out += top && bottom ? "█" : top ? "▀" : bottom ? "▄" : " ";
            }
            out += '\n';
        }
        return out;
    }
}
