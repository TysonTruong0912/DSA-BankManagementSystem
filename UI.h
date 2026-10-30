#ifndef UI_H
#define UI_H

#include <string>
#include <vector>

// Giao dien console (ANSI color + khung ve)
// Tuong thich Windows 10+ (conhost / Windows Terminal) va Linux / macOS.

namespace UI
{
    // ---- Mau ANSI ----
    extern const char* RESET;
    extern const char* BOLD;
    extern const char* DIM;

    extern const char* CYAN;
    extern const char* BRIGHT_CYAN;
    extern const char* TEAL;
    extern const char* BLUE;
    extern const char* BRIGHT_BLUE;
    extern const char* WHITE;
    extern const char* BRIGHT_WHITE;
    extern const char* GREEN;
    extern const char* BRIGHT_GREEN;
    extern const char* YELLOW;
    extern const char* BRIGHT_YELLOW;
    extern const char* RED;
    extern const char* BRIGHT_RED;
    extern const char* MAGENTA;
    extern const char* GRAY;

    void enableAnsi();          // bat xu ly ANSI tren Windows
    void clearScreen();

    // Ve khung / tieu de
    void drawHeader();          // banner chinh he thong ngan hang
    void drawStatusBar(int soTK, int dangHoatDong, int daDong, double tongSoDu,
                       bool coSqlite, const std::string& thoiGian);
    void drawCards(const std::vector<std::pair<std::string, std::string>>& cards);
    void drawMenuBox(const std::string& title, const std::vector<std::string>& lines);
    void drawSectionTitle(const std::string& title);
    void drawBoxLine(const std::string& content, int width = 72);
    void drawSeparator(int width = 72);

    // Thong bao
    void success(const std::string& msg);
    void error(const std::string& msg);
    void warn(const std::string& msg);
    void info(const std::string& msg);
    void notify(const std::string& title, const std::vector<std::string>& lines);
    void receipt(const std::string& title, const std::vector<std::pair<std::string, std::string>>& rows);

    void title(const std::string& text);
    void prompt(const std::string& text);

    // Menu doc, chon bang mui ten (hoac so). Tra ve chi so 0..n-1.
    // Neu stdin khong phai TTY (pipe/test) -> nhap so nhu cu.
    // defaultIndex: dong dang duoc highlight luc mo menu.
    int selectMenu(const std::string& title,
                   const std::vector<std::string>& options,
                   int defaultIndex = 0);
}

#endif
