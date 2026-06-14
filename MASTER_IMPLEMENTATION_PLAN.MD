# VM Assembly Interpreter — Implementation Master Plan
## CCP6124 | P1 Owner Document

---

## 1. Project Overview

A single `.cpp` file that reads a `.asm` text file (via `argv[1]`), parses it into
instruction objects, runs them on a virtual machine, and dumps the final VM state.
One run, one file, no loops back.

**VM Architecture:**
- 8 general-purpose registers R0–R7, each `int8_t` (signed, -128 to 127)
- 64-cell `int8_t` memory array, addresses 0–63
- 4 flags: CF, ZF, OF, UF (all bool)
- PC: starts at 0, Runner increments it after every instruction
- SI: stack index, managed inside CPU, incremented on PUSH / decremented on POP
- Stack: 8-byte structure, lives inside CPU

---

## 2. Decisions Log (All Q&A Resolved)

| # | Decision | Answer |
|---|----------|--------|
| Q1 | Factory for instruction dispatch | Big if-else chain inside `Runner::parseProgram()`. No separate factory class. |
| Q2 | Whitespace/comma fix | Replace every `,` with `' '` before tokenising. Handles `R0,R1` and `R0 ,   R1` equally. |
| Q3 | Store operands as string or int | **Parse to int at parse time.** `execute()` only does arithmetic, never string work. |
| Q4 | RESET behaviour | `RESET <FlagName>` zeros a **specific flag**, not all registers. `ResetInstruction` stores `flagTarget: int`. |
| Q5 | RESET class placement | **Direct child of `Instruction`**, not under `MoveInstruction`. `MoveInstruction` now has exactly 3 children. |
| Q6 | Instruction hierarchy (mid-level abstracts) | Keep the abstract intermediate classes (ArithmeticInstruction etc.). They hold shared fields and constructor. Each leaf is tiny (~8 lines). |
| Q7 | MOV disambiguation | Check second token: starts with `R` → `MovRegReg`, starts with `[` → `MovIndirect`, digit → `MovImm`. |
| Q8 | Arithmetic second operand type | Support both register and literal. Add `isNum: bool` to `ArithmeticInstruction`. If `isNum` is true, use `srcVal` directly; if false, use it as a register index. |
| Q9 | INC/DEC under ArithmeticInstruction | `IncInstruction::execute()` does `val + 1` directly. `DecInstruction` does `val - 1`. The inherited `srcVal` field is not used — no marks deducted, no special case needed. |
| Q10 | Comments/blanks in .asm | Skip lines that are empty after trimming whitespace. No comment handling needed for test files. |
| Q11 | Stack location | Stack (`CustomStack`) lives **inside CPU**. `PUSH`/`POP` instructions call `vm.getCPU().push(val)` / `vm.getCPU().pop()`. |
| Q12 | Flags on MOV | **Yes**, MOV instructions call `setFlags()` after writing — ZF if result is 0, OF/UF if out of range. Safer for examiner edge cases. |
| Q13 | RESET flag encoding | Top-of-file `enum FlagCode { CF=0, ZF=1, OF=2, UF=3 };`. P1 uses it in parsing, P2 uses it in FlagRegister. Single source of truth. |
| Q14 | Cross-team stubs | P1 ships a complete compilable file on day one with empty method bodies for all classes. Other people fill in only their `execute()` / method bodies. Runner is never touched again. |
| Q15 | CustomQueue ownership | P1 writes the stub (3 methods: `enqueue`, `dequeue`, `isEmpty`). P3 fills in the real implementation. |
| Q16 | Dump format | Each class owns its own print section. `VirtualMachine::printState()` calls them in order. Format must match exactly — see Section 5. |
| Q17 | Who calls printState() | `Runner::run()` calls `vm.printState()` as its last line. `main()` stays clean. |
| Q18 | Unknown opcode | `cerr << "Unknown instruction: " << opcode` then `exit(1)`. |
| Q19 | Out-of-range memory | `Memory::read()` / `write()` print error to `cerr` and call `exit(1)`. |

---

## 3. Updated Class Hierarchy

```
Instruction  (abstract)
├── ArithmeticInstruction  (abstract)  # destReg: int, srcVal: int, isNum: bool
│   ├── AddInstruction
│   ├── SubInstruction
│   ├── MulInstruction
│   ├── DivInstruction
│   ├── IncInstruction
│   └── DecInstruction
├── ShiftInstruction  (abstract)  # registerNum: int, amount: int
│   ├── ShlInstruction
│   ├── ShrInstruction
│   ├── RolInstruction
│   └── RorInstruction
├── IOInstruction  (abstract)  # registerNum: int
│   ├── InputInstruction
│   ├── DisplayInstruction
│   ├── LoadRegInstruction
│   ├── LoadMemInstruction
│   ├── StoreRegInstruction
│   └── StoreMemInstruction
├── MoveInstruction  (abstract)  # rDest: int
│   ├── MovRegRegInstruction     # + rSrc: int
│   ├── MovImmInstruction        # + immediateValue: int
│   └── MovIndirectInstruction   # + rSrc: int
├── StackInstruction  (abstract)  # registerNum: int
│   ├── PushInstruction
│   └── PopInstruction
└── ResetInstruction             # flagTarget: int  (DIRECT child — not under Move)
```

**Register hierarchy (P3 writes, must have virtual):**
```
Register  (abstract, virtual getValue/setValue)
├── GeneralRegister    # name: string
└── FlagRegister       # CF, ZF, OF, UF: bool
```

---

## 4. P1 Scope — What P1 Writes

### Classes P1 owns entirely:
- `CustomVector<T>` — template, dynamic array
- `Instruction` — abstract base
- `MoveInstruction` — abstract intermediate
- `MovRegRegInstruction` — execute: `R[dest] = R[src]`
- `MovImmInstruction` — execute: `R[dest] = immediateValue`
- `MovIndirectInstruction` — execute: `R[dest] = memory[R[src]]`
- `ResetInstruction` — execute: zero one specific flag
- `Runner` — loadProgram, parseProgram, run, destructor

### Shells P1 writes (other people fill in bodies):
- `CustomStack<T>` — stub with push/pop/peek/isEmpty
- `CustomQueue<T>` — stub with enqueue/dequeue/isEmpty
- `Register` — abstract base (P3 fills GeneralRegister/FlagRegister)
- `GeneralRegister` — shell (P3 fills)
- `FlagRegister` — shell (P2 fills setFlags/getCF/getZF/getOF/getUF)
- `Memory` — shell (P4 fills read/write/print)
- `CPU` — shell (P2 fills push/pop/peek, incrementPC, getPC)
- `VirtualMachine` — shell (P1 writes the composition + getters, teams fill in printState sections)
- All other instruction leaf classes — empty `execute()` bodies (teams fill in)

---

## 5. Parsing Algorithm (Runner)

### Phase 1 — `loadProgram(filename)`
```
open file via argv[1]
if file fails → cerr + exit(1)
for each line:
    trim leading/trailing whitespace
    if line is empty → skip
    replace every ',' with ' '
    rawLines.push_back(cleanedLine)
close file
```

### Phase 2 — `parseProgram()`
```
for each line in rawLines:
    tokenise with stringstream → word[0]=opcode, word[1]=op1, word[2]=op2
    
    parse op1 (if exists):
        strip 'R' prefix + cast to int → register index
        strip '[' ']' + check if starts with 'R' → indirect register index
        strip '[' ']' + cast to int → direct address
        plain int → immediate value
    
    if opcode == "MOV":
        if op2 starts with '[' → new MovIndirectInstruction(dest, src)
        else if op2 starts with 'R' → new MovRegRegInstruction(dest, src)
        else → new MovImmInstruction(dest, val)
    else if opcode == "RESET":
        map "CF"→0, "ZF"→1, "OF"→2, "UF"→3
        new ResetInstruction(flagCode)
    else if opcode == "ADD" → ...
    ... (full if-else chain for all opcodes)
    
    program.enqueue(instruction*)
```

### Phase 3 — `run()`
```
while program is not empty:
    Instruction* inst = program.dequeue()
    inst->execute(vm)
    vm.getCPU().incrementPC()
vm.printState()
```

### Phase 4 — `~Runner()` destructor
```
// program (CustomQueue) already consumed, nothing left to delete
// If program could be traversed: delete each Instruction*
```

---

## 6. Exact Output Format

Must match byte-for-byte. `#` is the only separator. No extra spaces.

```
#Begin#
#Registers#0000#0011#0000#0044#0000#0000#0000#0000#
#Flags#0#0#0#0#
#PC#0006#
#Memory#
#0000#0000#0000#0000#0000#0000#0000#0000#
#0000#0000#0000#0000#0000#0000#0000#0000#
#0000#0000#0000#0000#0044#0000#0000#0000#
#0000#0000#0000#0000#0000#0000#0000#0000#
#0000#0000#0000#0000#0000#0000#0000#0000#
#0000#0000#0000#0000#0000#0000#0000#0000#
#0000#0000#0000#0000#0000#0000#0000#0000#
#0000#0000#0000#0000#0000#0000#0000#0000#
#End#
```

**Formatting rules:**
- Registers: zero-padded to 4 digits, positive values `0044`, negative values `-005`
- Flags: `0` or `1` only (CF # ZF # OF # UF order)
- PC: zero-padded to 4 digits
- Memory: 8 values per row, 8 rows (64 total), each zero-padded to 4 digits
- Negative memory/register values: `-` then 3 digits e.g. `-005`

**Who prints what:**
- `GeneralRegister::print()` → handles its own value formatting (P3)
- `FlagRegister::print()` → prints CF#ZF#OF#UF values (P2)
- `Memory::print()` → prints all 64 cells in 8×8 grid (P4)
- `VirtualMachine::printState()` → calls them all in order (P1 shell, teams fill their print methods)

---

## 7. Inter-Team Interface Contracts

These are the method signatures other teams **must not rename**.
P1 writes the signatures. Everyone fills the bodies.

```cpp
// P3 writes bodies:
class GeneralRegister : public Register {
public:
    GeneralRegister(string n);
    int getValue();
    void setValue(int v);
    void print();
};

// P2 writes bodies:
class FlagRegister : public Register {
public:
    void setFlags(int result, int op1, int op2);
    bool getCF(); bool getZF(); bool getOF(); bool getUF();
    void print();
};

// P2 writes bodies for push/pop/peek, incrementPC already trivial:
class CPU {
public:
    int getPC();
    void incrementPC();
    void push(int val);
    int pop();
    int peek();
};

// P4 writes bodies:
class Memory {
public:
    int read(int addr);
    void write(int addr, int val);
    void print();
};

// VirtualMachine getters — P1 writes these, everyone calls them:
VirtualMachine::getCPU()       → CPU&
VirtualMachine::getRegister(i) → GeneralRegister&
VirtualMachine::getFlags()     → FlagRegister&
VirtualMachine::getMemory()    → Memory&
```

---

## 8. Naming Conventions

| Concept | Name chosen | Reason |
|---------|-------------|--------|
| Arithmetic source operand | `srcVal` | Short, obvious |
| "Is the source a literal number?" | `isNum` | Natural, not jargon |
| Destination register index | `rDest` | Matches UML |
| Source register index | `rSrc` | Matches UML |
| Immediate value in MovImm | `immediateValue` | Matches UML, acceptable |
| Flag selector in ResetInstruction | `flagTarget` | Clear purpose |
| Raw lines in Runner | `rawLines` | Self-documenting |
| Parsed instruction queue in Runner | `program` | Matches UML |
| Register array in VirtualMachine | `registers` | Matches UML |
| Flag enum values | `CF=0, ZF=1, OF=2, UF=3` | Defined once at top of file |

---

## 9. Key Constraints (from PDF + Briefing)

- **Single `.cpp` file** — no custom `.h` includes, all in one
- **No STL containers** — Vector, Stack, Queue, Deque, List, Pair, Map, Set all banned
- `<cstdint>` is allowed (`int8_t`, `uint8_t`)
- `<fstream>`, `<iostream>`, `<sstream>` are allowed
- **35-line limit per function** — leaf instruction `execute()` bodies are naturally 5-10 lines
- **Comment who wrote each class/template** at the top of the class
- Every instruction that writes a register must update flags
- PC starts at 0, incremented by Runner (not by instructions)
- Program runs once — no re-run without re-launching the binary

---

## 10. File Structure (Top to Bottom Order in .cpp)

```
1.  #includes
2.  enum FlagCode { CF=0, ZF=1, OF=2, UF=3 };
3.  CustomVector<T>           (P1)
4.  CustomStack<T>            (P2 fills)
5.  CustomQueue<T>            (P3 fills)
6.  Register (abstract)       (P3 fills virtual getValue/setValue)
7.  GeneralRegister           (P3 fills)
8.  FlagRegister              (P2 fills)
9.  Memory                    (P4 fills)
10. CPU                       (P2 fills)
11. VirtualMachine            (P1 shell, teams fill print methods)
12. Instruction (abstract)    (P1)
13. ArithmeticInstruction     (P1 shell, P4 fills leaves)
14.   AddInstruction ... DecInstruction
15. ShiftInstruction          (P1 shell, P2 fills leaves)
16.   ShlInstruction ... RorInstruction
17. IOInstruction             (P1 shell, P3 fills leaves)
18.   InputInstruction ... StoreMemInstruction
19. MoveInstruction           (P1)
20.   MovRegRegInstruction, MovImmInstruction, MovIndirectInstruction
21. ResetInstruction          (P1)
22. StackInstruction          (P1 shell, P2 fills leaves)
23.   PushInstruction, PopInstruction
24. Runner                    (P1)
25. main()
```