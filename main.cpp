#include <iostream>
#include "BankSystem.h"
#include "UI.h"

int main()
{
    try
    {
        UI::enableAnsi();
        BankSystem bank;
        bank.khoiTao();
        bank.menu();
    }
    catch (const InputClosed&)
    {
        std::cout << "\nDong nhap da dong. Ket thuc chuong trinh.\n";
    }
    return 0;
}
