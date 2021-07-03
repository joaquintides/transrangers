## Transrangers in Rust: a C++/Rust comparison
* [Intro](#intro)
* [Rust iterators for C++ programmers](#rust-iterators-for-c-programmers)
  * [Iterator folding](#iterator-folding)
* [pushgen](#pushgen)
* [Performance](#performance)
* [Conclusions](#conclusions)
### Intro
[pushgen](https://github.com/AndWass/pushgen) from Andreas Wass is an implementation of [transrangers](README.md) in Rust following the philosophy and idiomatic conventions of this language. We analyze the design principles of pushgen and provide some performance comparisons between transrangers in C++ and Rust, [Range-v3](https://github.com/ericniebler/range-v3) and Rust [iterators](https://doc.rust-lang.org/std/iter/index.html). 
### Rust iterators for C++ programmers
A Rust iterator implements the following *trait* (generic interface):
```rust
// std::iter::Iterator
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
#### Iterator folding
The previous snippet can be rewritten as:
```rust
data.iter().filter(|x| *x % 2 == 0).map(|x| x * 3).for_each(process);
```
Where the regular `for` loop has been replaced by a call to the `std::iter::Iterator` *provided* method `for_each`. Provided methods have a default implementation that can be overridden by concrete iterator implementations. In the case of `for_each`, the default implementation relies on provided method `fold` (C++ `accumulate`), which in its turn relies on `next`.

So, `fold` default implementation is basically pull-based, but the standard library document encourages iterator implementors to override this with push-based code when more performance can be gained. `fold` and related method `try_fold` (a variation with early termination) are then used as customization points for performance improvement. C++ ranges/Range-v3 miss this important concept and cannot escape out of their pull-based interface all across range adaptor chains. 
### pushgen
\[For Andreas to write. I suggest you focus on how the original ideas in C++ translate to Rust and what the differences are (e.g. values are passed directly rather than via cursors, which makes `map` potentially more performant that in C++; how this affects the implementation of `dedup`; the usage of `ValueResult::MoreValues` and similar constants instead of booleans; etc.\]
### Performance
The same [tests](README.md#performance) originally written in C++ to compare transrangers and Range-v3 have been [ported to Rust](https://github.com/AndWass/pushgen/tree/main/benches) to do the analogous comparison between pushgen and Rust iterators:
* Test 1: `filter|transform` over 1M integers (Rust `filter.map`.)
* Test 2: `concat|take(1.5M)|filter|transform` over two vectors of 1M integers each (Rust `chain.take.filter.map`.)
* Test 3: `unique|filter` over 100k integers (Rust `dedup.filter`.)
* Test 4: `join|unique|filter|transform` over a collection of 10 vectors of 100k integers each (Rust `flatten.dedup.filter.map`.)
* Test 5: `transform(unique)|join|filter|transform` over a collection of 10 vectors of 100k integers each. (Rust `map(dedup).flatten.filter.map`.)
* Test 6: `zip(·,·|transform)|transform(sum)|filter` over two vectors of 1M integers each (Rust `zip(.map).map(sum).filter`.)

We measure three different mechanisms of range processing with Rust iterators:
* With a regular `for` loop (relying on `next`).
* With `for_each` (relying on `fold`).
* With `try_for_each` (relying on `try_fold`).

Tests for both C++ and Rust have been run on the same local Suse machine, so execution times can be compared directly. For C++, GCC 11.1.1 and Clang 12.0.0 have been used with `-O3 -DNDEBUG`. Rust tests have been benchmarked with [Criterion.rs](https://github.com/bheisler/criterion.rs). Execution times are in microseconds.
| Test | GCC 11.1<br/>transrangers | GCC 11.1<br/>Range-v3 | Clang 12.0<br/>transrangers | Clang 12.0<br/>Range-v3 | Rust<br/>pushgen | Rust<br/>iterators<br/>`for` loop | Rust<br/>iterators<br/>`for_each` | Rust<br/>iterators<br/>`try_for_each` |
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| Test 1    | 211              | 512           | 176                | 511             | 174     | 171               | 829                   | 422           |
| Test 2    | 1091             | 4075          | 1056               | 7231            | 923     | 2743              | 2834                  | 2911          |
| Test 3    | 24               | 73            | 28                 | 88              | 24      | 93                | 38                    | 65            |
| Test 4    | 752              | 603           | 249                | 896             | 304     | 465               | 1022                  | 1901          |
| Test 5    | 286              | 997           | 270                | 954             | 306     | 324               | 730                   | 675           |
| Test 6    | 931              | 1016          | 345                | 1199            | 356     | 353               | 1089                  | 751           |

The following matrix shows the *geometric average* relative execution time of scenarios in each row vs. scenarios in each column.
| | GCC 11.1<br/>transrangers | GCC 11.1<br/>Range-v3 | Clang 12.0<br/>transrangers | Clang 12.0<br/>Range-v3 | Rust<br/>pushgen | Rust<br/>iterators<br/>`for` loop | Rust<br/>iterators<br/>`for_each` | Rust<br/>iterators<br/>`try_for_each` |
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| **GCC 11.1<br/>transrangers** |**100%**|48%|145%|39%|143%|88%|50%|49%|
| **GCC 11.1<br/>Range-v3** |209%|**100%**|303%|81%|300%|185%|104%|103%|
| **Clang 12.0<br/>transrangers** |69%|33%|**100%**|27%|99%|61%|34%|34%|
| **Clang 12.0<br/>Range-v3** |259%|124%|375%|**100%**|371%|229%|129%|128%|
| **Rust<br/>pushgen** |70%|33%|101%|27%|**100%**|62%|35%|34%|
| **Rust<br/>iterators<br/>`for` loop** |113%|54%|164%|44%|162%|**100%**|56%|56%|
| **Rust<br/>iterators<br/>`for_each`** |201%|96%|290%|78%|288%|177%|**100%**|99%|
| **Rust<br/>iterators<br/>`try_for_each`** |203%|97%|293%|78%|291%|179%|101%|**100%**|

Some observations:
* To write
### Conclusions
To write
