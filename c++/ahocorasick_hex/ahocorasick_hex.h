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

    bool add_keyword(uint8_t* data, size_t len);
    bool add_keyword(const char* str, size_t len);
    ahocorasick_match match_one(uint8_t* data, size_t len);
    ahocorasick_match match_one(const char* str, size_t len);
    std::vector<ahocorasick_match> match_all(uint8_t* data, size_t len);
    std::vector<ahocorasick_match> match_all(const char* str, size_t len);
    bool finalize();

private:
    std::shared_ptr<ahocorasick_trie_node> _trie_root;
};

#endif
