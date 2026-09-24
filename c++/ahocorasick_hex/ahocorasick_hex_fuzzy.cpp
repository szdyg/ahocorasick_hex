// Copyright (c) 2024 szdyg
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#include "ahocorasick_hex_fuzzy.h"

#include <algorithm>


ahocorasick_hex_fuzzy::ahocorasick_hex_fuzzy() {
}

ahocorasick_hex_fuzzy::~ahocorasick_hex_fuzzy() {
}


bool ahocorasick_hex_fuzzy::parse(const char* hex, size_t hex_len,
                                  std::vector<uint8_t>& value,
                                  std::vector<uint8_t>& mask) {
    value.clear();
    mask.clear();

    const int NIBBLE_NONE = -2;  // 尚未读到高半字节
    const int NIBBLE_ANY = -1;   // 该半字节为 '?'

    int high = NIBBLE_NONE;

    for (size_t i = 0; i < hex_len; i++) {
        char c = hex[i];

        // 允许 "AE CC 32 56 ?? ??" 这种带分隔的写法
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            continue;
        }

        int nibble = 0;
        if (c == '?') {
            nibble = NIBBLE_ANY;
        }
        else if (c >= '0' && c <= '9') {
            nibble = c - '0';
        }
        else if (c >= 'a' && c <= 'f') {
            nibble = c - 'a' + 10;
        }
        else if (c >= 'A' && c <= 'F') {
            nibble = c - 'A' + 10;
        }
        else {
            return false;  // 非法字符
        }

        if (high == NIBBLE_NONE) {
            high = nibble;
            continue;
        }

        if (high == NIBBLE_ANY && nibble == NIBBLE_ANY) {
            // "??"：整字节通配
            value.push_back(0x00);
            mask.push_back(0x00);
        }
        else if (high == NIBBLE_ANY || nibble == NIBBLE_ANY) {
            // "A?" / "?A"：半字节通配，当前不支持
            return false;
        }
        else {
            value.push_back((uint8_t)((high << 4) | nibble));
            mask.push_back(0xFF);
        }

        high = NIBBLE_NONE;
    }

    if (high != NIBBLE_NONE) {
        return false;  // 半字节个数为奇数
    }

    return !value.empty();
}


bool ahocorasick_hex_fuzzy::add_pattern(const std::string& hex_pattern, size_t pattern_id) {
    return add_pattern(hex_pattern.data(), hex_pattern.size(), pattern_id);
}

bool ahocorasick_hex_fuzzy::add_pattern(const char* hex_pattern, size_t hex_len, size_t pattern_id) {
    if (hex_pattern == nullptr || hex_len == 0 || _finalized) {
        return false;
    }

    if (_ids.count(pattern_id)) {
        return false;  // 编号重复
    }

    pattern pat;
    if (!parse(hex_pattern, hex_len, pat.value, pat.mask)) {
        return false;
    }

    // 选取最长的字面量片段作为锚点；长度相同时取靠前的那个。
    // 锚点越长，AC 的误触发越少，需要做掩码校验的候选就越少。
    size_t best_offset = 0;
    size_t best_len = 0;
    size_t cur_offset = 0;
    size_t cur_len = 0;

    for (size_t i = 0; i < pat.mask.size(); i++) {
        if (pat.mask[i] == 0xFF) {
            if (cur_len == 0) {
                cur_offset = i;
            }
            cur_len++;
            if (cur_len > best_len) {
                best_len = cur_len;
                best_offset = cur_offset;
            }
        }
        else {
            cur_len = 0;
        }
    }

    if (best_len == 0) {
        return false;  // 全是通配符，没有可用来定位的字面量
    }

    pat.anchor_offset = best_offset;
    pat.anchor_len = best_len;
    pat.id = pattern_id;

    size_t index = _patterns.size();
    std::string key((const char*)&pat.value[best_offset], best_len);

    auto it = _anchors.find(key);
    if (it == _anchors.end()) {
        // 只有首次出现的锚点才喂给 AC，以锚点编号作为 pattern_id。
        // 同一锚点重复添加会重复回报命中，这里靠去重规避。
        size_t anchor_id = _anchor_refs.size();
        if (!_ac.add_keyword(&pat.value[best_offset], best_len, anchor_id)) {
            return false;
        }
        _anchors[key] = anchor_id;
        _anchor_refs.push_back({ { index, best_offset } });
    }
    else {
        _anchor_refs[it->second].push_back({ index, best_offset });
    }

    _patterns.push_back(std::move(pat));
    _ids.insert(pattern_id);
    return true;
}


bool ahocorasick_hex_fuzzy::finalize() {
    if (_finalized) {
        return true;  // 幂等：底层 finalize() 重复调用会导致命中被重复回报
    }
    _finalized = true;
    return _ac.finalize();
}


bool ahocorasick_hex_fuzzy::verify(const pattern& pat, uint8_t* data, size_t len, size_t start) const {
    // 用减法做边界检查，避免 start + size() 溢出
    if (start > len || pat.value.size() > len - start) {
        return false;
    }

    for (size_t i = 0; i < pat.value.size(); i++) {
        if ((data[start + i] & pat.mask[i]) != pat.value[i]) {
            return false;
        }
    }
    return true;
}


std::vector<fuzzy_match> ahocorasick_hex_fuzzy::match_all(uint8_t* data, size_t len) {
    std::vector<fuzzy_match> results;

    if (data == nullptr || len == 0 || !_finalized) {
        return results;
    }

    auto hits = _ac.match_all(data, len);

    for (auto& hit : hits) {
        for (auto& ref : _anchor_refs[hit.pattern_id]) {
            if (hit.offset < ref.anchor_offset) {
                continue;  // 模式起点会落到缓冲区之前
            }

            size_t start = hit.offset - ref.anchor_offset;
            const pattern& pat = _patterns[ref.pattern_index];
            if (!verify(pat, data, len, start)) {
                continue;
            }

            fuzzy_match match;
            match.pattern_id = pat.id;
            match.offset = start;
            match.data.assign(data + start, data + start + pat.value.size());
            results.push_back(std::move(match));
        }
    }

    // AC 按锚点结束位置回报，锚点在模式中的偏移又各不相同，
    // 直接输出顺序没有意义。这里统一按模式起始偏移排序。
    std::sort(results.begin(), results.end(),
        [](const fuzzy_match& a, const fuzzy_match& b) {
            if (a.offset != b.offset) {
                return a.offset < b.offset;
            }
            return a.pattern_id < b.pattern_id;
        });

    return results;
}


fuzzy_match ahocorasick_hex_fuzzy::match_one(uint8_t* data, size_t len) {
    auto all = match_all(data, len);
    if (all.empty()) {
        return fuzzy_match();
    }
    return all.front();
}


size_t ahocorasick_hex_fuzzy::pattern_count() const {
    return _patterns.size();
}
