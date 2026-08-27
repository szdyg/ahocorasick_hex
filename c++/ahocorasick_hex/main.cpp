#include <iostream>
#include <string>
#include <vector>

#include "ahocorasick_hex.h"
#include "ahocorasick_hex_fuzzy.h"
using namespace std;

static string to_hex(const vector<uint8_t>& data)
{
    static const char* digits = "0123456789ABCDEF";
    string s;
    for (auto b : data)
    {
        s += digits[b >> 4];
        s += digits[b & 0x0F];
    }
    return s;
}

int main()
{
    string s1 = "abc";
    string s2 = "cmd";
    string s3 = "gcc";
    string s4 = "saiodcnasiabcmdsdjigccmd";
    ahocorasick_hex ac;
    ac.add_keyword(s1.c_str(), s1.length());
    ac.add_keyword(s2.c_str(), s2.length());
    ac.add_keyword(s3.c_str(), s3.length());
    ac.finalize();

    auto matchs = ac.match_all(s4.c_str(), s4.length());

    for (auto macth : matchs)
    {
        macth.keyword.push_back('\0');
        string key = (char*)macth.keyword.data();
        cout << "key:" << key << " pos:" << macth.offset << endl;
    }
    cout << "------------------------------------" << endl;
    auto one = ac.match_one(s4.c_str(), s4.length());
    one.keyword.push_back('\0');
    string key = (char*)one.keyword.data();
    cout << "key:" << key << " pos:" << one.offset << endl;

    // 通配模式匹配："??" 表示任意一个字节
    cout << "------------------------------------" << endl;
    ahocorasick_hex_fuzzy fuzzy;
    fuzzy.add_pattern("AECC3256????CEFF1256", 1001);  // 编号由调用者指定
    fuzzy.finalize();

    vector<uint8_t> buf = {
        0x11, 0x22, 0x33, 0x44,
        0xAE, 0xCC, 0x32, 0x56,   // 锚点
        0xAB, 0x12,               // 通配位，任意值
        0xCE, 0xFF, 0x12, 0x56,
        0x99, 0x88,
    };

    for (auto& m : fuzzy.match_all(buf.data(), buf.size()))
    {
        cout << "pattern:" << m.pattern_id
             << " pos:" << m.offset
             << " data:" << to_hex(m.data) << endl;
    }

    return 0;
}