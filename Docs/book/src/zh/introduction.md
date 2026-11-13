<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright(c) 2026-Present Pen-NineCat(Pen-NineCat) -->
<!-- https://github.com/Pen-NineCat -->

# 介绍

r[introduction.overview]

## 总述

本书是Pxc编程语言的语言规范与主要参考手册，本书包括以下信息：

- 本语言的对象与所有权模型、并发与并行模型、内存与GC模型、反射以及其他的运行时服务等Pxc核心模型。
- 本语言的标准库列表及其接口与功能。
- 一些对本语言有深远影响的语言，编程原理与参考资料。

除此之外，本书并不包括其他内容，例如：

- Pxc语言编译器`PLang`的使用方式与实现路线。
- Pxc包管理器`PPak`的使用方式与实现路线。

> [!EVOLVING]
>
>此书尚未完成，并语言仍然在持续演进中。有关本书未记录的内容，请参阅项目的[Issues](https://github.com/Pen-NineCat/Pxc/issues)列表中，带有`Pxc reference`标签的issue。

r[introduction.read-rule]

## 如何阅读此书

本书**不是**对Pxc的入门教学，并不一定适合初学者，如果您是第一次接触到Pxc，并且需要学习基础知识，请参阅[Pxc编程教程]()。

> [!INCOMPLETE]
> 教程文档还未开始编写。

如果您是初学者，并且想直接阅读该书而不是教程，我们鼓励您沿着章节顺序阅读，章节顺序已经以递进的方式介绍了本语言的核心内容。

如果您已经不是初学者，可以自行独立阅读每一章，以下可能是您期望的阅读方式：

- 如果您需要搜索特定的内容，并且您知道内容的标题或者关键词，可以浏览目录并跳转到相应的章节与子小节；如果您不太清楚，可以点击网站左上角的放大镜按钮，或者按下`s`以打开搜索框，并输入与您需要的内容相关的关键字。

- 如果您只是为了了解本语言的更多内容，只需浏览本书的目录，挑选其中您不是很清楚或者感兴趣的内容。

r[introduction.read-symbol]

## 约定

与其他技术性文档以及网站，例如`cpp reference`，`rust reference`，`msdn`一样，本书也有一定的符号与约定：

### 信息块

Example 信息块代表

> [!EXAMPLE]
>
> ```
> Foo a := Foo(...);
> dyn b := a;
> dyn c := b;
> ```

Note 信息块代表当前内容的额外有效信息，但是不属于本书的范围。

> [!NOTE]
> 这是一个有效信息示例。

Warning 信息块代表当前内容易于混淆，需要开发者仔细考虑，谨慎使用。

> [!WARNING]
> 这是一个`unsafe`操作，运行时无法分析出托管对象是否仍然被外部持有。

Incomplete 信息块代表当前内容已经被纳入标准，但是本书并未编写完成其内容，其主要内容可能仍然位于项目的[Issues](https://github.com/Pen-NineCat/Pxc/issues)列表中。

> [!INCOMPLETE]
> 关于`Std.Collections.DynamicArray`的文档正在编写中。

Evolving 信息块代表当前内容正在演进中，当前版本并不是稳定版本，其常见于快照版本，不推荐用于实际项目。

> [!EVOLVING]
>
> 关于`Std.Collections.Stream`的接口设计仍在演进，当前版本并不代表稳定内容。

Deprecated 信息块代表当前内容在某个版本后已经被弃用，只用于兼容性保留。

> [!DEPRECATED-2026-rc4-442.5]
> `consume`枚举已经被弃用，请采用`relaxed`枚举。

Version 信息块代表当前内容在不同的大型发布版本中表现不同。

> [!VERSION-2026-rc3-422.1]
> Before Version:
> 在该版本之前，`dyn`对象将在对象元数据段之外额外分配托管元数据段。
> 
> After Version:
> 在该版本后，编译器将尝试通过混合按位存储以最大化对象元数据段与托管元数据段存储效率。

