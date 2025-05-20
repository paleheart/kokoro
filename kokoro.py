# Kokoro Compiler - Prototype v0.1
# By Kuroshio Industrial Logic Systems

def handle_store(tokens, output):
    # Example: store 42 in x as WholeNumber
    value = tokens[1]
    name = tokens[3]
    # For now, we ignore type
    output.write(f"LDA #{value}\nSTA {name}\n\n")

def handle_print(tokens, output):
    # Example: print("Hello")
    output.write("; PRINT not implemented yet\n\n")

def handle_input(tokens, output):
    # Example: input prompt "Your name?" store in name as Word
    output.write("; INPUT not implemented yet\n\n")

def handle_unknown(tokens, output):
    output.write(f"; Unrecognized line: {' '.join(tokens)}\n\n")

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
