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

def math_eval(tokens, output):
    

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
