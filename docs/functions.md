## Useful functions

- **fn** - a helper function for defining functions.
    ```
    def [fn] ($ [params body] [def (car params) ($ (cdr params) body)])
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
