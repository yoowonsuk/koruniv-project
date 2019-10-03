type pgm = exp
and exp =
  CONST of int
  | VAR of string
  | ADD of exp * exp
  | SUB of exp * exp
  | ISZERO of exp
  | IF of exp * exp * exp
  | LET of string * exp * exp
  | READ;;
  
let pgm1 =
  LET ("x", CONST 1, LET ("y", LET ("x", CONST 2, ADD (VAR "x", CONST 2)), ADD( VAR "x", VAR "y")));;
  
type value = INT of int | BOOL of bool;;

module Env = struct
  type t = (string * value) list;;
  let empty = []
  let rec lookup x e =
    match e with
      [] -> raise (Failure "Env")
      | (y,v)::tl -> if y=x then v else lookup x tl
  let update x v e = (x,v)::e
end

(*
module Env = struct
  type t = string -> value
  let empty = fun _ -> raise (Failure "empty")
  let lookup x e = e x
  let update x v e = fun y -> if x = y then v else e x
end
*)

let rec binop env e1 e2 op =
  let v1 = eval env e1 in
  let v2 = eval env e2 in
    (match v1, v2 with
      INT n1, INT n2 -> INT (op n1 n2)
      | _ -> raise (Failure ("error: Binop must take integers")))
          
and eval : Env.t -> exp -> value
= fun env exp ->
  match exp with
    CONST n -> INT n
    | VAR x -> Env.lookup x env
    (*
    | ADD (e1, e2) -> 
      let v1 = eval env e1 in
      let v2 = eval env e2 in
        (match v1, v2 with
          INT n1, INT n2 -> INT (n1 + n2)
          | _ -> raise (Failure ("error: Addition must take integers")))
    | SUB (e1, e2) ->
      let v1 = eval env e1 in
      let v2 = eval env e2 in
        (match v1, v2 with
          INT n1, INT n2 -> INT (n1 - n2)
          | _ -> raise (Failure ("error: Subtraction must take integers")))
    *)
    | ADD (e1, e2) -> binop env e1 e2 (+)
    | SUB (e1, e2) -> binop env e1 e2 (-)
    | READ -> INT (read_int ())
    | ISZERO e -> (match (eval env e) with
                    INT 0 -> BOOL true
                    | INT _ -> BOOL false
                    | _ -> raise (Failure "XX"))
    | IF (e1, e2, e3) -> (match eval env e1 with
                          BOOL true -> eval env e2
                          | BOOL false -> eval env e3
                          | _ -> raise (Failure "XX"))
    | LET (x, e1, e2) ->
      let v1 = eval env e1 in
      let v = eval (Env.update x v1 env) e2 in v;;
      
eval Env.empty pgm1;;
