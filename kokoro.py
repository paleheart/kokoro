#kokoro

#Kuroshio Industrial Logic Systems

# compiler.py
with open("example.kokoro", "r") as source, open("output.asm", "w") as output:
    for line in source:
        line = line.strip().lower()
        if line.startswith("store"):
            parts = line.split()
            value = parts[1]
            name = parts[3]
            output.write(f"LDA #{value}\nSTA {name}\n\n")
        elif line.startswith("print"):
            # super simple fake print for now
            output.write(f"; PRINT not implemented yet\n\n")
        else:
            output.write(f"; Unrecognized line: {line}\n")
