# Documentation

> **NOTE** the interpreter has a few bugs that will not be fixed as the project
> acheived its purpose.

## Contents

- [Primitive Types](#primitive-types)
- [Built-ins](#built-ins)
- [Language Design](#language-design)
  - [Defining Primitive Types](#defining-primitive-types)
  - [Operations](#operations)
  - [Comparisons](#comparisons)
  - [Flow Control](#flow-control)
  - [I/O](#io)

## Customizing the language

## Primitive Types

- NULL
- NUMBER
- CHAR
- STRING
- ARRAY
- DICTIONARY
- FUNCTION

## Built-ins

- `NULL`
- `FALSE`
- `TRUE`
- `exit(arg)`
  - forces the interpreter to exit with the value of `arg`
  - `arg` can be anything
- `reset()`
  - restarts the interpreter without quitting
- `int(value)`
  - converts `value` to an integer
  - `value` must be a `NUMBER`
  - throws an error if `value` is not a `NUMBER`
- `number(value)`
  - tries to convert `value` to a `NUMBER`
  - if it fails, the program terminates
- `string(value)`
  - converts `value` to a `STRING`
- size(array)
  - returns the length of `array`
  - throws an error if `array` is not a `STRING` or `ARRAY`
- type(value)
  - return the type of `value` as a `STRING`
- `get_dict_keys(dictionary)`
  - returns the keys of `dictionary` as an `ARRAY`
  - throws an error if `dictionary` is not a `DICTIONARY`
- `globals()`
  - returns the variables and functions stored in the global context as a `DICTIONARY`
- `locals()`
  - returns the variables and functions stored in the local context as a `DICTIONARY`
- `global_assign(name, value)`
  - assigns a variable to the global contet
  - `name` represents the name of the variable
  - `value` represents the value of the variable
- `open_file(identifier, path, mode)`
  - opens a file
  - `identifier` represents the name of the opened file
  - `path` represents the path to the file
  - `mode` represents the mode in which the file is opened
    - read -> default `r`
    - write -> default `w`
    - append -> default `a`
- `close_file(identifier)`
  - closes the file associated with `identifier`

## Language Design

### Defining Primitive Types

```text
$ this is a comment

$ assignment
a <- 1
const b <- 2

$ NULL value
NULL

$ NUMBER
0
5
3.14
2.71828
123.456
-7
-0.001
-42.5

$ CHAR
'A'
'3'
'$'
'\n'
'\t'
'\0'
$ STRING
"apple"
"number"
"12345"

$ ARRAY
$ array items can be of any type
[0, 1, 2]
["apple", "banana", "cherry"]
[0, "apple"]

$ DICTIONARY
$ must be defined on one line
$ keys can be number, characters or strings
$ values can be of any type
{ 1:0, '2':"value2", "key3":"value3" }

$ FUNCTION
function identifier(arg1, arg2):
...function body
end
```

### Operations

- `NUMBER`

  - ```text
    $ addition
    1 + 2 $ = 3

    $ subtraction
    1 - 2 $ = -1

    $ multiplication
    1 * 2 $ = 2

    $ division
    1 / 2 $ = 0.5

    $ modulo
    1 % 2 $ = 1
    ```

- `CHAR`

  - ```text
    $ addition
    'A' - 32  $ = 'a'
    's' + "tring" = "string"

    $ subtraction
    'a' - 32  $ = 'A'

    $ multiplication
    'a' * 2 $ = "aa"
    ```

- `STRING`

  - ```text
    $ addition
    "hello " + "world" $ = "hello world"
    "hello world" + '!' $ = "hello world!"
    "hello world" + 6 $ = "world"

    $ multiplication
    "abc" * 2 $ = "abcabc"

    $ get at index
    "abc"[0] $ = 'a'

    $ set at index
    "abc"[0] <- 'A' $ string becomes "Abc"
    ```

- `ARRAY`

  - ```text
    $ addition
    ["hello"] + "world" $ = ["hello", "world"]
    [0, 1] + 2 $ = [0, 1, 2]
    [0, 1] + ["hello", "world"] $ = [0, 1, "hello", "world"]

    $ subtraction
    [0, 1, 2, 3] - 2 $ = [2, 3]

    $ multiplication
    [0, 1] * 2 $ = [0, 1, 0, 1]

    $ get at index
    [0, 1][0] $ = 0

    $ set at index
    [0, 1][0] <- 'a' $ array becomes ['a', 1]
    ```

- `DICTIONARY`

  - ```text
    $ subtraction
    { 1: 2, "key2": "value2" } - "key2" $ = { 1: 2 }

    $ get at index
    { 1: 2, "key2": "value2" }[1] $ = 2

    $ set at index
    { 1: 2, "key2": "value2" }["key2"] <- 'a' $ dictionary becomes
                                                 { 1: 2, "key2": 'a' }
    ```

### Comparisons

- equal: `=`
- different: `<>`
- less than: `<`
- less than or equal: `<=`
- greater than: `>`
- greater than or equal: `=>`
- and: `and`
- or: `or`

### Flow Control

```text
$ examples

if 1>2 then
...body
else
  if 2=2 then
  ...body
  end
end

$ loops support 'break' and 'continue'
for i<-2, i<10 execute
...body
end

$ i will have only odd values
for i<-1, i<10, 2 execute
...body
end

i<-10
while i>0 execute
...body
end

i<-10
do
...body
until i<0

try
...body
end


try
...body
catch
...body
end

function a(arg1, arg2):
...body

return value
end


$ runs the code in 'script.psc'
include "script.psc"

$ resets the interpreter and runs the code in 'script.psc'
run "script.psc"
```

### I/O

- stdin, stdout

  - ```text
    read a
    print TRUE = FALSE
    ```

- files

  - ```text
    $ opening a file
    open_file("f","file.txt","r")

    $ reading line by line from a file
    read line : f

    $ closing a file
    close_file("f")

    $ writing to a file
    open_file("g","file.txt","w")
    print "value" : g
    ```

## Customizing the language

If you want to tweak the language, just head over to the 'Resources' folder and
modigy the files. Inside each file, you'll find key-value pairs on separate
lines. But here's the catch: don't mess with the first string in each pair
(that's the key). Instead, focus on the second word (that's the part you can change).
