<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright(c) 2026-Present Pen-NineCat(PenNineCat) -->
<!-- https://github.com/Pen-NineCat -->

# Attributes

> [!INCOMPLETE]
>
> Some of the examples in this section are incomplete: readers may not always find in them the information they expect, and this section has to wait for other chapters to be filled in.

> The content of this section lives in the module unit `Std.Meta.Attribute`.

r[attribute.define]

## Definition

An attribute is a metadata system shared across the whole program: that metadata is attached to its target according to the attribute's description, name and conventions, and it is available for reading.

Attributes are divided into:

- built-in attributes
- attribute classes declared through the attribute class description attribute

## Attribute declaration syntax

```
[[Attribute(...)]]
[[Attribute]] // the argument list is omitted
```

r[attribute.builtin]

## Built-in attributes

The following are some of the attributes built into the language; for a detailed description of any of them, go to its own page.

| Built-in attribute | Equivalent attribute class description attribute |
| --- | --- |
| `[[Flag]]` | `[[AttributeDescription(ReflectState.Static, AttributeTarget.Enum, false, false)]]` |

r[attribute.class]

## Attribute classes

An attribute class is a user-definable kind of attribute object: once a class carries the attribute class description attribute `[[AttributeDescription(...)]]`, it becomes an **attribute class**, and it can then be attached to other targets.

An attribute class must have at least one usable constructor, and that constructor must be `constexpr`, because an attribute class has to be constructed at **compile time** by non-designated initialization; the constructor may be a defaulted one.

The arguments in an attribute declaration are passed to the constructor and matched by overload resolution; if there is no constructor that matches, it is a compile error.

> [!EXAMPLE]
>
> The following declares an attribute class that annotates a listening port and accepts one `U16` argument, together with a function that uses this attribute.
>
> ```
> [[AttributeDescription(ReflectState.Dynamic, AttributeTarget.Function)]]
> class ListenPortAttr
> {
> 	public def constexpr ListenPortAttr(U16 port) noexcept : m_port(port)
> 	{
> 		
> 	}
> 	
> 	private U16 m_port;
> }
> 
> [[ListenPortAttr(500)]]
> def static MyPortListener(ref MessageBuffer msg) -> void
> {
> 	...
> }
> ```

## Types that an attribute class can store

An attribute class can only store objects that are storable in a `constexpr` context, including:

- the integer types `I8`, `I16`, `I32`, `I64`, `U8`, `U16`, `U32`, `U64`, `Isize`, `Usize`;
- the floating-point types `F32`, `F64`;
- the boolean type `bool`;
- the character types `Char`, `Char8`, `Char16`;
- all enumeration types;
- pointer types (which require an `unsafe` context);
- class types that are storable in a `constexpr` context;
- array types of the above.

## The attribute class description attribute

The attribute class description attribute lives in the module unit `Std.Meta.Attribute`; it is defined, equivalently, as follows:

```
[[AttributeDescription(ReflectState state, AttributeTarget target, bool allowMultiple = false, bool inherit = false)]]
```

It accepts the following parameters:

- `state`: the storage duration of the metadata carried by the attribute; accepts `ReflectState` enumeration values.
- `target`: the object declarations the attribute can act on; accepts `AttributeTarget` enumeration values.
- `allowMultiple`: whether the attribute may be used as a multiple attribute; defaults to `false`.
- `inherit`: whether this kind of attribute may be inherited by derived classes and implementing classes; defaults to `false`.

### The `ReflectState` enumeration

```
enum ReflectState : U8
{
	Static,
	Dynamic
}
```

| Value | Meaning |
| --- | --- |
| Static | The metadata carried by the attribute is available only at compile time |
| Dynamic | The metadata carried by the attribute can be read at runtime |

### The `AttributeTarget` enumeration

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

| Value | Meaning |
| --- | --- |
| `Class` | The attribute can be applied to a class |
| `Interface` | The attribute can be applied to an interface |
| `Enum` | The attribute can be applied to an enumeration type |
| `EnumMember` | The attribute can be applied to an enumeration member |
| `Field` | The attribute can be applied to a member of a class |
| `Function` | The attribute can be applied to a function, including member functions and free functions, but not constructors |
| `Constructor` | The attribute can be applied to a constructor |
| `Parameter` | The attribute can be applied to a method parameter |
| `ReturnValue` | The attribute can be applied to a return value |
| `Module` | The attribute can be applied to a module |

Because `AttributeTarget` conforms to the `[[Flag]]` attribute, several enumeration values can be combined bitwise to obtain the final list of targets.

Attaching an attribute to a target that `AttributeTarget` does not list is a compile error.

> [!EXAMPLE]
>
> The following is a combination example: it represents an attribute that can be applied to classes and interfaces, and that runtime reflection is allowed to obtain
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

## Multiple attributes

On one target, attributes with the same attribute type and the same arguments count as one and the same attribute, and the compiler keeps only one copy; otherwise they are **multiple attributes**, and at compile time independent metadata is generated for each of them.

With `allowMultiple = false` (the default), multiple attributes are not allowed on the same target.

> [!EXAMPLE]
>
> - An attribute without parameters: writing `[[Flag]]` twice on the same target gives the same single attribute both times.
> - An attribute with different arguments: `[[Api("a")]]` and `[[Api("b")]]` differ in their arguments, so they are not the same attribute.
> - An attribute with the same arguments: `[[Api("a")]]` and `[[Api("a")]]` have the same arguments, so they are the same attribute.

## Inherited attributes

With `inherit = false` (the default), an attribute acts only on its own target.

With `inherit = true`, the attribute is passed down the inheritance chain, all the way to the **most derived class**:

- an attribute on a base class is passed down to derived classes;
- an attribute on an interface is passed down to the classes implementing it;
- an attribute on a base class virtual method is passed down to the overriding method in derived classes;
- an attribute on an interface virtual method is passed down to the overriding method in derived classes.

Only attributes whose target is `Class`, `Interface` or `Function` can be inherited; setting this option on any other target has no effect.

> [!EXAMPLE]
>
> The following describes a set of inheritable attributes and a set of inheriting classes. In the end, `MyClassB` has the `MyClassAttribute` attribute, and its inherited method `Func()` also has the `MyMethodAttribute` attribute
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
