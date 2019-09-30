let rec factorial a = if a = 1 then 1 else a * factorial (a - 1);;

(* factorial 10000000000;; *)

let rec fold_right f l a = match l with | [] -> a | hd::tl -> f hd (fold_right f tl a);;
let rec fold_left f a l = match l with
                          [] -> a
                          | h::t -> fold_left f (f a h) t;;

let sum lst = fold_right (fun x y -> x + y) lst 0;;

let rec range n m a =
  if n = m then a
  else range (n+1) m (n::a);;
  
let ll = range 1 100 [];;

let length l = fold_left (fun a x -> a+ 1) 0 l;;

length ll;;

let rec filter f l =
  match l with
    [] -> []
    | h::t -> if f h then (filter f t) else l;;

let drop
= filter (fun x -> x mod 2 = 1);; (*TODO*)

drop [1; 3; 5; 6; 7];;

let rec iter (n, f) a
= if n = 0 then a else iter ((n-1), f) (f a);;

iter(10, fun x -> x + 2) 0;;
