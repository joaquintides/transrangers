## Transrangers in Rust: a C++/Rust comparison
* [Intro](#intro)
* [Rust iterators for C++ programmers](#rust-iterators-for-c-programmers)
* [pushgen](#pushgen)
* [Performance](#performance)
* [Conclusions](#conclusions)
### Intro
[pushgen](https://github.com/AndWass/pushgen) from Andreas Wass is an implementation of [transrangers](README.md) in Rust following the philosophy and idiomatic conventions of this language. We analyze the design principles of pushgen and provide some performance comparisons between transrangers in C++ and Rust, [Range-v3](https://github.com/ericniebler/range-v3) and Rust [iterators](https://doc.rust-lang.org/std/iter/index.html). 
### Rust iterators for C++ programmers
A Rust iterator implements the following *trait* (generic interface):
```rust
trait Iterator {
    type Item;
    fn next(&mut self) -> Option<Self::Item>;
}
```
Whereas in C++ (input) iterators have a triad of operations for dereferencing, increment and comparison, Rust coalesces the entire functionality into a single `next` method that returns the "pointed-to" value (or nothing if at the end of the range) and moves to the following position.

In a sense, Rust iterators are more similar to C++ ranges than actual C++ iterators, since range termination is checked internally without the need for user code to keep an end iterator/sentinel for comparison. This allows for easy composition via *iterator adapters*:
```rust
for item in data.iter().filter(|x| *x % 2 == 0).map(|x| x * 3) {
    process(item);
}
```
Iterator adapters are then functionally equivalent to C++ ranges/Range-v3 range adaptors:
|Rust iterator adapter|C++ ranges/Range-v3|
|:-------------------:|:-----------------:|
|`chain`              |`concat`           |
|`dedup`              |`unique`           |
|`filter`             |`filter`           |
|`flatten`            |`join`             |
|`map`                |`transform`        |
|`take`               |`take`             |
|`zip`                |`zip`              |
### pushgen
\[For Andreas to write. I suggest you focus on how the original ideas in C++ translate to Rust and what the differences are (e.g. values are passed directly rather than via cursors, which makes `map` potentially more performant that in C++; how this affects the implementation of `dedup`; the usage of `ValueResult::MoreValues` and similar constants instead of booleans; etc.\]
### Performance
To write
### Conclusions
To write
