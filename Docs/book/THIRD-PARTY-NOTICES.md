<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright(c) 2026-Present Pen-NineCat(PenNineCat) -->
<!-- https://github.com/Pen-NineCat -->

# 第三方声明

*[English version](THIRD-PARTY-NOTICES.en.md)* —— 英文版与本文件放在一起，仓库根另有一份同样的副本
（[THIRD-PARTY-NOTICES.md](../../THIRD-PARTY-NOTICES.md)）。所有副本内容需一致：改一份就要改其余几份。

本目录的站点在构建与渲染时用到下列第三方作品。渲染产物 `book/` 不入库，但**分发渲染结果时
应连同本文件一并提供**。

## rust-lang/reference

- 项目：<https://github.com/rust-lang/reference>
- 许可：MIT OR Apache-2.0（本仓库按 **MIT** 使用）
- 用到的部分：`theme/pxc.css` 中三栏栅格与「左边距规则标签」的做法（`grid-template-columns`、
  `max-width`、`container-type` 等少量功能声明），以及提示块的设计取向。提示块的具体样式由
  本仓库另写，不含该项目的代码。
- 该项目的版权与许可声明，原样保留如下：

```
Copyright (c) 2010 The Rust Project Developers

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

## mdBook

- 项目：<https://github.com/rust-lang/mdBook>
- 许可：**MPL-2.0**，全文见 <https://www.mozilla.org/MPL/2.0/>
- 用到的部分：`book/` 下的 CSS、JavaScript、页面模板、字体与界面图标，均由 mdBook 生成或原样
  复制；本仓库未修改这些文件。若需要这些文件的源码，见上游仓库对应版本（v0.5.4）。
- 随产物输出的字体许可：`book/fonts/OPEN-SANS-LICENSE.txt`、`book/fonts/SOURCE-CODE-PRO-LICENSE.txt`

## 字体与图标

- **Open Sans**、**Source Code Pro**：SIL Open Font License 1.1，由 mdBook 分发，许可全文随
  渲染产物输出到 `book/fonts/`。
- **mdBook 界面图标**：Font Awesome Free 6.x（图标部分 CC BY 4.0）；复制按钮的图标取自
  GitHub octicons（MIT）。版权声明以注释形式保留在 mdBook 生成的内联 SVG 中。
- **提示块的五个图标**（注 / 警告 / 示例 / 该条目不完整 / 该条目可能过期）：本仓库自绘的几何
  图形，见 `tools/pxc_spec.py`，不涉及第三方素材。
