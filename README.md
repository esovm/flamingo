# 🦩 Welcome to Flamingo

Flamingo is a lisp-like programming language designed to be simple, embeddable and self-extending.

```lisp
(puts 'Hello, World!')
```

Or something a little more advanced, let's define a `map` function:
```lisp
# apply function `func` on each element of list `l`.
(fn [map func l] [
  if (== l null)
    [null]
    [attach (list (func (st l))) (map func (rest l))]
])
```
Simple, isn't it? Now let's use it with a `cube` function (third power):
```c
=> fn [cube x] [pow x 3]
()
=> map cube [4 5 6]
[64 125 216]
```

## Building on Unix-like systems
#### Requirements

* Make
* A modern C compiler
* editline (on Debian-based systems: `sudo apt install libedit-dev`.
    on macOS with Homebrew installed: `brew install libedit`)

now simply run `make` in the root directory of the project.

---

## Hello Flamingo

Like every language (we hope), Flamingo is capable of printing stuff to the screen:

```lisp
(puts 'I love pizza 🍕!')
```
From this simple example, you can already see 3 things about Flamingo:
* Every expression in Flamingo is parenthesized (except for in REPL, where they are optional).
* `puts` is short for "put string" and can output any expression to standard output.
* Strings are sequences of characters enclosed in single quotes.

### Main concepts

**S-Expression**

`(function param1 param2 ...)`

An s-expression (for "symbolic expression") is a way of writing lists, and using them for code ***and*** data. In the usual parenthesized syntax, an s-expression is classically defined as

1. an atom (e.g. a symbol, number, or a string), or
2. an expression of the form `(a b)` where `a` and `b` are s-expressions.


**B-Expression (for "Bracket expression")**

`[a b c ...]`

Like s-expressions, b-expression are lists. Unlike s-expressions, they don't get evaluated by the default `eval` function. This is useful when you need to decide if the should ever be executed, or should be executed several times, or just kept as it is.
Good example would structure "if", in this case "No" never gets printed and the expression `[puts 'No']`is never evaluated.

```lisp
(if [== 1 1] [puts 'Yes'] [puts 'No'])
```

With the nature of a B-expression, you can make lots of interesting things, from arrays to dictionary-like structures:

```lisp
(first [11 22 33 44 55 66 77]) # 11
```

Mix in some types, why not?

```lisp
['Hello world' 1 2 3 [1 'Nested'] some-identifier 'The next is an expression, that can be evalueted' [* 2 PI]]
```

To evaluate a B-expression, simply call the built-in `eval` function:

```lisp
(eval [* 2 PI])) # 6.28319
```

---

**Lets look at some built-in and base functions as well as some essential structures!**
---
`fn` - a helper function for defining functions

This function uses the built-in lambda (see `$`), and is defined in the base library as follows:

```lisp
(def [fn] ($ [params body] [
  def (first params) ($ (rest params) body)
]))
```

As we can see, it creates a global variable (see `def` section), whose value is a lambda function. Then, it uses the `first` argument as the function name, and the `rest` of the arguments as the arguments of the actual function.

---
`use` - import a file into another and evaluate its contents

Let's say we have a file named `add2.fl` with the following function definition:
```lisp
# add2.fl
(fn [add2 x] [+ x 2])
```
We also have another file, `add4.fl` which wants to use the `add2` function. How do we import it? with the built-in `use` function!:

```lisp
# add4.fl

(use 'add2.fl')

(fn [add4 x] [ add2 (add2 x) ])
```

---

`if`

Function that chooses the branch depending on given condition:

```lisp
(if [some-very-complex-expression] [puts 'Then branch'] [puts 'Else branch'])
```

---

`do`

Function that evaluates a list of expressions in order and returns the last one:


```lisp
(do
    (puts 'First')
    (puts 'Second')
    (puts 'Third')
    'Return me'
)
```

---

`let`

Create a scope for stuff to take place in:

```lisp
(let [do
    (= [x] 10)
    (puts x) # 10
])

(puts x) # [error] Use of undefined symbol 'x'

```

---

`$`

Creates a lambda function:

```lisp
(puts (($ [x] [* 2 x]) 10)) # Lamda that multiplies by 2,
# we then pass a parameter of 10, outputs 20
```


---

`def`

Creates a global variable with specified name and initial value:

```lisp
(def [var-name] value)
```


---

`first`

Returns first element from B-expression:

```lisp
(puts (first [3 2 1])) # Outputs 3
```

---

`rest`

Returns rest of B-expression (i.e. the B-expression without the first element):

```lisp
(puts (rest [1 2 3])) # Outputs [2 3]
```

---

`rev`

Reverses B-expression:

```lisp
(puts (rev [1 2 3])) # Outputs [3 2 1]
```

---

`at`

Returns the element the specified index of B-expression:

```lisp
(puts (at 1 [1 2 3])) # Outputs 2
```

---

`len`

Returns length (i.e. number of elements) of B-expression:

```lisp
(puts (len [1 3 5 93.4 ['nested' 'cool']])) # Outputs 5
```

---

`map`

Applies a function on each element of the list:

```lisp
(fn [my-function x] [* 3 x])

(puts (map my-function [1 2 3])) # Outputs [3 6 9]

# We can also use lambdas here

(puts (map ($[x] [* 3 x]) [1 2 3])) # Outputs [3 6 9]
```

---

`range`

Generic range function

`range from to step`

```lisp
(puts (range 1 5 1)) # [1 2 3 4]
(puts (range -1 5 1)) # [-1 0 1 2 3 4]
(puts (range 5 1 1)) # [5 3 4 2]
(puts (range 5 -1 1)) # [5 3 4 2 1 0]
```

---

`abs`

Returns absolute value of expression

```lisp
(puts (abs -4)) # 4
```
