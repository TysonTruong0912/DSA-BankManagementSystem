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
namespace UI {
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

}
