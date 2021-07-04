## Transrangers in Rust: a C++/Rust comparison
*[Joaquín M López Muñoz](https://github.com/joaquintides), [Andreas Wass](https://github.com/AndWass)*
* [Intro](#intro)
* [Rust iterators for C++ programmers](#rust-iterators-for-c-programmers)
  * [Iterator folding](#iterator-folding)
* [pushgen](#pushgen)
  * [Values instead of cursors](#values-instead-of-cursors)
* [Performance](#performance)
* [Conclusions](#conclusions)
### Intro
[pushgen](https://github.com/AndWass/pushgen) from Andreas Wass is an implementation of [transrangers](README.md) in Rust following the philosophy and idiomatic conventions of this language. We analyze the design principles of pushgen and provide some performance comparisons between transrangers in C++ and Rust, [Range-v3](https://github.com/ericniebler/range-v3) and Rust [iterators](https://doc.rust-lang.org/std/iter/index.html). 
### Rust iterators for C++ programmers
At its core, a Rust iterator implements the following *trait* (generic interface):
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
Where the regular `for` loop has been replaced by a call to the `std::iter::Iterator` *provided* method `for_each`. Provided methods have a default implementation that can be overridden by concrete iterator implementations. In the case of `for_each`, the default implementation relies on provided method `fold` (C++ `accumulate`), which in its turn, by default, relies on `next`.

So, `fold` default implementation is basically pull-based, but the standard library document encourages iterator implementors to override this with push-based code when more performance can be gained. `fold` and related method `try_fold` (a variation with early termination) are then used as customization points for performance improvement. C++ ranges/Range-v3 miss this important concept and cannot escape out of their pull-based interface all across range adaptor chains. 
### pushgen
A primary goal for pushgen is to allow iterator-styled code but making use
of the push-based approach presented by transrangers. This means that instead of writing
```cpp
transform(x3, filter(is_even, range));
```
one should be able to write something similar to
```rust
range.filter(is_even)
     .transform(x3);
```
It should also be extendable in the same way as Iterators, which causes pushgen, like Rust iterators,
to be built around a single trait:
```rust
pub trait Generator {
    type Output;
    fn run(&mut self, output: impl FnMut(Self::Output) -> crate::ValueResult) -> GeneratorResult;
}
```
The `run` method is analogous to the ranger call operator in transrangers, invoking a consumption function (`output`)
for each value in the range. A small difference is the return value of the ranger and consumption
function. In transrangers they both return a `bool`, but while writing pushgen the author felt it was easier
to prevent mistakes if these were two distinct types instead. This is merely a stylistic choice.

This trait is accompanied by a `GeneratorExt` trait which provides already-implemented
extension methods, like `map`, for all types that implements the `Generator` trait.
Pushgen also provides `SliceGenerator`, a generator implementation that can be used with any type that can be converted to
a slice (think `std::span` in C++).

Here is a small example of using `pushgen`:
```rust
use pushgen::{GeneratorExt, SliceGenerator};

fn main() {
    let data = [1, 2, 3, 4];
    SliceGenerator::new(&data)
        .map(|x| format!("Hello {}", x))
        .for_each(|x| println!("{}", x));
}
```

#### Values instead of cursors

A major difference between transrangers and pushgen is that pushgen follows the idimatic Rust approach
and passes values and references directly. Transrangers passes a lightweight copyable cursor to all
consumers instead.

One effect of not using cursors is that pushgen does not require address stability in
the way that transrangers does. Values can come from any potential data source.
Hence, the trait is named *Generator* instead of *Ranger* or similar.

Passing values directly enables `map` to
call its associated closure exactly once for each value. In transrangers the mapping function
is called each time the value is inspected, even for downstream adaptors.
```cpp
// transrangers: some_transformation is invoked both by unique and potentially
// by some_filter for every value in the range
filter(some_filter, unique(transform(some_transformation, rgr)));
```
```rust
// pushgen: some_transformation is invoked once for every value in the range
range.map(some_transformation).dedup().filter(some_filter);
```
If `some_transformation` is expensive the cursor approach can cause a significant performance hit.

Value passing also enables closures to output data based on internal state, and change this state, in a way that is
virtually impossible in transrangers.
`enumerate`, which simply pairs an index with a value can easily be implemented manually in pushgen:
```rust
let mut index = 0usize;
range.map(move |value| {
    let current = index;
    index += 1;
    (current, value) // Return a tuple of index and value
});
```
The same code in transrangers
```cpp
transform([index=std::size_t(0)](auto&& value) mutable {
    auto old = index;
    index += 1;
    return make_pair(old, move(value));
}, range);
```
would cause the index associated with each value to change
depending on which, if any, down-stream adaptors are used.

The cursor approach is more performant for expensive-to-move types though.

The value-based approach can also cause adaptors to behave slightly differently between Rust and C++.
`unique` in C++ will forward the first value and then ignore any subsequent duplicate values,
only copying and storing the cursor to the value that was output.
In Rust, `dedup` will ignore all duplicate values and forward a value only when a non-duplicate is seen.
Implementing the C++ approach in Rust would require cloneable values,
which is more likely to impact performance.

| Source range | Output from `unique` in  C++        | Output from `dedup` in pushgen         |
|--------------|-------------------------------------|----------------------------------------|
| 1            | 1                                   |                                        |
| 2            | 2                                   | 1                                      |
| 2            |                                     |                                        |
| 3            | 3                                   | 2                                      |
| *End*        | *End*                               | 3                                      |
| *End*        | *End*                               | *End*                                  |

### Performance
The same [tests](README.md#performance) originally written in C++ to compare transrangers and Range-v3 have been [ported to Rust](https://github.com/AndWass/pushgen/tree/main/benches) to do the analogous comparison between pushgen and Rust iterators:
* Test 1: `filter|transform` over 1M integers (Rust `filter.map`.)
* Test 2: `concat|take(1.5M)|filter|transform` over two vectors of 1M integers each (Rust `chain.take.filter.map`.)
* Test 3: `unique|filter` over 100k integers (Rust `dedup.filter`.)
* Test 4: `join|unique|filter|transform` over a collection of 10 vectors of 100k integers each (Rust `flatten.dedup.filter.map`.)
* Test 5: `transform(unique)|join|filter|transform` over a collection of 10 vectors of 100k integers each. (Rust `map(dedup).flatten.filter.map`.)
* Test 6: `zip(·,·|transform)|transform(sum)|filter` over two vectors of 1M integers each (Rust `zip(.map).map(sum).filter`.)

We measure three different mechanisms of range processing with Rust iterators:
* With `for_each` (relying on `fold`).
* With `try_for_each` (relying on `try_fold`).
* With a regular `for` loop (relying on `next`).

Tests for both C++ and Rust have been run on the same local Suse machine, so execution times can be compared directly. For C++, GCC 11.1.1 and Clang 12.0.0 have been used with `-O3 -DNDEBUG`. Rust tests have been benchmarked with [Criterion.rs](https://github.com/bheisler/criterion.rs). Execution times are in microseconds.
| Test | GCC 11.1<br/>transrangers | GCC 11.1<br/>Range-v3 | Clang 12.0<br/>transrangers | Clang 12.0<br/>Range-v3 | Rust<br/>pushgen | Rust<br/>iterators<br/>`for_each` | Rust<br/>iterators<br/>`try_for_each` | Rust<br/>iterators<br/>`for` loop |
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| Test 1    | 211              | 512           | 176                | 511             | 174     | 171               | 829                   | 422           |
| Test 2    | 1091             | 4075          | 1056               | 7231            | 923     | 2743              | 2834                  | 2911          |
| Test 3    | 24               | 73            | 28                 | 88              | 24      | 93                | 38                    | 65            |
| Test 4    | 752              | 603           | 249                | 896             | 304     | 465               | 1022                  | 1901          |
| Test 5    | 286              | 997           | 270                | 954             | 306     | 324               | 730                   | 675           |
| Test 6    | 931              | 1016          | 345                | 1199            | 356     | 353               | 1089                  | 751           |

The following matrix shows the *geometric average* relative execution time of scenarios in each row vs. scenarios in each column.
| | GCC 11.1<br/>transrangers | GCC 11.1<br/>Range-v3 | Clang 12.0<br/>transrangers | Clang 12.0<br/>Range-v3 | Rust<br/>pushgen | Rust<br/>iterators<br/>`for_each` | Rust<br/>iterators<br/>`try_for_each` | Rust<br/>iterators<br/>`for` loop |
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| **GCC 11.1<br/>transrangers** |**100%**|48%|145%|39%|143%|88%|50%|49%|
| **GCC 11.1<br/>Range-v3** |209%|**100%**|303%|81%|300%|185%|104%|103%|
| **Clang 12.0<br/>transrangers** |69%|33%|**100%**|27%|99%|61%|34%|34%|
| **Clang 12.0<br/>Range-v3** |259%|124%|375%|**100%**|371%|229%|129%|128%|
| **Rust<br/>pushgen** |70%|33%|101%|27%|**100%**|62%|35%|34%|
| **Rust<br/>iterators<br/>`for_each`** |113%|54%|164%|44%|162%|**100%**|56%|56%|
| **Rust<br/>iterators<br/>`try_for_each`** |201%|96%|290%|78%|288%|177%|**100%**|99%|
| **Rust<br/>iterators<br/>`for` loop** |203%|97%|293%|78%|291%|179%|101%|**100%**|

Some observations:
* Transrangers in Clang and pushgen are the fastest scenarios and behave almost exactly on each test, which strongly suggests that the assembly code generated is basically the same (this has not been verified by visual inspection, though).
* Transrangers in GCC are still faster than their Range-v3 counterpart, but they lose some performace with respect to Clang, particularly in tests 4 and 6.
* Range-v3 performance is notably worse than that of Rust iterators (in any of its looping variants). Some Range-v3 tests run as much as three times slower than Rust iterators with `for_each`.
* As expected, Rust iterators deliver more performance when push-based-friendly `for_each` is used rather than `for` loops with pull-based `next`. `try_for_each`, on the other hand, does not fare better, which seems odd given that the same optimization possibilities exist as with `for_each` —incidentally, `try_for_each` is structurally equivalent to transrangers/pushgen. Source code inspection reveals that Rust iterators (both from the standard library and `dedup` from crate [`itertools`](https://docs.rs/itertools/)) are missing some optimization opportunities: for instance, `#[inline]` directives are not used everywhere, and `dedup` does not provide a bespoke `try_fold` implementation.
### Conclusions
Crate [pushgen](https://github.com/AndWass/pushgen) ports the transrangers design pattern to Rust and delivers there the same performance as in C++. Rust iterators provide custom optimization points `fold`/`try_fold` that can potentially achieve the same push-based performance, though as of this writing they are lagging behind pushgen. C++ ranges/Range-v3, which rely exclusively on a pull-based architecture, could benefit from adopting transrangers or a similar approach for the optimization of range processing operations: Rust iterators `fold`/`try_fold` show how this can be done in a non-intrusive, backwards-compatible manner.
