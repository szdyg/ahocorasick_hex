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

#ifndef _AHOCORASICK_HEX_FUZZY_H_
#define _AHOCORASICK_HEX_FUZZY_H_

#include <stdint.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "ahocorasick_hex.h"

//
// 带通配符的特征码搜索。
//
// 模式串形如 "AECC3256????CEFF1256"，其中 "??" 表示任意一个字节。
// 允许用空格分隔以提升可读性："AE CC 32 56 ?? ?? CE FF 12 56"。
// 仅支持整字节通配；半字节通配（如 "A?"）会被 add_pattern() 拒绝。
//
// 实现方式为「字面量锚点 + 掩码校验」：把模式按通配符切成若干字面量片段，
// 取其中最长的一段作为锚点喂给 ahocorasick_hex，锚点命中后再反推模式起点，
// 对整个模式做逐字节掩码比对。通配符不会进入 trie，因此不存在分支爆炸。
//

class fuzzy_match {
public:
    size_t pattern_id = 0;      // add_pattern() 时由调用者指定的模式编号
    size_t offset = 0;          // 模式在待匹配数据中的起始偏移
    std::vector<uint8_t> data;  // 命中处的实际字节（通配位为数据中的真实内容）
};

class ahocorasick_hex_fuzzy {
public:
    ahocorasick_hex_fuzzy();
    ~ahocorasick_hex_fuzzy();

    /// <summary>
    /// 添加通配模式，必须在 finalize() 之前调用。
    /// </summary>
    /// <param name="hex_pattern">模式串，如 "AECC3256????CEFF1256"</param>
    /// <param name="pattern_id">模式编号，由调用者指定，命中时原样回填到 fuzzy_match::pattern_id；不可与已添加的模式重复</param>
    /// <returns>成功返回 true；模式串非法、含半字节通配、全为通配符、pattern_id 重复或已 finalize() 时返回 false</returns>
    bool add_pattern(const std::string& hex_pattern, size_t pattern_id);

    /// <summary>
    /// 添加通配模式，必须在 finalize() 之前调用。
    /// </summary>
    /// <param name="hex_pattern">模式串</param>
    /// <param name="hex_len">模式串长度</param>
    /// <param name="pattern_id">模式编号，由调用者指定；不可与已添加的模式重复</param>
    /// <returns>成功返回 true；模式串非法、含半字节通配、全为通配符、pattern_id 重复或已 finalize() 时返回 false</returns>
    bool add_pattern(const char* hex_pattern, size_t hex_len, size_t pattern_id);

    /// <summary>
    /// 构建内部自动机。必须在所有 add_pattern() 之后、任何 match_*() 之前调用。
    /// 与底层 ahocorasick_hex::finalize() 不同，本函数是幂等的，重复调用安全。
    /// </summary>
    /// <returns>成功返回 true</returns>
    bool finalize();

    /// <summary>
    /// 匹配全部命中，调用前必须已 finalize()。
    /// 结果按 (offset, pattern_id) 升序排列。
    /// </summary>
    /// <param name="data">待匹配数据指针</param>
    /// <param name="len">待匹配数据长度</param>
    /// <returns>命中结果列表，未命中或未 finalize() 时为空</returns>
    std::vector<fuzzy_match> match_all(uint8_t* data, size_t len);

    /// <summary>
    /// 匹配起始偏移最小的一个命中，调用前必须已 finalize()。
    /// 注意：锚点命中不等于模式命中，无法提前收敛，内部仍是一次完整扫描，
    /// 开销与 match_all() 相同。
    /// </summary>
    /// <param name="data">待匹配数据指针</param>
    /// <param name="len">待匹配数据长度</param>
    /// <returns>命中结果，未命中时 data 为空、offset 为 0</returns>
    fuzzy_match match_one(uint8_t* data, size_t len);

    /// <summary>
    /// 已添加的模式数量。
    /// </summary>
    size_t pattern_count() const;

private:
    class pattern {
    public:
        size_t id = 0;               // 调用者指定的模式编号
        std::vector<uint8_t> value;  // 通配位置为 0x00
        std::vector<uint8_t> mask;   // 字面量位置为 0xFF，通配位置为 0x00
        size_t anchor_offset = 0;    // 锚点片段在模式中的偏移
        size_t anchor_len = 0;       // 锚点片段长度
    };

    class anchor_ref {
    public:
        size_t pattern_index = 0;    // _patterns 下标，与调用者指定的编号无关
        size_t anchor_offset = 0;
    };

    /// <summary>
    /// 解析十六进制模式串为 value/mask 对。
    /// </summary>
    /// <param name="hex">模式串，如 "AECC3256????CEFF1256"，空格/制表/换行会被忽略；不要求以 '\0' 结尾</param>
    /// <param name="hex_len">模式串长度</param>
    /// <param name="value">输出，字面量位置为对应字节值，通配位置为 0x00</param>
    /// <param name="mask">输出，字面量位置为 0xFF，通配位置为 0x00</param>
    /// <returns>成功返回 true；含非法字符、半字节通配、半字节个数为奇数或解析结果为空时返回 false</returns>
    static bool parse(const char* hex, size_t hex_len, std::vector<uint8_t>& value, std::vector<uint8_t>& mask);

    /// <summary>
    /// 对 data[start .. start + pat.value.size()) 做逐字节掩码比对。
    /// </summary>
    /// <param name="pat">待校验的模式</param>
    /// <param name="data">待匹配数据指针</param>
    /// <param name="len">待匹配数据长度</param>
    /// <param name="start">模式在待匹配数据中的起始偏移</param>
    /// <returns>完全匹配返回 true；越界或任一字节不符返回 false</returns>
    bool verify(const pattern& pat, uint8_t* data, size_t len, size_t start) const;

    ahocorasick_hex _ac;                 // 只装字面量锚点，不含任何通配信息
    std::vector<pattern> _patterns;
    // 锚点字节串 -> 锚点编号（即喂给 _ac 的 pattern_id），仅在 add_pattern() 时用于锚点去重。
    std::unordered_map<std::string, size_t> _anchors;
    // 锚点编号 -> 引用它的模式列表。多个模式可以共用同一锚点。
    std::vector<std::vector<anchor_ref>> _anchor_refs;
    std::unordered_set<size_t> _ids;     // 已占用的模式编号，用于拒绝重复
    bool _finalized = false;
};

#endif
