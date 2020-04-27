# 🦩 Welcome to Flamingo

Flamingo is a lisp-like programming language designed to be simple, embeddable and self-extending.

```lisp
(println "Hello, World!")
```

Or something a little more advanced, let's define a `map` function:
```lisp
# apply function `func` on each element of list `l`.
($ (map func l)
  (if (= l nil)
    nil
    (join (list (func (first l))) (map func (rest l)))))
```
Simple, isn't it? Now let's use it with a power function:
```c
=> ($ (square x) (* x x))
nil
=> (map square '(4 5 6))
(16 25 36)
```

## Building on Unix-like systems
#### Requirements

* Make
* A modern C compiler
* Did we notice you'll need a computer too? 🤭

Simply run `make` in the root directory of the project.
If everything goes right, you'll have the newly created executable `flamingo` there.

---

## Hello Flamingo

Like every language (we hope), Flamingo is capable of printing stuff to the screen:

```lisp
(println 'I love pizza 🍕!')
```
From this simple example, you can already see 3 things about Flamingo:
* Every expression in Flamingo is parenthesized (except for in REPL, where they are optional).
* `println` is short for "print line" and can output any expression to standard output with a newline (`'\n'`) after it. (there's also just `print`, if you don't want a newline).
* Strings are sequences of characters enclosed in double quotes `("...")`.

### Main concepts

**S-Expression**

`(function param1 param2 ...)`

An s-expression (for "symbolic expression") is a way of writing lists, and using them for code ***and*** data. In the usual parenthesized syntax, an s-expression is classically defined as

1. an atom (e.g. a symbol, number, or a string), or
2. an expression of the form `(a b)` where `a` and `b` are s-expressions.

**`quote` (or just `'` for short)**

`'(a b c ...)`

Like s-expressions, quotes are lists. Unlike s-expressions, they don't get evaluated, they just stay as they are. This is useful when you need to decide if they should ever be executed, or should be executed several times, or just kept as they are. A good example would be the standard library macro '`$`', which is defined as follows:

```lisp
# a helper macro for defining functions.
(set $ (macro (params . body)
  (list 'set (first params) (list 'fn (rest params) (cons 'do body)))))
```

As you can see, we use a quote on `set`, `fn` and `do` in order to force them to just stay as they are. Quotes are very commonly used with macros, e.g. the standard library `for` macro, which allows us to iterate over a list:
```lisp
# iterate over each element of list `l` with symbol `i`.
(set for (macro (i l . body)
  (list 'do
    (list 'let 'it l)
    (list 'while 'it
      (list 'let i '(first it))
      '(set it (rest it))
      (cons 'do body)))))
```
```lisp
(set primes (list 2 3 5 7 11))
(for p primes
  (print " -> " p))

# ->  2 ->  3 ->  5 ->  7 ->  11
```

With the nature of them, you can make lots of interesting things, from arrays to dictionary-like structures:

```lisp
(first '(11 22 33 44 55 66 77)) # 11
```

Mix in some types, why not?

```lisp
'("Hello world" 1 2 3 '(1 "Nested") some-identifier "The next is an expression, that can be evalueted" '(* 2 PI))
```

The following are equivalent: `'(x y z)`, `(list x y z)`.

---

**Lets look at some built-in and base functions as well as some essential structures!**
---
`fn` - a built-in function for defining functions

This function allows us to create user-defined functions, and using them by assigning them to symbols. For example, a simple factorial function can be written as:

```lisp
(set fact (n)
  (if (< n 2)
    n
    (* n (fact (- n 1)))))

(println (fact 6)) # 720
```
Usually, the macro `$` is used to create functions. (it uses `fn` under the hood).

---
`while`
(while condition ...)

Evaluate the rest of the arguments until condition evaluates to nil, which is falsey.

```lisp
(set i 10)
(while (> i 0)
  (print i)
  (dec i))

# 10  9  8  7  6  5  4  3  2  1
```

---

**TODO** `use` - import a file into another and evaluate its contents

Let's say we have a file named `add2.fl` with the following function definition:
```lisp
# add2.fl
($ (add2 x) (+ x 2))
```
We also have another file, `add4.fl` which wants to use the `add2` function. How do we import it? with the built-in `use` function!:

```lisp
# add4.fl

(use "add2.fl")

($ (add4 x) (add2 (add2 x)))
```

---

`if`

Function that chooses the branch depending on given condition:

```lisp
(if (some-very-complex-expression) (println 'Then branch') (println 'Else branch'))
```
As you would expect, you can use multiple conditions with `if`, just like `else if` in other languages:
```lisp
(set name "Michael")
(if (= name "John")
  (println "Hey Johnny boy!")
  (= name "Alice")
  (println "Hey Alice girl!")
  (= name "Michael")
  (println "Michael! Hee Hee")
  (println "I don't know you! get out of my house!"))

# prints "Michael! Hee Hee"
```
`=` is a built-in that checks for equality between two objects.

---

`do`

Function that evaluates a list of expressions in order and returns the last one:

```lisp
(do "First"
    "Second"
    "Third"
    "Return me")

# "Return me"
```

---

`let`

Creates a locally available variable with specified name and initial value:

```lisp
(let var-name value)
```

---


`set`

Creates a globally available variable with specified name and initial value:

```lisp
(set var-name value)
```


---

`first`

Returns first element from list:

```lisp
(println (first '(3 2 1))) # outputs 3
```

---

`rest`

Returns rest of list (i.e. the list without the first element):

```lisp
(println (rest '(1 2 3))) # outputs (2 3)
```

---

`rev`

Reverses list:

```lisp
(println (rev '(.5 1.5 2.5 3.5))) # outputs (3.5 2.5 1.5 .5)
```

---

`at`

Returns the element the specified index of list:

```lisp
(println (at 1 '(1 2 3))) # outputs 2
```

---

`len`

Returns length (i.e. number of elements) of list:

```lisp
(println (len '(1 3 5 93.4 '("nested" "cool")))) # outputs 5
```

---

`map`

Applies a function on each element of the list:

```lisp
($ (my-function x) (* 3 x))

(println (map my-function '(1 2 3))) # outputs (3 6 9)

# We can also use lambdas here

(println (map (fn (x) (* 3 x)) (list 1 2 3))) # same output
```

---

`abs`

Returns absolute value of expression

```lisp
(println (abs -4)) # 4
```
---

For a complete view of the useful functions and macros available in Flamingo, check out the base (standard) libary [here](https://github.com/TomerShech/flamingo/blob/master/lib/base.fl).
