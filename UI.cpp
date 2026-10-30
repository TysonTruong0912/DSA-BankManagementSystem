#include "UI.h"

#include <cctype>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#endif

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using namespace std;

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

    static const char* FG       = "\033[96m";
    static const char* FG_SOFT  = "\033[36m";
    static const char* FG_TEXT  = "\033[97m";
    static const char* FG_MUTE  = "\033[90m";
    static const char* BG_BAR   = "\033[48;5;23m";
    static const char* BG_SOFT  = "\033[48;5;17m";

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
    static int dispWidth(const string& s)
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

    static string padRight(const string& s, int width)
    {
        int w = dispWidth(s);
        if (w >= width) return s;
        return s + string(width - w, ' ');
    }

    static string center(const string& s, int width)
    {
        int w = dispWidth(s);
        if (w >= width) return s;
        int left = (width - w) / 2;
        int right = width - w - left;
        return string(left, ' ') + s + string(right, ' ');
    }

    static string repeat(const string& unit, int n)
    {
        string r;
        for (int i = 0; i < n; ++i) r += unit;
        return r;
    }

    void drawHeader()
    {
        const int W = 60;
        cout << "\n";
        cout << "  " << FG << "+" << repeat("=", W) << "+" << RESET << "\n";
        cout << "  " << FG << "|" << RESET
             << BG_BAR << FG_TEXT << BOLD
             << center("NEXUS BANK", W)
             << RESET << FG << "|" << RESET << "\n";
        cout << "  " << FG << "|" << RESET
             << BG_SOFT << FG
             << center("He thong quan ly tai khoan & giao dich", W)
             << RESET << FG << "|" << RESET << "\n";
        cout << "  " << FG << "|" << RESET
             << BG_SOFT << FG_MUTE
             << center("Tai khoan  |  Giao dich  |  Bao mat  |  Thong ke", W)
             << RESET << FG << "|" << RESET << "\n";
        cout << "  " << FG << "+" << repeat("=", W) << "+" << RESET << "\n";
    }

    void drawStatusBar(int soTK, int dangHoatDong, int daDong, double tongSoDu,
                       bool coSqlite, const string& thoiGian)
    {
        const int W = 60;
        string left  = "Nexus Bank";
        string mid   = string("  ") + (coSqlite ? "SQLite - tu dong luu" : "Che do file .txt");
        string right = thoiGian;
        int used = dispWidth(left) + dispWidth(mid) + dispWidth(right);
        int space = W - used;
        if (space < 1) space = 1;

        cout << "  " << FG_TEXT << left
             << FG_MUTE << mid
             << string(space, ' ')
             << FG_MUTE << right
             << RESET << "\n";
        cout << "  " << FG << repeat("-", W) << RESET << "\n\n";

        ostringstream money;
        money.imbue(locale::classic());
        if (tongSoDu >= 1e9)
            money << fixed << setprecision(1) << (tongSoDu / 1e9) << " ty";
        else if (tongSoDu >= 1e6)
            money << fixed << setprecision(1) << (tongSoDu / 1e6) << " tr";
        else
            money << (long long)tongSoDu;

        ostringstream c1, c2, c3, c4;
        c1 << soTK << " TK";
        c2 << dangHoatDong << " dang mo";
        c3 << daDong << " da dong";
        c4 << money.str() << " VND";

        vector<pair<string, string> > cards;
        cards.push_back(make_pair("Tong tai khoan", c1.str()));
        cards.push_back(make_pair("Dang hoat dong", c2.str()));
        cards.push_back(make_pair("Da dong",        c3.str()));
        cards.push_back(make_pair("Tong so du",     c4.str()));
        drawCards(cards);
        cout << "\n";
    }

    void drawCards(const vector<pair<string, string> >& cards)
    {
        // 1 card = 1 dong, khung ASCII, khong lech khi UTF-8 / Windows console
        const int INNER = 56;
        for (size_t i = 0; i < cards.size(); ++i)
        {
            string label = cards[i].first;
            string value = cards[i].second;
            // "Label : value" can le
            string line = label + ": " + value;
            while (dispWidth(line) > INNER)
            {
                if (value.empty()) break;
                value.erase(value.size() - 1);
                line = label + ": " + value;
            }
            cout << "  " << FG << "+-" << repeat("-", INNER) << "-+" << RESET << "\n";
            cout << "  " << FG << "| " << RESET
                 << FG_MUTE << padRight(label, 16) << RESET
                 << FG_TEXT << BOLD << padRight(value, INNER - 16) << RESET
                 << FG << " |" << RESET << "\n";
        }
        if (!cards.empty())
            cout << "  " << FG << "+-" << repeat("-", INNER) << "-+" << RESET << "\n";
    }

    void drawMenuBox(const string& title, const vector<string>& lines)
    {
        // Khung co dinh, can le bang dispWidth (UTF-8) -> khong lech cot
        const int INNER = 56;  // do rong noi dung trong khung
        cout << "\n";
        cout << "  " << FG << "+" << repeat("-", INNER + 2) << "+" << RESET << "\n";
        cout << "  " << FG << "|" << RESET
             << BG_BAR << FG_TEXT << BOLD << center(title, INNER + 2) << RESET
             << FG << "|" << RESET << "\n";
        cout << "  " << FG << "+" << repeat("-", INNER + 2) << "+" << RESET << "\n";

        for (size_t i = 0; i < lines.size(); ++i)
        {
            string line = lines[i];
            // Truncate neu qua dai
            while (dispWidth(line) > INNER)
            {
                if (line.empty()) break;
                // cat 1 codepoint cuoi
                size_t j = line.size();
                unsigned char c = (unsigned char)line[j - 1];
                if (c < 0x80) line.erase(j - 1);
                else if (j >= 2 && (c & 0xC0) == 0x80)
                {
                    while (j > 0 && ((unsigned char)line[j - 1] & 0xC0) == 0x80) --j;
                    if (j > 0) --j;
                    line.erase(j);
                }
                else line.erase(j - 1);
            }
            bool blank = (line.find_first_not_of(" \t") == string::npos);
            bool exitItem = (line.find("[0]") != string::npos);
            cout << "  " << FG << "| " << RESET;
            if (blank)
                cout << string(INNER, ' ');
            else if (exitItem)
                cout << FG << BOLD << padRight(line, INNER) << RESET;
            else
                cout << FG_TEXT << padRight(line, INNER) << RESET;
            cout << FG << " |" << RESET << "\n";
        }
        cout << "  " << FG << "+" << repeat("-", INNER + 2) << "+" << RESET << "\n";
    }

    void drawSectionTitle(const string& title)
    {
        cout << "\n  " << FG << "── " << RESET
             << FG_TEXT << BOLD << title << RESET
             << FG << " ──" << RESET << "\n";
    }

    void drawBoxLine(const string& content, int width)
    {
        cout << "  " << FG << "│ " << RESET
             << padRight(content, width - 2)
             << FG << " │" << RESET << "\n";
    }

    void drawSeparator(int width)
    {
        cout << "  " << FG << repeat("─", width) << RESET << "\n";
    }

    void success(const string& msg)
    {
        cout << "  " << FG << "[" << BRIGHT_GREEN << "OK" << FG << "] " << RESET
             << FG_TEXT << msg << RESET << "\n";
    }

    void error(const string& msg)
    {
        cout << "  " << FG << "[" << BRIGHT_RED << "LỖI" << FG << "] " << RESET
             << BRIGHT_RED << msg << RESET << "\n";
    }

    void warn(const string& msg)
    {
        cout << "  " << FG << "[" << BRIGHT_YELLOW << "!" << FG << "] " << RESET
             << BRIGHT_YELLOW << msg << RESET << "\n";
    }

    void info(const string& msg)
    {
        cout << "  " << FG << "[" << FG_SOFT << "i" << FG << "] " << RESET
             << FG_SOFT << msg << RESET << "\n";
    }

    void title(const string& text)
    {
        const int INNER = 56;
        cout << "\n";
        cout << "  " << FG << "+" << repeat("-", INNER + 2) << "+" << RESET << "\n";
        cout << "  " << FG << "|" << RESET
             << BG_BAR << FG_TEXT << BOLD << center(text, INNER + 2) << RESET
             << FG << "|" << RESET << "\n";
        cout << "  " << FG << "+" << repeat("-", INNER + 2) << "+" << RESET << "\n\n";
    }

    void prompt(const string& text)
    {
        cout << "  " << FG << "> " << RESET << FG_TEXT << text << RESET;
    }

    void notify(const string& title, const vector<string>& lines)
    {
        const int INNER = 56;
        cout << "\n  " << FG << "+" << repeat("-", INNER + 2) << "+" << RESET << "\n";
        cout << "  " << FG << "|" << RESET
             << BG_BAR << BRIGHT_YELLOW << BOLD << padRight(" [*] " + title, INNER + 2) << RESET
             << FG << "|" << RESET << "\n";
        cout << "  " << FG << "+" << repeat("-", INNER + 2) << "+" << RESET << "\n";
        if (lines.empty())
        {
            cout << "  " << FG << "| " << RESET << FG_MUTE
                 << padRight("(khong co thong bao moi)", INNER)
                 << FG << " |" << RESET << "\n";
        }
        else
        {
            for (size_t i = 0; i < lines.size(); ++i)
            {
                string line = lines[i];
                while (dispWidth(line) > INNER && !line.empty())
                    line.erase(line.size() - 1);
                cout << "  " << FG << "| " << RESET << FG_TEXT
                     << padRight(line, INNER)
                     << FG << " |" << RESET << "\n";
            }
        }
        cout << "  " << FG << "+" << repeat("-", INNER + 2) << "+" << RESET << "\n\n";
    }

    void receipt(const string& title, const vector<pair<string, string> >& rows)
    {
        const int INNER = 56;
        cout << "\n  " << FG << "+" << repeat("-", INNER + 2) << "+" << RESET << "\n";
        cout << "  " << FG << "|" << RESET
             << BG_BAR << BRIGHT_GREEN << BOLD << center(title, INNER + 2) << RESET
             << FG << "|" << RESET << "\n";
        cout << "  " << FG << "+" << repeat("-", INNER + 2) << "+" << RESET << "\n";
        for (size_t i = 0; i < rows.size(); ++i)
        {
            string line = rows[i].first + ": " + rows[i].second;
            while (dispWidth(line) > INNER && !line.empty())
                line.erase(line.size() - 1);
            cout << "  " << FG << "| " << RESET << FG_TEXT
                 << padRight(line, INNER)
                 << FG << " |" << RESET << "\n";
        }
        cout << "  " << FG << "+" << repeat("-", INNER + 2) << "+" << RESET << "\n\n";
    }


    // ---- Doc 1 phim (Windows / Linux). -1 = khong doc duoc ----
    static int readKey()
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
            fd_set fds;
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100000;
            FD_ZERO(&fds);
            FD_SET(STDIN_FILENO, &fds);
            if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0)
            {
                if (read(STDIN_FILENO, &seq[0], 1) != 1)
                    seq[0] = 0;
            }
            tv.tv_sec = 0;
            tv.tv_usec = 100000;
            FD_ZERO(&fds);
            FD_SET(STDIN_FILENO, &fds);
            if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0)
            {
                if (read(STDIN_FILENO, &seq[1], 1) != 1)
                    seq[1] = 0;
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

    static bool isInteractive()
    {
#ifdef _WIN32
        return _isatty(_fileno(stdin)) != 0;
#else
        return isatty(STDIN_FILENO) != 0;
#endif
    }

    // ---- Menu doc: khung CO DINH 1 cho, khong xuong dong lech ----
    // INNER = do rong chu ben trong (giua 2 dau | )
    // Moi dong in dung: "  |" + INNER ky tu + "|"  => tong ~ 2+1+INNER+1 = INNER+4 cot
    static const int MENU_INNER = 50;

    static string clipDisp(const string& s, int maxW)
    {
        if (dispWidth(s) <= maxW) return s;
        string out;
        int w = 0;
        for (size_t i = 0; i < s.size(); )
        {
            unsigned char c = (unsigned char)s[i];
            size_t n = 1;
            if (c < 0x80) n = 1;
            else if ((c & 0xE0) == 0xC0) n = 2;
            else if ((c & 0xF0) == 0xE0) n = 3;
            else if ((c & 0xF8) == 0xF0) n = 4;
            if (i + n > s.size()) break;
            if (w + 1 > maxW) break;
            out.append(s, i, n);
            ++w;
            i += n;
        }
        return out;
    }

    // In 1 dong khung: 2 space + | + dung INNER ky tu hien thi + |
    static void menuRow(const string& innerContent, bool highlight)
    {
        string body = padRight(clipDisp(innerContent, MENU_INNER), MENU_INNER);
        cout << "  " << FG << "|" << RESET;
        if (highlight)
            cout << BG_BAR << BRIGHT_YELLOW << BOLD << body << RESET;
        else
            cout << body;
        cout << FG << "|" << RESET << "\n";
    }

    static void menuBorder(char fill)
    {
        cout << "  " << FG << "+" << string(MENU_INNER, fill) << "+" << RESET << "\n";
    }

    // So dong thuc te paintVerticalMenu in ra (moi dong ket thuc \n)
    static int menuFrameLineCount(int optionCount)
    {
        // border + title + border + blank + opts + blank + border + hint = 6 + opts + 1 blank? 
        // border, title, border, blank, N opts, blank, border, hint = N + 7
        return optionCount + 7;
    }

    static void paintVerticalMenu(const string& title, const vector<string>& options, int selected)
    {
        menuBorder('=');
        {
            string t = clipDisp(title, MENU_INNER);
            string centered = center(t, MENU_INNER);
            cout << "  " << FG << "|" << RESET
                 << BG_BAR << FG_TEXT << BOLD << centered << RESET
                 << FG << "|" << RESET << "\n";
        }
        menuBorder('=');
        menuRow("", false);

        for (size_t i = 0; i < options.size(); ++i)
        {
            string label = clipDisp(options[i], MENU_INNER - 3);
            if ((int)i == selected)
                menuRow(" > " + label, true);
            else
                menuRow("   " + label, false);
        }

        menuRow("", false);
        menuBorder('=');
        {
            ostringstream hs;
            hs << "Len/Xuong | Enter chon | 1-" << options.size() << " nhay";
            cout << "  " << FG_MUTE
                 << padRight(clipDisp(hs.str(), MENU_INNER + 2), MENU_INNER + 2)
                 << RESET << "\n";
        }
        cout.flush();
    }

    int selectMenu(const string& title, const vector<string>& options, int defaultIndex)
    {
        if (options.empty()) return -1;
        int selected = defaultIndex;
        if (selected < 0) selected = 0;
        if (selected >= (int)options.size()) selected = (int)options.size() - 1;

        if (!isInteractive())
        {
            drawMenuBox(title, options);
            cout << "  " << FG << "> " << RESET << FG_TEXT
                 << "Nhap so (1-" << options.size() << "): " << RESET;
            string line;
            if (!getline(cin, line)) return -1;
            size_t a = 0, b = line.size();
            while (a < b && isspace((unsigned char)line[a])) ++a;
            while (b > a && isspace((unsigned char)line[b - 1])) --b;
            line = line.substr(a, b - a);
            if (line.empty()) return selected;
            int n = atoi(line.c_str());
            for (size_t i = 0; i < options.size(); ++i)
            {
                size_t p = 0;
                while (p < options[i].size() && isspace((unsigned char)options[i][p])) ++p;
                if (p < options[i].size() && isdigit((unsigned char)options[i][p]))
                {
                    int optNum = atoi(options[i].c_str() + p);
                    if (optNum == n) return (int)i;
                }
            }
            if (n >= 1 && n <= (int)options.size()) return n - 1;
            return -1;
        }

        // Luu vi tri con tro TRUOC khi ve menu -> moi lan ve lai dung 1 cho
        cout << "\033[s";  // save cursor
        paintVerticalMenu(title, options, selected);
        const int frameLines = menuFrameLineCount((int)options.size());

        while (true)
        {
            int key = readKey();
            if (key < 0) return selected;

            bool moved = false;
            if (key == 1000)
            {
                selected = (selected - 1 + (int)options.size()) % (int)options.size();
                moved = true;
            }
            else if (key == 1001)
            {
                selected = (selected + 1) % (int)options.size();
                moved = true;
            }
            else if (key == '\r' || key == '\n')
            {
                // xuong duoi khung roi thoat (con tro o cuoi hint)
                return selected;
            }
            else if (key >= '1' && key <= '9')
            {
                int n = key - '0';
                if (n >= 1 && n <= (int)options.size())
                    return n - 1;
            }

            if (moved)
            {
                // ve lai dung cho da luu — khong cuon, khong lech
                cout << "\033[u";  // restore cursor
                cout << "\033[J";  // xoa tu con tro den het phia duoi
                paintVerticalMenu(title, options, selected);
                (void)frameLines;
            }
        }
    }



}
