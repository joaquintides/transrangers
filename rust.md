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
The same [tests](README.md#performance) originally written in C++ to compare transrangers and Range-v3 have been ported to Rust to do the analogous comparison between pushgen and Rust iterators:
* Test 1: `filter|transform` over 1M integers (Rust [`filter.map`](https://github.com/AndWass/pushgen/blob/main/benches/filter_map.rs).)
* Test 2: `concat|take(1.5M)|filter|transform` over two vectors of 1M integers each (Rust [`chain.take.filter.map`](https://github.com/AndWass/pushgen/blob/main/benches/chain_take_filter_map.rs).)
* Test 3: `unique|filter` over 100k integers (Rust [`dedup.filter`](https://github.com/AndWass/pushgen/blob/main/benches/dedup_filter.rs).)
* Test 4: `join|unique|filter|transform` over a collection of 10 vectors of 100k integers each (Rust [`flatten.dedup.filter.map`](https://github.com/AndWass/pushgen/blob/main/benches/flatten_dedup_filter_map.rs).)
* Test 5: `transform(unique)|join|filter|transform` over a collection of 10 vectors of 100k integers each. (Rust `map(dedup).flatten.filter.map`.)
* Test 6: `zip(·,·|transform)|transform(sum)|filter` over two vectors of 1M integers each (Rust [`zip(.map).map(sum).filter`](https://github.com/AndWass/pushgen/blob/main/benches/transrangers_test6.rs).)

Tests for both C++ and Rust have been run on the same local Suse machine, so execution times can be compared directly. For C++, GCC 11.1.1 and Clang 12.0.0 have been used with `-O3 -DNDEBUG`. Rust tests have been benchmarked with [Criterion.rs](https://github.com/bheisler/criterion.rs). Execution times are in microseconds.
|Test    | GCC 11.1.1<br/>transrangers | GCC 11.1.1<br/>Range-v3 | Clang 12.0.0<br/>transrangers | Clange 12.0.0<br/>Range-v3 | pushgen | Rust iterators |
|:------:|:----:|:----:|:----:|:----:|:----:|:----:|
|Test 1  | 203  | 485  | 174  | 524  | 174  | 174  |
|Test 2  | 1080 | 4011 | 1039 | 7075 | 813  | 1267 |
|Test 3  | 24   | 71   | 22   | 77   | 24   |  94  |
|Test 4  | 700  | 602  | 253  | 887  | 271  | 520  |
|Test 5  | 283  | 1011 | 266  | 938  | 174  | 237  |
|Test 6  | 899  | 1002 | 347  | 1172 | 348  | 348  |
### Conclusions
To write
