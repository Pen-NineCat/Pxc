<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright(c) 2026-Present Pen-NineCat(PenNineCat) -->
<!-- https://github.com/Pen-NineCat -->

# Pxc 规范站点

本目录是 `Docs/book`，用 [mdBook](https://rust-lang.github.io/mdBook/) 渲染，站点风格取自
[Rust Reference](https://doc.rust-lang.org/reference/)：左侧栏可折叠（本仓库关掉了编号）、
正文左边距放规则 ID、提示块无背景只有左侧色条。

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
Docs/book/
├─ book.toml          # mdBook 配置：挂 CSS、挂预处理器、折叠层级
├─ src/
│  ├─ SUMMARY.md      # 目录：顶层是「中文 / English」两棵子树
│  ├─ zh/             # 中文（母本）
│  └─ en/             # 英文（译文）
├─ theme/pxc.css      # 站点主题，全站唯一一份样式
└─ tools/pxc_spec.py  # 预处理器：规则 ID / 提示块 / 规则引用 / 语言切换
```

`book/` 是构建输出，已在 `.gitignore` 里排除。

## 三种写法

### 1. 规则 ID

单独一行写给**紧跟着的那段正文**（标题或段落）打上可引用的锚点：

```markdown
r[storage.kinds]
## 三种存储期
```

渲染成左边距里的灰色标签 `[storage.kinds]`，锚点 `#r-storage.kinds`。ID 用点分段、从小写英文写起，
**一旦被别处引用就不再改**。中英两版同一条规则必须用同一个 ID。

围栏里的 `r[xxx]` 不会被当成规则，所以演示写法时照常放进代码围栏即可。

### 2. 提示块

```markdown
> [!WARNING]
> 跨过去只有两条路……
```

支持五种：`NOTE`、`WARNING`、`EXAMPLE`、`INCOMPLETE`（该条目不完整）、`STALE`（该条目可能过期）。
标题按所在语言自动切换（中文页显示「警告」，英文页显示「Warning」）。
代码围栏里的 `> [!NOTE]` 不会被转换，可以用它演示语法。

### 3. 引用别处的规则

正文只写短语，链接定义的目标写规则 ID：

```markdown
见 [三种存储期]。

[三种存储期]: storage.kinds
```

预处理器补全成 `storage.html#r-storage.kinds`；同一 ID 在中英两版都有时，指向当前语言那一份。
目标写成规则 ID 的样子却查不到时（多半是笔误），构建会报一行汇总并按原样保留，免得静默产出一个
点开即 404 的坏链。

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
- 第三方作品的署名与许可：见 `THIRD-PARTY-NOTICES.md`
