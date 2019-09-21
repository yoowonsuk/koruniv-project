let rec factorial a =
  if a = 1 then 1 else a * factorial (a-1);;
  
let rec factorial a =
  match a with
    1 -> 1
    |_ -> a * factorial(a-1);;
    
let isabc c = if c = 'a' then true
              else if c = 'b' then true
              else if c = 'c' then true
              else false

let isabc c =
  match c with
    'a' -> true
    | 'b' -> true
    | 'c' -> true
    | _ -> false

let isabc c =
  match c with
    'a' | 'b' | 'c' -> true
    | _ -> false
    
let sum_if_true (test : int -> bool) (x : int) (y : int) : int =
  (if test x then x else 0) + (if test y then y else 0);;
  
let id x = x;;
id 1;;
id "abc";;
id true;;

let first_if_true test x y =
  if test x then x else y;;
  
let fst p = match p  with (x,_) -> x;;
fst(1, 2);;
(*
fst(1);;
fst(1, 2, 3);;
*)

let fst (x,_) = x;;

let p = (1, true);;
let (x, y) = p;;

(1, 2) = (2, 1);;

let isnil l =
  match l with
    [] -> true
    | _ -> false;;
    
isnil [1];;
isnil [];;

let rec length l =
  match l with 
    [] -> 0
    | h::t -> 1 + length t;;
    
length [1;2;3];;

type days = Mon | Tue | Wed | Thu | Fri | Sat | Sun;;

Mon;;

let nextday d =
  match d with
    | Mon -> Tue | Tue -> Wed | Wed -> Thu | Thu -> Fri | Fri -> Sat | Sat -> Sun | Sun -> Mon ;;

nextday Mon;;

type shape = Rect of int * int | Circle of int;;

Rect (2, 3);;
Circle(5);;

let area s =
  match s with
    Rect (w, h) -> w * h
    | Circle r -> r * r * 3;; (* not 3.14 *)


area (Rect (2, 3));;
area (Circle (5));;

type intlist = Nil | Cons of int * intlist;;
Nil;;
Cons(1, Nil);;
Cons(1, Cons(2, Nil));;
