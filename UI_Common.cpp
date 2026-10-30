#include "UI.h"
#include "UI_Internal.h"
#include <cctype>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <conio.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#endif
using namespace std;
using namespace UI::Internal;

namespace UI
{
    const char* RESET         = "\033[0m";
    const char* BOLD          = "\033[1m";
    const char* DIM           = "\033[2m";
    const char* CYAN          = "\033[36m";
    const char* BRIGHT_CYAN   = "\033[96m";
    const char* TEAL          = "\033[38;5;44m";
    const char* BLUE          = "\033[34m";
    const char* BRIGHT_BLUE   = "\033[94m";
    const char* WHITE         = "\033[37m";
    const char* BRIGHT_WHITE  = "\033[97m";
    const char* GREEN         = "\033[32m";
    const char* BRIGHT_GREEN  = "\033[92m";
    const char* YELLOW        = "\033[33m";
    const char* BRIGHT_YELLOW = "\033[93m";
    const char* RED           = "\033[31m";
    const char* BRIGHT_RED    = "\033[91m";
    const char* MAGENTA       = "\033[35m";
    const char* GRAY          = "\033[90m";

    void enableAnsi()
    {
#ifdef _WIN32
        SetConsoleOutputCP(65001);
        SetConsoleCP(65001);
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE)
        {
            DWORD mode = 0;
            if (GetConsoleMode(hOut, &mode))
            {
                mode |= 0x0004;
                SetConsoleMode(hOut, mode);
            }
        }
#endif
    }

    void clearScreen()
    {
#ifdef _WIN32
        system("cls");
#else
        cout << "\033[2J\033[H";
#endif
    }

    // Do rong hien thi UTF-8 (moi code point = 1 cot; bo qua combining)

namespace Internal {
    const char* FG       = "\033[96m";
    const char* FG_SOFT  = "\033[36m";
    const char* FG_TEXT  = "\033[97m";
    const char* FG_MUTE  = "\033[90m";
    const char* BG_BAR   = "\033[48;5;23m";
    const char* BG_SOFT  = "\033[48;5;17m";
    int dispWidth(const string& s)
    {
        int w = 0;
        for (size_t i = 0; i < s.size(); )
        {
            unsigned char c = (unsigned char)s[i];
            if (c < 0x80) { ++w; ++i; }
            else if ((c & 0xE0) == 0xC0) { ++w; i += 2; }
            else if ((c & 0xF0) == 0xE0) { ++w; i += 3; }
            else if ((c & 0xF8) == 0xF0) { ++w; i += 4; }
            else { ++i; }
        }
        return w;
    }

    string padRight(const string& s, int width)
    {
        int w = dispWidth(s);
        if (w >= width) return s;
        return s + string(width - w, ' ');
    }

    string center(const string& s, int width)
    {
        int w = dispWidth(s);
        if (w >= width) return s;
        int left = (width - w) / 2;
        int right = width - w - left;
        return string(left, ' ') + s + string(right, ' ');
    }

    string repeat(const string& unit, int n)
    {
        string r;
        for (int i = 0; i < n; ++i) r += unit;
        return r;
    }

    int readKey()
    {
#ifdef _WIN32
        int c = _getch();
        if (c == 0 || c == 224)   // phim mo rong
        {
            int c2 = _getch();
            if (c2 == 72) return 1000; // UP
            if (c2 == 80) return 1001; // DOWN
            if (c2 == 75) return 1002; // LEFT
            if (c2 == 77) return 1003; // RIGHT
            return c2;
        }
        return c;
#else
        struct termios oldt, newt;
        if (tcgetattr(STDIN_FILENO, &oldt) != 0) return -1;
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        unsigned char c = 0;
        if (read(STDIN_FILENO, &c, 1) != 1)
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            return -1;
        }
        if (c == 27) // ESC sequence
        {
            unsigned char seq[2] = {0, 0};
            // timeout ngan
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(STDIN_FILENO, &fds);
            struct timeval tv = {0, 100000};
            if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0) {
                ssize_t _r1 = read(STDIN_FILENO, &seq[0], 1); (void)_r1;
            }
            tv.tv_usec = 100000;
            FD_ZERO(&fds);
            FD_SET(STDIN_FILENO, &fds);
            if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0) {
                ssize_t _r2 = read(STDIN_FILENO, &seq[1], 1); (void)_r2;
            }
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            if (seq[0] == '[')
            {
                if (seq[1] == 'A') return 1000; // UP
                if (seq[1] == 'B') return 1001; // DOWN
                if (seq[1] == 'C') return 1003;
                if (seq[1] == 'D') return 1002;
            }
            return 27;
        }
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return (int)c;
#endif
    }

    bool isInteractive()
    {
#ifdef _WIN32
        return true;
#else
        return isatty(STDIN_FILENO) != 0;
#endif
    }

    // Ve 1 lan menu doc: dong dang chon highlight nen
    void paintVerticalMenu(const string& title, const vector<string>& options, int selected)
    {
        const int INNER = 56;
        cout << "\n";
        cout << "  " << FG << "+" << repeat("=", INNER + 2) << "+" << RESET << "\n";
        cout << "  " << FG << "|" << RESET
             << BG_BAR << FG_TEXT << BOLD << center(title, INNER + 2) << RESET
             << FG << "|" << RESET << "\n";
        cout << "  " << FG << "+" << repeat("=", INNER + 2) << "+" << RESET << "\n";
        cout << "  " << FG << "|" << RESET << string(INNER + 2, ' ') << FG << "|" << RESET << "\n";

        for (size_t i = 0; i < options.size(); ++i)
        {
            string label = options[i];
            while (dispWidth(label) > INNER - 6 && !label.empty())
                label.erase(label.size() - 1);

            cout << "  " << FG << "|" << RESET;
            if ((int)i == selected)
            {
                // dong dang chon: nen + mui ten
                string body = "  > " + label;
                cout << BG_BAR << BRIGHT_YELLOW << BOLD
                     << padRight(body, INNER + 2) << RESET;
            }
            else
            {
                string body = "    " + label;
                cout << FG_TEXT << padRight(body, INNER + 2) << RESET;
            }
            cout << FG << "|" << RESET << "\n";
        }

        cout << "  " << FG << "|" << RESET << string(INNER + 2, ' ') << FG << "|" << RESET << "\n";
        cout << "  " << FG << "+" << repeat("=", INNER + 2) << "+" << RESET << "\n";
        cout << "  " << FG_MUTE
             << "  Len/Xuong: di chuyen   Enter: chon   so 1-" << options.size() << ": nhay nhanh"
             << RESET << "\n";
        cout.flush();
    }


} // Internal
} // UI
