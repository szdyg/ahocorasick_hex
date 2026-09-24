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

#ifndef _AHOCORASICK_HEX_H_
#define _AHOCORASICK_HEX_H_

#include <stdint.h>
#include <vector>
#include <memory>


class ahocorasick_match {
public:
    std::vector<uint8_t> keyword;
    size_t offset = 0;
};

class ahocorasick_trie_node {
public:
    ahocorasick_trie_node();
    ~ahocorasick_trie_node();

    std::shared_ptr<ahocorasick_trie_node> childs[256]; // 子节点
    ahocorasick_trie_node* fail = nullptr; // fail指针
    std::vector<size_t> exist_lens; // 关键字长度
};

class ahocorasick_hex {
public:
    ahocorasick_hex();
    ~ahocorasick_hex();

    // 禁止拷贝：_trie_root 是 shared_ptr，默认拷贝只会复制指针，两个对象共用同一棵 trie，
    // 一方 add_keyword() 会影响另一方，各自 finalize() 一次也等于对同一棵树重复 finalize()。
    ahocorasick_hex(const ahocorasick_hex&) = delete;
    ahocorasick_hex& operator=(const ahocorasick_hex&) = delete;

    /// <summary>
    /// 添加关键字，必须在 finalize() 之前调用；finalize() 之后再调用行为未定义。
    /// </summary>
    /// <param name="data">关键字数据指针</param>
    /// <param name="len">关键字长度</param>
    /// <returns>成功返回 true，data 为空或 len 为 0 返回 false</returns>
    bool add_keyword(uint8_t* data, size_t len);

    /// <summary>
    /// 添加关键字，必须在 finalize() 之前调用；finalize() 之后再调用行为未定义。
    /// </summary>
    /// <param name="str">关键字字符串</param>
    /// <param name="len">关键字长度</param>
    /// <returns>成功返回 true，str 为空或 len 为 0 返回 false</returns>
    bool add_keyword(const char* str, size_t len);

    /// <summary>
    /// 匹配第一个命中的关键字，调用前必须已 finalize()。
    /// </summary>
    /// <param name="data">待匹配数据指针</param>
    /// <param name="len">待匹配数据长度</param>
    /// <returns>命中结果，未命中时 keyword 为空、offset 为 0</returns>
    ahocorasick_match match_one(uint8_t* data, size_t len);

    /// <summary>
    /// 匹配第一个命中的关键字，调用前必须已 finalize()。
    /// </summary>
    /// <param name="str">待匹配字符串</param>
    /// <param name="len">待匹配字符串长度</param>
    /// <returns>命中结果，未命中时 keyword 为空、offset 为 0</returns>
    ahocorasick_match match_one(const char* str, size_t len);

    /// <summary>
    /// 匹配全部命中的关键字，调用前必须已 finalize()。
    /// </summary>
    /// <param name="data">待匹配数据指针</param>
    /// <param name="len">待匹配数据长度</param>
    /// <returns>命中结果列表，未命中时为空</returns>
    std::vector<ahocorasick_match> match_all(uint8_t* data, size_t len);

    /// <summary>
    /// 匹配全部命中的关键字，调用前必须已 finalize()。
    /// </summary>
    /// <param name="str">待匹配字符串</param>
    /// <param name="len">待匹配字符串长度</param>
    /// <returns>命中结果列表，未命中时为空</returns>
    std::vector<ahocorasick_match> match_all(const char* str, size_t len);

    /// <summary>
    /// 构建 fail 指针。必须在所有 add_keyword() 之后、任何 match_*() 之前调用，且只能调用一次；
    /// 重复调用会导致匹配结果重复，调用后再 add_keyword() 不会为新关键字构建 fail 链。
    /// </summary>
    /// <returns>始终返回 true</returns>
    bool finalize();

private:
    std::shared_ptr<ahocorasick_trie_node> _trie_root;
};

#endif
