// qrencode, the program: a QR code of a string, a file or standard input, as PNG, SVG or text, with
// libqrencode's command line where sc-qrencode can do what it asks.

#include <sc_qrencode.h>

#include <params.h>

#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>

namespace {
    // ASCII: two characters a module, a line a row, the quiet zone around. inverted: light
    // modules drawn instead of dark ones.
    std::string ascii(const sc::qr::code &code, const int margin, const bool inverted) {
        std::string out;
        for (int y = -margin; y < code.size() + margin; ++y) {
            for (int x = -margin; x < code.size() + margin; ++x) out += code.dark(x, y) != inverted ? "##" : "  ";
            out += '\n';
        }
        return out;
    }

    // UTF8i: two rows a line in half blocks, the dark modules drawn (for light terminals); UTF8,
    // the light ones, is code.text().
    std::string utf8_dark(const sc::qr::code &code, const int margin) {
        std::string out;
        for (int y = -margin; y < code.size() + margin; y += 2) {
            for (int x = -margin; x < code.size() + margin; ++x) {
                const bool top = code.dark(x, y), bottom = y + 1 < code.size() + margin && code.dark(x, y + 1);
                out += top && bottom ? "█" : top ? "▀" : bottom ? "▄" : " ";
            }
            out += '\n';
        }
        return out;
    }

    std::string read_all(std::istream &in) { return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()}; }
}

int main(int argc, char **argv) {
    const sc::params args{argc, argv, {
        {"name", "qrencode"},
        {"description", "Encodes data in a QR code and writes it as a PNG, an SVG or text. It takes libqrencode's qrencode "
                        "options for what sc-qrencode can do, so scripts written for either work with it."},
        {"version", SC_QRENCODE_VERSION},
        {"epilog", "Without STRING or --read-from it reads standard input. A PNG needs --output; the other types go to "
                   "standard output unless --output names a file. Data holding a NUL byte is encoded as bytes, as with --8bit."},
        {"options", {
            {"output", {{"short", "o"}, {"value", "FILENAME"}, {"help", "write the image to FILENAME (- for standard output)"}}},
            {"read-from", {{"short", "r"}, {"value", "FILENAME"}, {"help", "read the data from FILENAME"}}},
            {"size", {{"short", "s"}, {"type", "integer"}, {"value", "NUMBER"}, {"default", 3},
                      {"help", "a module's size in pixels (1 to 100); an SVG's with --dpi"}}},
            {"level", {{"short", "l"}, {"value", "LEVEL"}, {"default", "L"}, {"choices", {"L", "M", "Q", "H"}},
                       {"help", "the error correction, from L (lowest, about 7% may be damaged) to H (highest, 30%); "
                                "a higher one is used when it still fits"}}},
            {"margin", {{"short", "m"}, {"type", "integer"}, {"value", "NUMBER"}, {"default", 4},
                        {"help", "the quiet zone around the code, in modules"}}},
            {"dpi", {{"short", "d"}, {"type", "integer"}, {"value", "NUMBER"}, {"default", 72},
                     {"help", "pixels an inch, for an SVG's printed size"}}},
            {"type", {{"short", "t"}, {"value", "TYPE"}, {"default", "PNG"}, {"choices", {"PNG", "SVG", "UTF8", "UTF8i", "ASCII", "ASCIIi"}},
                      {"help", "the output: PNG (1-bit), SVG, UTF8 (half blocks for a dark terminal; UTF8i for a light one), "
                               "ASCII (# for dark; ASCIIi inverted)"}}},
            {"8bit", {{"short", "8"}, {"type", "boolean"}, {"help", "encode the data as bytes, as they are"}}}}},
        {"positional", {
            {"string", {{"required", false}, {"help", "the data (default: standard input)"}}}}}}};
    try {
        std::string data;
        if (const auto *text = args.lookup("string")) {
            data = text->get<std::string>();
        } else if (const auto *file = args.lookup("read-from")) {
            std::ifstream in{file->get<std::string>(), std::ios::binary};
            if (!in) throw std::runtime_error("can't read " + file->get<std::string>());
            data = read_all(in);
        } else {
            data = read_all(std::cin);
        }
        if (data.empty()) throw std::runtime_error("no data to encode");

        static const std::map<std::string, sc::qr::correction> levels{{"L", sc::qr::correction::low}, {"M", sc::qr::correction::medium},
                                                                      {"Q", sc::qr::correction::quartile}, {"H", sc::qr::correction::high}};
        const auto level = levels.at(args.as<std::string>("level"));
        const bool bytes = args.as<bool>("8bit") || data.find('\0') != std::string::npos;
        const auto code = bytes ? sc::qr::code::binary(data, level) : sc::qr::code{data, level};

        const auto margin = args.as<int>("margin"), size = args.as<int>("size"), dpi = args.as<int>("dpi");
        if (margin < 0 || margin > 100) throw std::invalid_argument("--margin is 0 to 100");
        if (dpi < 1) throw std::invalid_argument("--dpi must be at least 1");
        const auto type = args.as<std::string>("type");
        std::string out;
        if (type == "PNG") {
            out = code.png(size, margin);
        } else if (type == "SVG") {
            if (size < 1) throw std::invalid_argument("--size must be at least 1");
            const double inches = static_cast<double>((code.size() + 2 * margin) * size) / dpi;
            out = code.svg(sc::dist::inch(inches), margin);
        } else if (type == "UTF8") {
            out = code.text(margin);
        } else if (type == "UTF8i") {
            out = utf8_dark(code, margin);
        } else {
            out = ascii(code, margin, type == "ASCIIi");
        }

        const auto *output = args.lookup("output");
        const auto file = output ? output->get<std::string>() : std::string{};
        if (file.empty() || file == "-") {
            if (!output && type == "PNG") throw std::runtime_error("no output file given (-o FILENAME, or -o - for standard output)");
            std::cout << out;
        } else {
            std::ofstream to{file, std::ios::binary | std::ios::trunc};
            to << out;
            if (!to) throw std::runtime_error("can't write " + file);
        }
    } catch (const std::exception &problem) {
        std::cerr << "qrencode: " << problem.what() << '\n';
        return 1;
    }
    return 0;
}
