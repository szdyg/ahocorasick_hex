#include <iostream>
#include <string>

#include "ahocorasick_hex.h"
using namespace std;

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

    return 0;
}