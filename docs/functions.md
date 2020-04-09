## Useful function ideas for the standard library

- **fn** - a helper function for defining functions.
    ```
    def [fn] ($ [params body] [
        def (first params) ($ (rest params) body)
    ])
    ```
    Example:
    ```
    => fn [dec x] [- x 1]
    ()
    => dec 3
    2
    ```
- **pack** - pack arguments `args` to a list and call `function` on it.
    ```
    fn [pack function @ args] [function args]
    ```
    Example:
    ```
    => attach [1 2] (pack attach 3 4 5)
    [1 2 3 4 5]
    ```
- **unpack** - unpack list `lst` and call `function` with its arguments.
    ```
    fn [unpack function lst] [eval (attach [function] lst)]
    ```
    Example:
    ```
    => unpack max [104.5 3.14 786 2]
    786
    ```
- **len** - return the length (i.e. the number of elements) of list `lst`.
    ```
    fn [len lst] [
        if (== lst [])
        [0]
        [+ 1 (len (rest lst))]
    ]
    ```
    Example:
    ```
    => def [primes] [2 3 5 7 11]
    ()
    => len primes
    5
    ```
- **rev** - reverse list `lst`.
    ```
    fn [rev lst] [
        if (== lst [])
        [[]]
        [attach (rev (rest lst)) (first lst)]
    ]
    ```
    Example:
    ```
    => def [primes] [2 3 5 7 11]
    ()
    => rev (attach [1.5 2.5 3.5] primes)
    [11 7 5 3 2 3.5 2.5 1.5]
    ```
- **fact** - compute the factorial of `n`.
    ```
    fn [fact n] [
        if (< n 1)
        [1]
        [* n (fact (- n 1))]
    ]
    ```
    Example:
    ```
    => fact 5
    120
    ```
