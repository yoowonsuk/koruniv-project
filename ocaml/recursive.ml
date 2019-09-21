type exp =
  Int of int
  | Minus of exp
  | Plus of exp * exp
  | Mult of exp * exp;;
  
(* 1 + 2 * (-3) => -5 *)
let e = Plus(Int 1, Mult(Int 2, Minus(Int 3)));;

let rec eval : exp -> int
= fun exp -> match exp with
  Int n -> n
  | Minus exp' -> - (eval exp')
  | Plus (e1, e2) -> eval e1 + eval e2
  | Mult(e1, e2) -> eval e1 * eval e2;;

eval e;;

(*
let div a b =
  try
    a / b
  with Division_by_zero -> 0;;

10 / 2;;
10 / 0;;

exception Problem

let div a b =
  if b = 0 then raise Problem
  else a/b;;
  
10 / 2;;
10 / 0;;

try
  div 10 0
  with Problem -> 0;;
*)
  
let rec append l1 l2 =
  match l1 with
    [] -> l2
    | h::t -> h::(append t l2);;
    
append [1;2;3] [4; 5; 6];;

let rec append_last l a =
  match l with
    [] -> [a]
    | h::t -> h::(append_last t a);;
    
let rec append l1 l2 =
  match l2 with
    [] -> l1
    | h::t -> append (append_last l1 h) t;;
    
append ['a'; 'b'; 'c'; 'd'] ['e'; 'f'; 'g'; 'h'];;
