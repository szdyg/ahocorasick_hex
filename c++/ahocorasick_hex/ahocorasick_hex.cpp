
#include "ahocorasick_hex.h"
#include <queue>

ahocorasick_trie_node::ahocorasick_trie_node()
{
}

ahocorasick_trie_node::~ahocorasick_trie_node()
{
}


ahocorasick_hex::ahocorasick_hex()
{
    _trie_root = std::make_shared<ahocorasick_trie_node>();
}

ahocorasick_hex::~ahocorasick_hex()
{
}



bool ahocorasick_hex::add_keyword(uint8_t* data, size_t len)
{
    if (data == nullptr || len == 0)
    {
        return false;
    }

    // 构建tire树
    std::shared_ptr<ahocorasick_trie_node> it = _trie_root;

    for (size_t i = 0; i < len; i++)
    {
        auto k = data[i];
        if (!it->childs[k])
        {
            it->childs[k] = std::make_shared<ahocorasick_trie_node>();
        }
        it = it->childs[k];
    }
    it->exist_lens.push_back(len);
    return true;
}

bool ahocorasick_hex::add_keyword(const char* str, size_t len)
{
    return add_keyword((uint8_t*)str, len);
}

bool ahocorasick_hex::finalize()
{
    std::queue<std::shared_ptr<ahocorasick_trie_node>> bfs_queue;

    // 第一层fail指针 指向root
    for (size_t i = 0; i < 256; i++)
    {
        if (_trie_root->childs[i])
        {
            bfs_queue.push(_trie_root->childs[i]);
            _trie_root->childs[i]->fail = _trie_root;
        }
    }

    // 第二层开始
    while (!bfs_queue.empty())
    {
        //取出一个节点
        auto node_entry = bfs_queue.front(); bfs_queue.pop();
        for (size_t i = 0; i < 256; i++)
        {
            // 遍历子节点
            auto node_child = node_entry->childs[i];
            if (node_child)
            {
                // 父节点的fail指针
                auto parent_node_fail = node_entry->fail;
                while (parent_node_fail && !parent_node_fail->childs[i])
                {
                    parent_node_fail = parent_node_fail->fail;
                }

                if (!parent_node_fail)
                {
                    // 回溯到了根节点
                    node_child->fail = _trie_root;
                }
                else
                {
                    node_child->fail = parent_node_fail->childs[i];
                }

                if (!node_child->fail->exist_lens.empty())
                {
                    node_entry->exist_lens.insert(
                        node_entry->exist_lens.end(),
                        node_child->fail->exist_lens.begin(),
                        node_child->fail->exist_lens.end());
                }
                bfs_queue.push(node_child);
            }
        }
    }

    return true;
}


ahocorasick_match ahocorasick_hex::match_one(uint8_t* data, size_t len)
{
    ahocorasick_match match;

    do
    {
        if (data == nullptr || len == 0)
        {
            break;
        }
        auto scan_node = _trie_root;
        for (size_t i = 0; i < len; i++)
        {
            auto k = data[i];
            while (!scan_node->childs[k] && scan_node->fail)
            {
                scan_node = scan_node->fail;
            }
            if (scan_node->childs[k])
            {
                scan_node = scan_node->childs[k];
            }
            else
            {
                continue;
            }

            if (scan_node->exist_lens.size())
            {

                for (auto exist_len : scan_node->exist_lens)
                {
                    size_t pos = i - exist_len + 1;
                    match.offset = pos;
                    match.keyword.resize(exist_len);
                    memcpy(match.keyword.data(), &data[pos], exist_len);
                    return match;
                }

            }
        }

    } while (false);

    return match;
}

ahocorasick_match ahocorasick_hex::match_one(const char* str, size_t len)
{
    return match_one((uint8_t*)str, len);
}

std::vector<ahocorasick_match> ahocorasick_hex::match_all(uint8_t* data, size_t len)
{
    std::vector<ahocorasick_match> matchs;

    do
    {
        if (data == nullptr || len == 0)
        {
            break;
        }
        auto scan_node = _trie_root;
        for (size_t i = 0; i < len; i++)
        {
            auto k = data[i];
            while (!scan_node->childs[k] && scan_node->fail)
            {
                scan_node = scan_node->fail;
            }
            if (scan_node->childs[k])
            {
                scan_node = scan_node->childs[k];
            }
            else
            {
                continue;
            }

            if (scan_node->exist_lens.size())
            {
                for (auto exist_len : scan_node->exist_lens)
                {
                    ahocorasick_match match;
                    size_t pos = i - exist_len + 1;
                    match.offset = pos;
                    match.keyword.resize(exist_len);
                    memcpy(match.keyword.data(), &data[pos], exist_len);
                    matchs.push_back(std::move(match));
                }

            }
        }


    } while (false);

    return matchs;
}

std::vector<ahocorasick_match> ahocorasick_hex::match_all(const char* str, size_t len)
{
    return match_all((uint8_t*)str, len);
}

