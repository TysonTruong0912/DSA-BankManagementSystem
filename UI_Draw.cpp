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


}
