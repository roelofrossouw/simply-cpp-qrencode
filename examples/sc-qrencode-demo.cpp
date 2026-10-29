// Makes a QR code of some text: shows it in the terminal, or writes it as SVG or PNG.
//
//     sc-qrencode-demo "https://example.com/invoice/123"
//     sc-qrencode-demo "WIFI:S:Home;T:WPA;P:secret;;" --svg wifi.svg --size 40
//     sc-qrencode-demo "Hello" --png hello.png --scale 10 --correction high

#include <sc_qrencode.h>
#include <params.h>

#include <iostream>
#include <map>

int main(int argc, char **argv) {
    const sc::params args{argc, argv, {
        {"name", "sc-qrencode-demo"},
        {"description", "Makes a QR code of TEXT and shows it in the terminal, or writes it as SVG (--svg) or PNG (--png)."},
        {"options", {
            {"svg", {{"short", "s"}, {"value", "FILE"}, {"default", ""}, {"help", "write an SVG"}}},
            {"png", {{"short", "p"}, {"value", "FILE"}, {"default", ""}, {"help", "write a PNG"}}},
            {"size", {{"value", "MM"}, {"type", "number"}, {"default", 0}, {"help", "the SVG's width and height in mm (default: none, it scales)"}}},
            {"scale", {{"value", "PIXELS"}, {"type", "number"}, {"default", 8}, {"help", "the PNG's pixels a module"}}},
            {"correction", {{"short", "c"}, {"value", "LEVEL"}, {"default", "medium"},
                            {"help", "low, medium, quartile or high: how much damage it survives (7, 15, 25, 30%)"}}}}},
        {"positional", {
            {"text", {{"default", "https://example.com"}, {"help", "what the code holds"}}}}}}};
    try {
        const std::map<std::string, sc::qr::correction> levels{{"low", sc::qr::correction::low}, {"medium", sc::qr::correction::medium},
                                                               {"quartile", sc::qr::correction::quartile}, {"high", sc::qr::correction::high}};
        const auto level = levels.find(args.as<std::string>("correction"));
        if (level == levels.end()) throw std::invalid_argument("--correction is low, medium, quartile or high");

        // [readme]
        const sc::qr::code code{args.as<std::string>("text"), level->second};
        const auto svg = args.as<std::string>("svg"), png = args.as<std::string>("png");
        if (!svg.empty()) {
            const auto mm = args.as<double>("size");
            code.save_svg(svg, mm > 0 ? std::optional<sc::distance>{sc::dist::mm(mm)} : std::nullopt);
        }
        if (!png.empty()) code.save_png(png, args.as<int>("scale"));
        if (svg.empty() && png.empty()) std::cout << code.text();
        std::cout << "Version " << code.version() << ": " << code.size() << " x " << code.size() << " modules\n";
        // [/readme]
        return 0;
    } catch (const std::exception &problem) {
        std::cerr << "sc-qrencode-demo: " << problem.what() << '\n';
    }
    return 1;
}
