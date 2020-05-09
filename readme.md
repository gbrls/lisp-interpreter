# What is myLisp?
myLisp is a lisp-like language that I created in order to learn how to write an
interpreter for a lisp-like language.

## Features

Some basic Lisp functionality:
```lisp
(quote (a b c d))
> (A B C D)
(atom (quote ()))
> T
(atom (quote (a b)))
> nil
(cons (quote a) (cons (quote b) nil))
> (A B)
(length (quote (a b c d e)))
> 5
((lambda (x y) (cons y x)) (quote a) (quote b))
> (B . A)
```
Setting variables:
```lisp
(setf a (cons (quote a) (quote a)))
> nil
a
> (A . A)
```
Calling functions:
```lisp
(setf length+1 (quote (lambda (list) (length (cons (quote a) list)))))
> nil 
(length+1 (quote (a b)))
> 3
```

# Missing features
+ Garbage Collection.
+ Lexical scope.
+ Be usable.
