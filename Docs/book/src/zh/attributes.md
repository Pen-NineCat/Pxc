<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright(c) 2026-Present Pen-NineCat(PenNineCat) -->
<!-- https://github.com/Pen-NineCat -->

# 属性

> [!INCOMPLETE]
>
> 本节的一些示例并不完整，读者可能不一定能够从中了解到期望的信息，需要等待其他章节的内容补充。

> 本节的内容位于模块单元`Std.Meta.Attribute`内。

r[attribute.define]

## 定义

属性是一种全程序通用的元数据系统，这些元数据会依据其属性描述、名称、约定附加给其目标，并供读取。本语言的属性结合了 C++ 的静态注解，以及 C# / Java 等语言的动态特性/注解，其语法来自于 C++。

属性包括：

- 内建属性
- 通过属性类描述属性声明的属性类

r[attribute.builtin]

## 内建属性

以下是一些语言内建的属性，其详细描述请跳转到各自的详细页面。

| 内建属性   | 等价的属性类描述属性                                         |
| ---------- | ------------------------------------------------------------ |
| `[[Flag]]` | `[[AttributeDescription(ReflectState.Static, AttributeTarget.Enum, false, false)]]` |

r[attribute.class]

## 属性类与属性类描述属性

为类添加属性`[[AttributeDescription(...)]]`标注，其就成为一个**属性类**，可以附加给其他目标。

属性类描述属性的等价定义如下：

```
[[AttributeDescription(ReflectState state, AttributeTarget target, bool allowMultiple = false, bool inherit = false)]]
```

其接受参数：

- `state`：该属性所携带的元数据存储期，接受`ReflectState`枚举值。
- `target`：该属性可以作用的对象声明，接受`AttributeTarget`枚举值。
- `allowMultiple`：是否允许将该属性作为多重属性使用，默认为`false`。
- `inherit`：是否允许该类属性被派生类与实现类继承，默认为`false`。

### `ReflectState`枚举

```
enum ReflectState : U8
{
	Static,
	Dynamic
}
```

| 值      | 语义                                 |
| ------- | ------------------------------------ |
| Static  | 该属性所携带的元数据只在编译期可用   |
| Dynamic | 该属性所携带的元数据可以在运行时访问 |

### `AttributeTarget`枚举

```
[[Flag]]
enum class AttributeTarget
{
	Class = 1 << 0,
	Interface = 1 << 1,
	Enum = 1 << 2,
	EnumMember = 1 << 3,
	Field = 1 << 4,
	Function = 1 << 5,
	Constructor = 1 << 6,
	Parameter = 1 << 7,
	ReturnValue = 1 << 8,
	Module = 1 << 9
}
```

| 值            | 语义                                                         |
| ------------- | ------------------------------------------------------------ |
| `Class`       | 该属性可以应用在类上                                         |
| `Interface`   | 该属性可以应用在接口上                                       |
| `Enum`        | 该属性可以应用在枚举类型上                                   |
| `EnumMember`  | 该属性可以应用在枚举成员上                                   |
| `Field`       | 该属性可以应用在类的成员上                                   |
| `Function`    | 该属性可以应用在函数上，包括成员函数与自由函数，但不包括构造函数 |
| `Constructor` | 该属性可以应用在构造函数上                                   |
| `Parameter`   | 该属性可以应用在方法参数上                                   |
| `ReturnValue` | 该属性可以应用在返回值上                                     |
| `Module`      | 该属性可以应用在模块上                                       |

由于`AttributeTarget`符合`[[Flag]]`属性，所以可以将多个枚举值按位组合，得到最终的目标列表。

将属性附加给`AttributeTarget`没有列出的目标是编译错误。

> [!EXAMPLE]
>
> 以下是一个组合样例，其代表一个可以应用于类与接口上，并且允许运行期反射得到的属性。
>
> ```
> [[AttributeDescription(ReflectState.Dynamic, AttributeTarget.Class | AttributeTarget.Interface)]]
> public class MyAttribute
> {
> 	...
> }
> 
> [[MyAttribute]]
> class MyClass
> {
> 
> }
> ```

## 多重属性

同一目标上，若多个属性的类型一致、参数一致，则视为同一个属性，编译器只会保留一份；否则为**多重属性**，编译期会为每一个属性都生成独立的元数据。

`allowMultiple = false`（默认）时，同一目标不允许多重属性。

> [!EXAMPLE]
>
> - 无参数的属性：同一目标上写两次`[[Flag]]`，两次就是同一份。
> - 带不同参数的属性：`[[Api("a")]]`与`[[Api("b")]]`实参不同，不是同一份。
> - 带相同参数的属性：`[[Api("a")]]`与`[[Api("a")]]`实参相同，是同一份。

## 继承属性

`inherit = false`（默认）时，属性只作用于它的目标。

`inherit = true`时，这份属性沿继承关系向下传递，直到**最派生类**：

- 基类上的属性传给派生类。
- 接口上的属性传给实现它的类。
- 基类虚方法上的属性传给重写的派生类方法。
- 接口虚方法上的属性传给重写的派生类方法。

只有目标为`Class`、`Interface`、`Function`的属性可以被继承，其他目标设置该选项无效。

> [!EXAMPLE]
>
> 以下描述了一组允许继承的属性，以及一组继承类。最终，可以发现`MyClassB`拥有`MyClassAttribute`属性，其继承方法`Func()`也有`MyMethodAttribute`属性。
>
> ```
> [[AttributeDescription(ReflectState.Dynamic, AttributeTarget.Class, false, true)]]
> class MyClassAttribute
> {}
> 
> [[AttributeDescription(ReflectState.Dynamic, AttributeTarget.Function, false, true)]]
> class MyMethodAttribute
> {}
> 
> [[MyClassAttribute]]
> class MyClassA
> {
> 	[[MyMethodAttribute]]
> 	virtual def Func() -> void {}
> }
> 
> class MyClassB : MyClassA
> {
> 	virtual def Func() override -> void {}
> }
> ```
