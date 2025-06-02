LDA #10
STA $0200

LDA #3
STA $0201

LDA $0200
CLC
ADC $0201
STA $0202

LDA $0200
SEC
SBC $0201
STA $0203

LDA $0200
; 6502 multiply: JSR Multiply (args: x, y)
STA $0204

LDA $0200
; 6502 divide: JSR Divide (args: x, y)
STA $0205

LDA $0200
; MODULO operation not implemented yet
STA $0206

