<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright(c) 2026-Present Pen-NineCat(Pen-NineCat) -->
<!-- https://github.com/Pen-NineCat -->

# Introduction

r[introduction.overview]

## Overview

This book is the language specification and primary reference manual of the Pxc programming language, and it covers the following information:

- The object and ownership model, the concurrency and parallelism model, the memory and GC model, reflection, and the other runtime services — that is, the core models of Pxc.
- The list of the language's standard libraries, together with their interfaces and functionality.
- Some of the languages, programming principles and reference material that have deeply influenced this language.

Apart from that, this book does not include other content, for example:

- How the Pxc compiler `PLang` is used, and its implementation roadmap.
- How the Pxc package manager `PPak` is used, and its implementation roadmap.

> [!EVOLVING]
>
> This book is not yet finished, and the language is still evolving. For content that this book does not yet record, see the issues labelled `Pxc reference` in the project's [Issues](https://github.com/Pen-NineCat/Pxc/issues) list.

r[introduction.read-rule]

## How to read this book

This book is **not** an introductory tutorial to Pxc, and it is not necessarily suitable for beginners. If this is your first contact with Pxc and you need to learn the basics, see [the Pxc programming tutorial]().

> [!INCOMPLETE]
> The tutorial has not been started yet.

If you are a beginner and would rather read this book directly than the tutorial, we encourage you to read in chapter order: the chapter order already introduces the core content of the language in a progressive way.

If you are no longer a beginner, you can read each chapter independently. The following may be how you would like to read:

- If you need to look up something specific and you know its title or keywords, browse the table of contents and jump to the corresponding chapter and subsection; if you are not sure, click the magnifier button in the top-left corner of the site, or press `s` to open the search box, and enter keywords related to the content you need.

- If you are only here to learn more about this language, just browse the book's table of contents and pick out the content that is not clear to you or that interests you.

r[introduction.read-symbol]

## Conventions

Like other technical documents and websites, such as `cpp reference`, `rust reference` and `msdn`, this book has its own set of symbols and conventions:

### Admonitions

An Example admonition stands for

> [!EXAMPLE]
>
> ```
> Foo a := Foo(...);
> dyn b := a;
> dyn c := b;
> ```

A Note admonition stands for additional valid information about the current content, but that information is outside the scope of this book.

> [!NOTE]
> This is an example of valid information.

A Warning admonition stands for content that is easy to confuse; the developer needs to consider it carefully and use it with caution.

> [!WARNING]
> This is an `unsafe` operation; at runtime it cannot be analysed whether a managed object is still held from outside.

An Incomplete admonition stands for content that has been adopted into the standard, but whose write-up in this book is not finished; its main body may still be in the project's [Issues](https://github.com/Pen-NineCat/Pxc/issues) list.

> [!INCOMPLETE]
> The documentation about `Std.Collections.DynamicArray` is being written.

An Evolving admonition stands for content that is still evolving and whose current version is not a stable one; it usually appears in snapshot builds and is not recommended for real projects.

> [!EVOLVING]
>
> The interface design of `Std.Collections.Stream` is still evolving, and the current version does not represent stable content.

A Deprecated admonition stands for content that has been deprecated since some version and is kept only for compatibility.

> [!DEPRECATED-2026-rc4-442.5]
> The `consume` enumeration is deprecated; use the `relaxed` enumeration instead.

A Version admonition stands for content that behaves differently across major releases.

> [!VERSION-2026-rc3-422.1]
> Before Version:
> Before this version, a `dyn` object allocated an extra managed metadata block beyond the object metadata block.
> 
> After Version:
> From this version on, the compiler tries to maximise the storage efficiency of the object metadata block and the managed metadata block by mixing bitwise storage.
