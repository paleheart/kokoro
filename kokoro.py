# Kokoro Compiler - Prototype v0.1
# By Kuroshio Industrial Logic Systems

def handle_store(tokens, output, symbol_table=None):
    # Find positions of key keywords
    in_index = tokens.index("in")
    as_index = tokens.index("as")

    # Extract destination name and optional type
    name = tokens[in_index + 1]
    var_type = tokens[as_index + 1] if as_index + 1 < len(tokens) else None

    # Get everything between 'store' and 'in' as the expression
    expr_tokens = tokens[1:in_index]

    # Pass to math evaluator (returns either a constant or instruction list)
    value = math_eval(expr_tokens)

    # Emit for now as simple constant assignment
    output.write(f"LDA #{value}\nSTA {name}\n\n")

def handle_print(tokens, output):
    # Example: print("Hello")
    output.write("; PRINT not implemented yet\n\n")

def handle_input(tokens, output):
    # Example: input prompt "Your name?" store in name as Word
    output.write("; INPUT not implemented yet\n\n")

def handle_unknown(tokens, output):
    output.write(f"; Unrecognized line: {' '.join(tokens)}\n\n")

def math_eval(tokens):
    """
    Evaluates a mathematical expression token list using PEMDAS-style reduction.
    Returns either a single numeric value (for constants) or a structure for later codegen.
    """

    def evaluate_flat_expression(tokens):
        # Reduce a flat, parenthesis-free list of tokens by precedence
        precedence = [
            ["*", "/"],
            ["+", "-"]
        ]

        tokens = tokens[:]  # make a mutable copy

        for ops in precedence:
            i = 0
            while i < len(tokens):
                if tokens[i] in ops:
                    left = float(tokens[i - 1])
                    right = float(tokens[i + 1])
                    operator = tokens[i]

                    if operator == "*":
                        result = left * right
                    elif operator == "/":
                        result = left / right
                    elif operator == "+":
                        result = left + right
                    elif operator == "-":
                        result = left - right
                    else:
                        raise Exception(f"Unknown operator: {operator}")

                    # Replace [left, op, right] with result
                    tokens[i - 1:i + 2] = [str(result)]
                    i = max(i - 1, 0)
                else:
                    i += 1

        return tokens[0]  # should be one value left

    def resolve_parentheses(tokens):
        # Recursively resolve innermost ( ... ) first
        i = 0
        while i < len(tokens):
            if tokens[i] == "(":
                depth = 1
                j = i + 1
                while j < len(tokens):
                    if tokens[j] == "(":
                        depth += 1
                    elif tokens[j] == ")":
                        depth -= 1
                        if depth == 0:
                            break
                    j += 1
                if depth != 0:
                    raise Exception("Mismatched parentheses")

                # Evaluate subexpression
                subexpr = tokens[i + 1:j]
                result = resolve_parentheses(subexpr)
                result = evaluate_flat_expression([result])

                # Replace ( ... ) with result
                tokens[i:j + 1] = [result]
                i = max(i - 1, 0)
            else:
                i += 1

        return evaluate_flat_expression(tokens)

    try:
        result = resolve_parentheses(tokens)
        return float(result)
    except Exception as e:
        print("Math evaluation error:", e)
        return None


# Command name to function mapping
handlers = {
    "store": handle_store,
    "print": handle_print,
    "input": handle_input
}

# Main compile loop
with open("example.kokoro", "r") as source, open("output.asm", "w") as output:
    for line in source:
        # Strip leading/trailing whitespace, make lowercase
        line = line.strip().lower()
        if not line or line.startswith("#"):
            continue  # skip blank lines and comments

        tokens = line.split()
        keyword = tokens[0]

        # Find matching handler or use fallback
        handler = handlers.get(keyword, handle_unknown)
        handler(tokens, output)
