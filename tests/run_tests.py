#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Bo kiem thu Nexus Bank — chay: python3 tests/run_tests.py (tu thu muc goc du an)."""
from __future__ import print_function
import os, sys, subprocess, tempfile, shutil, re

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
BANK = os.path.join(ROOT, "bank")
if not os.path.isfile(BANK):
    BANK = os.path.join(ROOT, "bank.exe")

passed = failed = 0

def ok(name, cond, detail=""):
    global passed, failed
    if cond:
        passed += 1
        print("[PASS] " + name)
    else:
        failed += 1
        print("[FAIL] " + name + ((" — " + detail) if detail else ""))

def run_bank(inputs, cwd):
    """inputs: list of lines to feed stdin. Returns (stdout, returncode)."""
    text = "\n".join(inputs) + "\n"
    env = os.environ.copy()
    env["TERM"] = "dumb"
    p = subprocess.Popen(
        [BANK], cwd=cwd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, env=env)
    out, _ = p.communicate(text.encode("utf-8"), timeout=30)
    return out.decode("utf-8", errors="replace"), p.returncode

def strip_ansi(s):
    return re.sub(r"\x1b\[[0-9;]*[a-zA-Z]", "", s)

def main():
    global passed, failed
    if not os.path.isfile(BANK):
        print("Khong tim thay binary 'bank' hoac 'bank.exe'. Hay bien dich truoc (make).")
        sys.exit(2)

    # --- 1. Hash Table basic via binary behavior (startup creates sample) ---
    td = tempfile.mkdtemp(prefix="nbank_test_")
    try:
        # Clean start
        out, rc = run_bank(["3"], td)  # exit immediately
        out = strip_ansi(out)
        ok("Khoi dong tao CSDL / du lieu mau", "NEXUS BANK" in out or "tai khoan" in out.lower() or "CSDL" in out)
        ok("Khong in mat khau admin tren man hinh chao", "admin123" not in out)
        ok("Khong in PIN mau cong khai (123456) tren man hinh chao",
           "PIN: 123456" not in out and "PIN mac dinh: 123456" not in out)

        # Extract a sample account if listed
        tks = re.findall(r"\b(\d{10})\b", out)
        ok("Co so tai khoan mau (10 chu so)", len(tks) >= 1, "found=%d" % len(tks))

        if tks:
            tk = tks[0]
            # Login customer wrong PIN x3
            out2, _ = run_bank(["1", tk, "0000", "0000", "0000", "3"], td)
            out2 = strip_ansi(out2)
            ok("Sai PIN 3 lan -> khoa", "KHOA" in out2 or "khoa" in out2.lower())

            # Admin unlock (need admin login)
            out3, _ = run_bank([
                "2", "admin", "admin123",
                "15", tk, "1", "y",  # mo khoa
                "21",  # dang xuat
                "3"
            ], td)
            out3 = strip_ansi(out3)
            ok("Admin dang nhap thanh cong", "Admin thanh cong" in out3 or "QUAN TRI" in out3 or "thanh cong" in out3.lower())
            ok("Admin mo khoa PIN", "mo khoa" in out3.lower() or "Da mo khoa" in out3 or "OK" in out3)

            # Correct login after unlock
            out4, _ = run_bank(["1", tk, "123456", "17", "3"], td)
            out4 = strip_ansi(out4)
            ok("Dang nhap khach dung PIN sau mo khoa", "Xin chao" in out4 or "MENU KHACH" in out4)

        # Transfer test if 2 accounts
        if len(tks) >= 2:
            a, b = tks[0], tks[1]
            # ensure unlocked - admin unlock both
            run_bank(["2","admin","admin123","15",a,"1","y","15",b,"1","y","21","3"], td)
            out5, _ = run_bank([
                "1", a, "123456",
                "5", b, "50000", "test", "y", "123456",
                "",  # enter pause
                "17", "3"
            ], td)
            out5 = strip_ansi(out5)
            ok("Chuyen khoan thanh cong", "OK" in out5 and ("gui den" in out5.lower() or "thanh cong" in out5.lower() or "100" in out5 or "50,000" in out5 or "50000" in out5))

        # Persistence: restart and check bank.db exists
        ok("Tao file bank.db", os.path.isfile(os.path.join(td, "bank.db")))

        # Restart loads data
        out6, _ = run_bank(["3"], td)
        out6 = strip_ansi(out6)
        ok("Khoi dong lai doc du lieu tu CSDL", "doc du lieu" in out6.lower() or "CSDL" in out6 or "tai khoan" in out6.lower())

        # Security: hash module exists - compile-time already; check no plain PIN in db if possible
        import sqlite3
        dbpath = os.path.join(td, "bank.db")
        if os.path.isfile(dbpath):
            conn = sqlite3.connect(dbpath)
            rows = conn.execute("SELECT pin_hash, pin_salt FROM accounts LIMIT 1").fetchone()
            if rows:
                ok("PIN luu dang hash (khong rong)", len(rows[0]) > 10 and len(rows[1]) > 0)
                ok("PIN hash khong bang PIN goc", rows[0] != "123456")
            # money_requests table exists
            tables = [r[0] for r in conn.execute("SELECT name FROM sqlite_master WHERE type='table'").fetchall()]
            ok("Bang money_requests ton tai trong schema", "money_requests" in tables)
            conn.close()

    finally:
        shutil.rmtree(td, ignore_errors=True)

    print("")
    print("Ket qua: %d PASS, %d FAIL (tong %d)" % (passed, failed, passed + failed))
    return 0 if failed == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
