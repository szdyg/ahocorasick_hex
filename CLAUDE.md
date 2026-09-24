# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

基于 AC 自动机（Aho-Corasick）的 **字节级** 多模式匹配库，用于特征码搜索。字母表是完整的 0–255 字节空间，而不是文本字符集——所以叫 "hex"。

全部源文件在 `c++/ahocorasick_hex/`：
- `ahocorasick_hex.h` / `.cpp` — 纯字面量 AC 引擎
- `ahocorasick_hex_fuzzy.h` / `.cpp` — 通配模式层（`"AECC3256????CEFF1256"`，`??` = 任意字节），构建在引擎之上
- `main.cpp` — 手写的冒烟测试（`main()`，打印匹配结果），仓库里没有单元测试框架

源码注释和提交信息用中文，文件为 UTF-8 编码（见提交 `028d387 utf8`）。新增代码请保持一致。

## 构建与运行

MSVC 项目，`v143` 工具集，配置为 `Application`（因为包含 `main.cpp`）。四种配置：`Debug|Release` × `Win32|x64`。

```bash
# 从 Git Bash 调用 MSBuild（路径按本机 VS 安装位置调整）
"/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" \
    "c++/ahocorasick_hex.sln" -p:Configuration=Release -p:Platform=x64

# 运行冒烟测试
"c++/x64/Release/ahocorasick_hex.exe"
```

没有 lint、格式化或测试脚本。验证改动的方式就是改 `main.cpp` 里的关键字/待匹配串，重新构建并肉眼比对输出。

**编码陷阱：** 源文件是**无 BOM** 的 UTF-8，注释含中文。在默认代码页非 UTF-8 的机器上（如本机 936/GBK），编译器按 GBK 解析，中文注释的末字节会吞掉行尾换行符，把下一行并进注释——报出 `C1075: “{” 未找到匹配令牌` 这类完全无关的错误。因此**每个配置都必须带 `/utf-8`**。目前 `Debug|x64`、`Release|x64` 有，两个 `Win32` 配置**仍缺失，构建会失败**。手工编译时也要加：

```bash
cl /nologo /EHsc /utf-8 /std:c++17 ahocorasick_hex.cpp main.cpp
```

## 架构要点

**使用契约（顺序敏感，头文件里有 XML 注释说明）：**
所有 `add_keyword()` → 恰好一次 `finalize()` → 任意多次 `match_*()`。违反顺序不会报错，只会静默给出错误结果：`finalize()` 之后添加的关键字没有 fail 链；重复 `finalize()` 会让 `exist_keywords` 重复累加，导致同一命中被报告多次。改动这三个阶段中任意一个时，都要检查是否破坏了这个契约。

**Trie 节点是定长 256 路数组**（`ahocorasick_trie_node::childs[256]`，`shared_ptr`）。换来 O(1) 的字节分派，代价是 `sizeof(ahocorasick_trie_node) == 4128` 字节（x64 实测；`shared_ptr` 16 字节 × 256）。因此 `finalize()` 里的 BFS 和 `match_*()` 都直接按字节下标索引，没有任何 map 查找，但节点数一多内存增长很快（2 万节点 ≈ 82MB）。

**关键字长度与编号而非关键字本身存在 trie 里**（`exist_keywords`，元素为 `{len, pattern_id}`）。命中时用 `pos = i - exist.len + 1` 从**输入缓冲区**回读原始字节重建关键字。含义：匹配结果依赖 `data` 在调用期间有效，并且节点不持有关键字副本。

**后缀输出在 finalize 期合并，不在匹配期遍历。** `finalize()` 建 fail 指针时，会把 `node->fail->exist_keywords` 直接追加进 `node->exist_keywords`（BFS 顺序保证 fail 节点已处理完，所以是传递闭包）。这就是为什么 `match_*()` 的热循环里不需要沿 fail 链走一遍收集输出——但也正是重复调用 `finalize()` 会造成结果重复的原因。

**根节点的 `fail` 是 `nullptr`，充当哨兵**。`finalize()` 和匹配循环中的 `while (... && scan_node->fail)` / `while (parent_node_fail && ...)` 都依赖这一点来终止。不要给根节点设 fail 自环。

**`match_one` 与 `match_all` 是两份并行实现**（`.cpp` 中结构几乎相同的两个循环）。修 bug 或改匹配语义时必须同步改两处。`match_one` 的内层 `for` 循环体无条件 `return`，实际只取 `exist_keywords` 的第一项。

**通配层对引擎零侵入。** `ahocorasick_hex_fuzzy` 不改 AC 引擎的任何一行，做法是「字面量锚点 + 掩码校验」：模式按 `??` 切成字面量片段，**最长**的一段作为锚点喂给内部的 `ahocorasick_hex`，锚点命中后反推模式起点 `start = hit.offset - anchor_offset`，再对整个模式做 value/mask 逐字节比对。通配符永不进入 trie——把 `??` 展开成 256 个分支会导致 `256^k` 路径爆炸，这是该设计存在的全部理由。

这一层与引擎的约定，改引擎时注意别破坏：
- 锚点喂给引擎时以**锚点编号**作为 `pattern_id`，命中后直接用 `hit.pattern_id` 下标访问 `_anchor_refs` 取出引用该锚点的模式列表，不再按命中字节查表。`_anchors`（字节串 → 锚点编号）只在 `add_pattern()` 时用于去重。因此引擎必须把 `pattern_id` 原样回填到 `ahocorasick_match`。
- 多个模式可共用同一锚点，所以 `_anchor_refs` 的元素是 `vector<anchor_ref>`；**同一锚点只会 `add_keyword()` 一次**，否则会对同一位置回报两次锚点命中、产生重复结果。
- 通配层自己的 `finalize()` 是幂等的（`_finalized` 标志），刻意规避引擎「重复 finalize 导致重复命中」的缺陷。引擎侧该缺陷仍未修。

**`pattern_id` 由调用者指定且不可重复**：引擎 `add_keyword()` 与通配层 `add_pattern()` 都要求传入编号，已占用的编号（`_ids`）会让添加返回 `false`。同一关键字字节串可以用不同编号添加多次，命中时按编号各报一次——这是有意为之，不是缺陷。

`match_one()` 无法提前收敛（锚点命中 ≠ 模式命中），内部就是完整 `match_all()` 再取首个，开销相同，不要误以为它更快。

**`~ahocorasick_trie_node()` 的迭代销毁是必需的，不要"简化"回空实现。** 默认的 `shared_ptr` 链式析构会按 trie 深度递归，深度超过约 2000–3000（单个关键字 ~2–3KB）时销毁阶段直接栈溢出崩溃。现在的实现把子节点先搬到显式工作栈再逐个释放，递归深度恒为 1，已验证深度 20 万正常。其中 `use_count() == 1` 的判断用于避免在节点被多处持有时破坏树结构。

**已知缺陷（均已实测复现，尚未修复）：**
- **重复 `finalize()` 会多报命中**：关键字 `{"ab","b"}` 匹配 `"xabx"`，调用两次 `finalize()` 得到 3 个命中而非 2 个。
- `uint8_t*` 重载不带 `const`。

核心匹配算法本身经 1.2 万组随机用例与暴力匹配对拍（含 0–255 全字节、重叠/嵌套/后缀关键字），结果完全一致——上面这些都是工程缺陷，不是算法错误。
