<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright(c) 2026-Present Pen-NineCat(PenNineCat) -->
<!-- https://github.com/Pen-NineCat -->

# Pxc 规范站点

*[English](README.en.md)*

本目录是 `Docs/book`，用 [mdBook](https://rust-lang.github.io/mdBook/) 渲染，站点风格取自
[Rust Reference](https://doc.rust-lang.org/reference/)：左侧栏可折叠（本仓库关掉了编号）、
正文左边距放规则 ID、提示块画成按类型着色的圆角包围框、表格画成三线表。

## 构建

需要 **mdBook 0.5 或更新**（0.5 改了预处理器协议）与 Python 3：

```shell
# 本地预览，改文件自动重建：http://localhost:3000
mdbook serve

# 只构建到 book/（该目录不提交）
mdbook build
```

首次使用需安装 mdBook：

```shell
cargo install mdbook --locked
```

## 目录

```
PxcReference
├─ book.toml          # mdBook 配置：挂 CSS、挂预处理器、折叠层级
├─ src/
│  ├─ SUMMARY.md      # 目录：顶层是「中文 / English」两棵子树
│  ├─ zh/             # 中文（母本）
│  └─ en/             # 英文（译文）
├─ theme/
│  ├─ pxc.css         # 全站唯一一份样式
│  ├─ rule-fit.js     # 规则标签放不下时换成 [*]
│  └─ cjk-search.js   # 接管搜索索引，让搜索认得中文（见「搜索」）
└─ tools/pxc_spec.py  # 预处理器：规则 ID / 提示块 / 规则引用 / 语言切换
```

`book/` 是构建输出，已在 `.gitignore` 里排除。

## 页面样式规则

**字体**：正文用系统 UI 字体栈（Windows 上是 Segoe UI，macOS 上是 SF），代码段用等宽栈
（`SF Mono` → `JetBrains Mono` → `Fira Code` → `Consolas` → `Liberation Mono` …），
不再沿用 mdBook 自带的 Open Sans 与 Source Code Pro；两套栈都写在 `theme/pxc.css` 的 §0。
分开选的理由很具体：比例字体里 `>` 对齐的是大写字母中线，比 `-` 高 0.094em（16px 下 1.5px），
`->` 会像浮在半空；等宽字体里两个字形同轴、字宽固定，`->`、`::` 与用空格对齐的注释列都读得对。

**代码**——行内代码与围栏代码块——用等宽字体、**正文字号**（不缩字号）：代码块靠浅灰底色与
正文分开，提示块与表格里的代码块和行内代码改成描边而不铺底（半透明底色叠在彩色框上会显脏）。

## 搜索

mdBook 的搜索索引在**构建时**就建好了，分词只认空白与标点：一整段中文会被当成一个词，
最后被 trimmer 丢掉。索引里含中文的 token 屈指可数（写这段时全书 30 节、695 个 token，
只有 2 个含中文，还是同一处夹在英文里的残留），所以「存储期」搜不到「元数据存储期」。

`theme/cjk-search.js` 把这件事补在浏览器里。mdBook 的搜索器对索引只有一句
`elasticlunr.Index.load(index).search(词, 选项)`，脚本把 `Index.load` 换成自己的实现，
用索引里保留的原文（`documentStore.docs` 的 title / body / breadcrumbs）现算结果——
**不改 mdBook、不改构建产物、不加构建期依赖**。

- 中文按**相邻两字**切（单字查询就是那个字），英文按整词切；查询里每个词都要在同一节里
  出现才成结果（与 `[output.html.search] use-boolean-and = true` 一致）。
- 分数按字段加权：标题 4，面包屑与正文 1；与当前页面同语言的结果再乘 2——中英两版内容成对，
  不加这一下，中文页搜 `ReflectState` 会先看到英文那份。
- 摘要取命中处前后 24 / 80 字：搜索器的摘要按**空格**取词，中文整节只有一个「词」，
  不裁会把整节正文印进结果行。

已知边界：

- 摘要里**没有**命中高亮（正文在渲染前已被搜索器转义，`<em>` 注入不进去）；点进页面后的
  `?highlight=` 整词标亮不受影响。
- 没有错字容错：按字面匹配，不像 fzf 那样允许字序跳跃。
- 用 `?search=` 深链首次打开时有毫秒级竞态：那一次可能仍用 mdBook 自带的索引，刷新即好。
- 不想要这套行为时，删掉 `additional-js` 里的 `cjk-search.js` 一项即可。

## 暂不实现：文法块与铁路图

Rust Reference 的 `grammar` 围栏会渲染成两层：字面量带圆角块、非终结符带颜色的**文法块**，
以及从同一份 BNF 自动生成的**铁路图**（页面上 "Show syntax diagrams" 那个开关）。
本仓库两样都没做——Pxc 目前还没有正式的文法产物，先写普通代码围栏即可：

````markdown
```ebnf
Expression -> ExpressionWithoutBlock | ExpressionWithBlock
```
````

以后要那层外观，只需在 `theme/pxc.css` 补样式并在 `tools/pxc_spec.py` 加一个围栏分支，
正文的写法不用改。

## 双语约定

- 中文是母本，英文是译文；两版的章节顺序、规则 ID、提示块类型一一对应。
- 页面按「去掉语言目录后的相对路径」配对（`zh/storage.md` ↔ `en/storage.md`）；配对成功时
  页面右上角出现「中文 / English」切换，所以新增一章要两侧同时建同名文件。
- 构建时若某种语言缺少另一种语言已有的规则 ID，预处理器会在 stderr 报一行汇总。

## 许可证

- 站点代码与配置（`book.toml`、`theme/`、`tools/`、本文件）：**Apache-2.0**，见仓库根 `LICENSE.txt`
- 规范正文（`src/` 下的 Markdown）：**[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)**
- 第三方作品的署名与许可：本目录的 `THIRD-PARTY-NOTICES.md`（中文）与
  [THIRD-PARTY-NOTICES.en.md](THIRD-PARTY-NOTICES.en.md)（英文），仓库根另有一份英文副本
  （[THIRD-PARTY-NOTICES.md](../../THIRD-PARTY-NOTICES.md)）；三份内容需一致
