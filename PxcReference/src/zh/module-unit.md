<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright(c) 2026-Present Pen-NineCat(PenNineCat) -->
<!-- https://github.com/Pen-NineCat -->

# 模块单元

r[module.define]

## 定义

模块单元是程序的组织单元，也是编译与链接的对象，同时也是版本控制所管理的单元。



## 模块路径

模块路径用于控制单元之间，哪些对象是所有单元都可见的，而哪些对象是



r[module.include]

## 单元项

单元项是指每个单元可以添加哪些内容

其包括

- 所属模块

- 单元导入
- 静态函数
- 静态变量
- 静态常量
- 接口
- 类
- 联合体
- 枚举
- 约束
- 类型别名
- 外部函数定义

## `import`导入

`import`用于将目标单元中所有**可见**的声明引入当前单元，被引入的名字仍然带着它原本的模块前缀。

> [!EXAMPLE]
>
> 以下代码演示了如何导入一个单元并且使用
>
> ```pxc
> module MyModule;
> import OtherModule.Container;
> 
> def MyFunc() -> void
> {
> 	auto con := OtherModule.Container();
> 	
> 	...
> }

## `using import`导入



