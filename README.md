# The GG Programming Language

```h
=> + 8 9
17
=> def [add] (> [a b] [+ a b])
()
=> add 8 9
17
```

```h
=> [pow 5 2]
[pow 5 2]
=> eval [pow 5 2]
25
=> list (eval [pow 5 2]) (/ 100 (* 3.14 7.5))
[25 4.24628]
=> def [l] (list (eval [pow 5 2]) (/ 100 (* 3.14 7.5)))
()
=> l
[25 4.24628]
=> car l
[25]
=> cdr l
[4.24628]
=> attach l [1 2 3]
[25 4.24628 1 2 3]
=> attach l l l
[25 4.24628 25 4.24628 25 4.24628]
```
