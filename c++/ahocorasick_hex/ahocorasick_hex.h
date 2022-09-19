#ifndef _AHOCORASICK_HEX_H_
#define _AHOCORASICK_HEX_H_


#include <stdint.h>
#include <vector>
#include <memory>



class ahocorasick_match
{
public:
    std::vector<uint8_t> keyword;
    size_t offset = 0;
};

class ahocorasick_trie_node
{
public:
    ahocorasick_trie_node();
    ~ahocorasick_trie_node();

public:
    std::shared_ptr<ahocorasick_trie_node> childs[256];     // 子节点
    std::shared_ptr<ahocorasick_trie_node> fail;            // fail指针
    std::vector<size_t> exist_lens;                         // 关键字长度
};

class ahocorasick_hex
{
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