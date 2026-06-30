; to test StoreMemInstruction + LoadMEmInstruction

MOV R0, 55
MOV R1, 10
STORE [R1], R0
MOV R0, 0
LOAD R0, [R1]
DISPLAY R0
