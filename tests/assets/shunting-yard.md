> To implement right-hand associativity in the Shunting-yard algorithm, you should modify the condition for popping operators from the stack. Instead of popping operators of greater or equal precedence, you only pop operators with strictly greater precedence. This leaves operators of the same precedence on the stack, ensuring that the rightmost operator is processed last, which is the defining characteristic of right-associativity. Shunting-yard algorithm with right-hand associativity

# Shunting-yard with right-hand associativity

The main change is in the rule for handling an incoming operator token:

* If the incoming token is an operator:
  - While there is an operator token op1 at the top of the operator stack AND either (the precedence of op1 is strictly greater than the precedence of the incoming operator) or (the precedence of op1 is equal to the precedence of the incoming operator AND op1 is left-associative):
    - Pop op1 from the operator stack and add it to the output queue.
  - Push the incoming operator onto the operator stack.

# Example

Consider the expression \(a^{b^{c}}\) with right-associative exponentiation.

1. Process a: Push a to the output.
2. Process ^: Stack is empty, so push ^ onto the operator stack.
3. Process b: Push b to the output.
4. Process ^:
    - The incoming ^ has the same precedence as the one on the stack, but is right-associative.
    - Because the condition for popping is "strictly greater precedence," we do not pop the stack.
    - Push the new ^ onto the stack.
5. Process c: Push c to the output.
6. End of input: Pop operators from the stack and add them to the output. Both ^ operators will be popped.

> The resulting output queue will be a b c ^ ^, correctly representing \(a^{(b^{c})}\). If the algorithm were left-associative, the output would be a b ^ c ^ (\((a^{b})^{c}\)), due to the different rule for popping on equal precedence. 
