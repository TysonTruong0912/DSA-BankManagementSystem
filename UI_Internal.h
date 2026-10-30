#ifndef UI_INTERNAL_H
#define UI_INTERNAL_H
#include <string>
#include <vector>
namespace UI {
namespace Internal {
    extern const char* FG;
    extern const char* FG_SOFT;
    extern const char* FG_TEXT;
    extern const char* FG_MUTE;
    extern const char* BG_BAR;
    extern const char* BG_SOFT;
    int dispWidth(const std::string& s);
    std::string padRight(const std::string& s, int width);
    std::string center(const std::string& s, int width);
    std::string repeat(const std::string& unit, int n);
    int readKey();
    bool isInteractive();
    void paintVerticalMenu(const std::string& title, const std::vector<std::string>& options, int selected);
}
}
#endif
