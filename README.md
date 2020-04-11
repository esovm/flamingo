# 🦩 Welcome to Flamingo

```c
(puts 'Hello, World!')
```

Or something a little more advanced, let's define a `map` function:
```py
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
