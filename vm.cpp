#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdint>
using namespace std;

// Flag selector — P1 defines, P2 uses in FlagRegister
enum FlagCode { FLAG_CF = 0, FLAG_ZF = 1, FLAG_OF = 2, FLAG_UF = 3 };

// ============================================================
// CustomVector<T>  — CHIAM JUIN HOONG
// Dynamic array. Replaces std::vector.
// ============================================================
template <typename T>
class CustomVector {
    T*  data;
    int sz;
    int cap;

    void grow() {
        cap *= 2;
        T* tmp = new T[cap];
        for (int i = 0; i < sz; i++) tmp[i] = data[i];
        delete[] data;
        data = tmp;
    }

public:
    CustomVector() : sz(0), cap(8) { data = new T[cap]; }
    ~CustomVector() { delete[] data; }

    void push_back(const T& item) {
        if (sz == cap) grow();
        data[sz++] = item;
    }

    T& get(int i) {
        if (i < 0 || i >= sz) {
            cerr << "CustomVector: index out of bounds" << endl;
            exit(1);
        }
        return data[i];
    }

    int  getSize() const { return sz; }
    bool isEmpty() const { return sz == 0; }
};

// ============================================================
// CustomStack<T>  — Author: helyz
// Fixed 8-slot stack used inside CPU.
// ============================================================
template <typename T>
class CustomStack {
    T   data[8];
    int top;
public:
    CustomStack() : top(-1) {}

    void push(const T& val) {
        if (top >= 7) { cerr << "Stack overflow\n"; exit(1); }
        data[++top] = val;
    }

    T pop() {
        if (top < 0) { cerr << "Stack underflow — program crash\n"; exit(1); }
        return data[top--];
    }

    T    peek()    const { return data[top]; }
    bool isEmpty() const { return top < 0;  }
};

// ============================================================
// CustomQueue<T>  — Author: [P3]
// Circular array queue. Stores Instruction* during run.
// ============================================================
template <typename T>
class CustomQueue {
    T*  data;
    int head, tail, sz, cap;
public:
    CustomQueue() : head(0), tail(0), sz(0), cap(64) {
        data = new T[cap];
    }
    ~CustomQueue() { delete[] data; }

    void enqueue(const T& item) {
        if (sz == cap) { cerr << "Queue full\n"; exit(1); }
        data[tail] = item;
        tail = (tail + 1) % cap;
        sz++;
    }

    T dequeue() {
        if (sz == 0) { cerr << "Queue empty\n"; exit(1); }
        T item = data[head];
        head = (head + 1) % cap;
        sz--;
        return item;
    }

    bool isEmpty() const { return sz == 0; }
};

// ============================================================
// Register (abstract base)  — Author: [P3]
// GeneralRegister             — Author: [P3]
// FlagRegister                — Author: Helyz
// ============================================================
class Register {
protected:
    int value;
public:
    Register() : value(0) {}
    virtual ~Register() {}
    virtual int  getValue() const { return value; }
    virtual void setValue(int v)  { value = v; }
};

class GeneralRegister : public Register {
    string name;
public:
    GeneralRegister()               {}
    GeneralRegister(const string& n) : name(n) { value = 0; }
    int  getValue() const   { return value; }        // P3 fills
    void setValue(int v)    { value = (int8_t)v; }   // P3 fills (clamps to int8)
    void print() const      { 
        int v = value;
        if (v < 0) {
            cout << "#";

            int display = ( v < 0 ) ? ( v + 256 ) : v;
            if (display < 1000) cout << "0";
            if (display < 100) cout << "0";
            if (display < 10) cout << "0";
            cout << display;
        } else {
            cout << "#";
            if (v < 1000) cout << "0";
            if (v < 100) cout << "0";
            if (v < 10) cout << "0";
            cout << v;
        }
    }
};

class FlagRegister : public Register {
    bool CF, ZF, OF, UF;
public:
    FlagRegister() : CF(false), ZF(false), OF(false), UF(false) {}

    void setFlags(int result, int op1, int op2); // P2 fills

    void resetFlag(FlagCode f) {
        if      (f == FLAG_CF) CF = false;
        else if (f == FLAG_ZF) ZF = false;
        else if (f == FLAG_OF) OF = false;
        else if (f == FLAG_UF) UF = false;
    }

    bool getCF() const { return CF; }
    bool getZF() const { return ZF; }
    bool getOF() const { return OF; }
    bool getUF() const { return UF; }
    void print() const {
        cout << "#Flags#"
             << (CF ? 1 : 0) << "#"
             << (ZF ? 1 : 0) << "#"
             << (OF ? 1 : 0) << "#"
             << (UF ? 1 : 0) << "#\n";
    }
};

// ============================================================
// Memory  — Author: [P4]
// 64-cell signed byte array.
// ============================================================
class Memory {
    int8_t mem[64];
public:
    Memory() { for (int i = 0; i < 64; i++) mem[i] = 0; }

    int read(int addr) {           // P4 fills (add bounds check)
        if (addr < 0 || addr > 63) {
            cerr << "Memory Error: Address " << addr << "out of bounds for read" << endl;
            exit(1);
        }
        return mem[addr];
    }

    void write(int addr, int val) { // P4 fills (add bounds check)
        if (addr < 0 || addr > 63) {
            cerr << "Memory Error: Address " << addr << " out of bounds for write." << endl;
            exit(1);
        }
        mem[addr] = (int8_t)val;
    }

    void print() const {
        cout << "#Memory#\n";

        for (int i = 0; i < 64; i++) {
            int v = mem[i];
            cout << "#";

            if (v < 0) {
                cout << "-";
                int display = -v; //Converts to positive for printing padding

                if (display < 100) 
                    cout << "0";

                if (display < 10) cout << "0";
                cout << display;    
            } 
            else {
                if (v < 1000)
                    cout << "0";
                
                if (v < 100)
                    cout << "0";
                
                if (v < 10)
                    cout << "0";

                cout << v;
            }

            //Add a new line after every 8th item to make the grid
            if ((i + 1) % 8 == 0) {
                cout << "#\n";
            }
        }
    }
};

// ============================================================
// CPU  — Author: Helyz
// Owns the stack and program counter.
// ============================================================
class CPU {
    int pc;
    CustomStack<int> stack;
public:
    CPU() : pc(0) {}
    int  getPC()        const { return pc; }
    void incrementPC()        { pc++; }
    void push(int val)        { stack.push(val); }  // P2 manages SI
    int  pop()                { return stack.pop(); }
    int  peek()         const { return stack.peek(); }
    void print() const {
        cout << "#PC#";
        if      (pc < 10)   cout << "000";
        else if (pc < 100)  cout << "00";
        else if (pc < 1000) cout << "0";
        cout << pc << "#\n";
    }
};

// ============================================================
// VirtualMachine  — Author: [P1] shell
// Owns all components. Teams call getX() in execute().
// ============================================================
class VirtualMachine {
    CPU             cpu;
    Memory          memory;
    FlagRegister    flags;
    CustomVector<GeneralRegister> registers;
public:
    VirtualMachine() {
        const string names[] =
            {"R0","R1","R2","R3","R4","R5","R6","R7"};
        for (int i = 0; i < 8; i++)
            registers.push_back(GeneralRegister(names[i]));
    }

    CPU&             getCPU()          { return cpu;            }
    GeneralRegister& getRegister(int i){ return registers.get(i);}
    FlagRegister&    getFlags()        { return flags;          }
    Memory&          getMemory()       { return memory;         }
    void printState() const;  // P1 fills last (calls each print())
};

// ============================================================
// Instruction  — CHIAM JUIN HOONG
// Abstract base for all assembly commands.
// ============================================================
class Instruction {
public:
    virtual void   execute(VirtualMachine& vm) = 0;
    virtual string getName() = 0;
    virtual ~Instruction() {}
};

// ============================================================
// ArithmeticInstruction  — Author: [P1] shell, [P4] fills leaves
// isNum: true  → srcVal is a literal integer
//        false → srcVal is a register index
// ============================================================
class ArithmeticInstruction : public Instruction {
protected:
    int  destReg;
    int  srcVal;
    bool isNum;
public:
    ArithmeticInstruction(int d, int s, bool n)
        : destReg(d), srcVal(s), isNum(n) {}
    string getName() { return "ARITH"; }
};

class AddInstruction : public ArithmeticInstruction {
public:
    AddInstruction(int d, int s, bool n)
        : ArithmeticInstruction(d, s, n) {}
    void   execute(VirtualMachine& vm); // P4 fills
    string getName() { return "ADD"; }
};

class SubInstruction : public ArithmeticInstruction {
public:
    SubInstruction(int d, int s, bool n)
        : ArithmeticInstruction(d, s, n) {}
    void   execute(VirtualMachine& vm); // P4 fills
    string getName() { return "SUB"; }
};

class MulInstruction : public ArithmeticInstruction {
public:
    MulInstruction(int d, int s, bool n)
        : ArithmeticInstruction(d, s, n) {}
    void   execute(VirtualMachine& vm); // P4 fills
    string getName() { return "MUL"; }
};

class DivInstruction : public ArithmeticInstruction {
public:
    DivInstruction(int d, int s, bool n)
        : ArithmeticInstruction(d, s, n) {}
    void   execute(VirtualMachine& vm); // P4 fills
    string getName() { return "DIV"; }
};

// INC/DEC: inherit srcVal=1, isNum=true for consistency
class IncInstruction : public ArithmeticInstruction {
public:
    IncInstruction(int d) : ArithmeticInstruction(d, 1, true) {}
    void   execute(VirtualMachine& vm); // P4 fills: dest = dest + 1
    string getName() { return "INC"; }
};

class DecInstruction : public ArithmeticInstruction {
public:
    DecInstruction(int d) : ArithmeticInstruction(d, 1, true) {}
    void   execute(VirtualMachine& vm); // P4 fills: dest = dest - 1
    string getName() { return "DEC"; }
};

// ============================================================
// ShiftInstruction  — Author: [P1] shell, helyz
// ============================================================
class ShiftInstruction : public Instruction {
protected:
    int registerNum;
    int amount;
public:
    ShiftInstruction(int r, int a) : registerNum(r), amount(a) {}
    string getName() { return "SHIFT"; }
};

class ShlInstruction : public ShiftInstruction {
public:
    ShlInstruction(int r, int a) : ShiftInstruction(r, a) {}
    void execute(VirtualMachine& vm); // P2 fills
    string getName() { return "SHL"; }
};

class ShrInstruction : public ShiftInstruction {
public:
    ShrInstruction(int r, int a) : ShiftInstruction(r, a) {}
    void execute(VirtualMachine& vm); // P2 fills
    string getName() { return "SHR"; }
};

class RolInstruction : public ShiftInstruction {
public:
    RolInstruction(int r, int a) : ShiftInstruction(r, a) {}
    void execute(VirtualMachine& vm); // P2 fills
    string getName() { return "ROL"; }
};

class RorInstruction : public ShiftInstruction {
public:
    RorInstruction(int r, int a) : ShiftInstruction(r, a) {}
    void execute(VirtualMachine& vm); // P2 fills
    string getName() { return "ROR"; }
};

// ============================================================
// IOInstruction  — Author: [P1] shell, [P3] fills leaves
// ============================================================
class IOInstruction : public Instruction {
protected:
    int registerNum;
public:
    IOInstruction(int r) : registerNum(r) {}
    string getName() { return "IO"; }
};

class InputInstruction : public IOInstruction {
public:
    InputInstruction(int r) : IOInstruction(r) {}
    void execute(VirtualMachine& vm); // P3 fills
    string getName() { return "INPUT"; }
};

class DisplayInstruction : public IOInstruction {
public:
    DisplayInstruction(int r) : IOInstruction(r) {}
    void execute(VirtualMachine& vm); // P3 fills
    string getName() { return "DISPLAY"; }
};

// LOAD R1, [20]  — destination register + direct address
class LoadRegInstruction : public IOInstruction {
    int addr;
public:
    LoadRegInstruction(int destReg, int a)
        : IOInstruction(destReg), addr(a) {}
    void execute(VirtualMachine& vm); // P3 fills
    string getName() { return "LOAD"; }
};

// LOAD R1, [R2]  — destination register + address-in-register
class LoadMemInstruction : public IOInstruction {
    int addrReg;
public:
    LoadMemInstruction(int destReg, int aReg)
        : IOInstruction(destReg), addrReg(aReg) {}
    void execute(VirtualMachine& vm); // P3 fills
    string getName() { return "LOAD"; }
};

// STORE 20, R3  — direct address + source register
class StoreRegInstruction : public IOInstruction {
    int addr;
public:
    StoreRegInstruction(int a, int srcReg)
        : IOInstruction(srcReg), addr(a) {}
    void execute(VirtualMachine& vm); // P3 fills
    string getName() { return "STORE"; }
};

// STORE [R2], R1  — address-in-register + source register
class StoreMemInstruction : public IOInstruction {
    int addrReg;
public:
    StoreMemInstruction(int aReg, int srcReg)
        : IOInstruction(srcReg), addrReg(aReg) {}
    void execute(VirtualMachine& vm); // P3 fills
    string getName() { return "STORE"; }
};

// ============================================================
// MoveInstruction  — CHIAM JUIN HOONG
// Three variants: reg-reg, immediate, register-indirect
// ============================================================
class MoveInstruction : public Instruction {
protected:
    int rDest;
public:
    MoveInstruction(int d) : rDest(d) {}
    string getName() { return "MOV"; }
};

class MovRegRegInstruction : public MoveInstruction {
    int rSrc;
public:
    MovRegRegInstruction(int d, int s) : MoveInstruction(d), rSrc(s) {}
    void   execute(VirtualMachine& vm);
    string getName() { return "MOV"; }
};

class MovImmInstruction : public MoveInstruction {
    int immediateValue;
public:
    MovImmInstruction(int d, int v)
        : MoveInstruction(d), immediateValue(v) {}
    void   execute(VirtualMachine& vm);
    string getName() { return "MOV"; }
};

// MOV R0, [R1] — reads memory at address stored in R1
class MovIndirectInstruction : public MoveInstruction {
    int rSrc;
public:
    MovIndirectInstruction(int d, int s)
        : MoveInstruction(d), rSrc(s) {}
    void   execute(VirtualMachine& vm);
    string getName() { return "MOV"; }
};

// ============================================================
// ResetInstruction  — CHIAM JUIN HOONG
// Direct child of Instruction. Zeroes one flag.
// ============================================================
class ResetInstruction : public Instruction {
    int flagTarget; // 0=CF, 1=ZF, 2=OF, 3=UF  (see FlagCode enum)
public:
    ResetInstruction(int f) : flagTarget(f) {}
    void   execute(VirtualMachine& vm);
    string getName() { return "RESET"; }
};

// ============================================================
// StackInstruction  — Author: alex, helyz
// ============================================================
class StackInstruction : public Instruction {
protected:
    int registerNum;
public:
    StackInstruction(int r) : registerNum(r) {}
    string getName() { return "STACK"; }
};

class PushInstruction : public StackInstruction {
public:
    PushInstruction(int r) : StackInstruction(r) {}
    void execute(VirtualMachine& vm); // P2 fills
    string getName() { return "PUSH"; }
};

class PopInstruction : public StackInstruction {
public:
    PopInstruction(int r) : StackInstruction(r) {}
    void execute(VirtualMachine& vm); // P2 fills
    string getName() { return "POP"; }
};

// ============================================================
// Runner  — CHIAM JUIN HOONG
// Phase 1: loadProgram  — reads .asm into rawLines
// Phase 2: parseProgram — builds Instruction* queue
// Phase 3: run          — executes queue, dumps state
// ============================================================
class Runner {
    VirtualMachine            vm;
    CustomVector<string>      rawLines;
    CustomQueue<Instruction*> program;

    // ── Operand helpers ─────────────────────────────────────
    // "R3"    → 3
    int extractReg(const string& s) {
        return s[1] - '0';
    }

    // "[R3]"  → 3  (register index from indirect token)
    int extractIndirectReg(const string& s) {
        return s[2] - '0';
    }

    // "[20]"  → 20 (direct memory address)
    int extractAddr(const string& s) {
        int val = 0;
        for (int i = 1; s[i] != ']'; i++)
            val = val * 10 + (s[i] - '0');
        return val;
    }

    // "42" / "-5"  → integer
    int extractNum(const string& s) {
        int val = 0, sign = 1, i = 0;
        if (!s.empty() && s[0] == '-') { sign = -1; i = 1; }
        for (; i < (int)s.size(); i++) val = val * 10 + (s[i] - '0');
        return sign * val;
    }

    // ── Parse-group helpers ──────────────────────────────────

    Instruction* parseMoveGroup(const string& op,
                                const string& op1,
                                const string& op2) {
        if (op == "RESET") {
            int f = FLAG_CF;
            if      (op1 == "ZF") f = FLAG_ZF;
            else if (op1 == "OF") f = FLAG_OF;
            else if (op1 == "UF") f = FLAG_UF;
            return new ResetInstruction(f);
        }
        int dest = extractReg(op1);
        if (op2[0] == '[')
            return new MovIndirectInstruction(dest, extractIndirectReg(op2));
        if (op2[0] == 'R')
            return new MovRegRegInstruction(dest, extractReg(op2));
        return new MovImmInstruction(dest, extractNum(op2));
    }

    Instruction* parseArith(const string& op,
                             const string& op1,
                             const string& op2) {
        int dest = extractReg(op1);
        if (op == "INC") return new IncInstruction(dest);
        if (op == "DEC") return new DecInstruction(dest);
        bool numMode = (op2[0] != 'R');
        int src = numMode ? extractNum(op2) : extractReg(op2);
        if (op == "ADD") return new AddInstruction(dest, src, numMode);
        if (op == "SUB") return new SubInstruction(dest, src, numMode);
        if (op == "MUL") return new MulInstruction(dest, src, numMode);
        return new DivInstruction(dest, src, numMode);
    }

    Instruction* parseShift(const string& op,
                             const string& op1,
                             const string& op2) {
        int reg = extractReg(op1);
        int amt = extractNum(op2);
        if (op == "SHL") return new ShlInstruction(reg, amt);
        if (op == "SHR") return new ShrInstruction(reg, amt);
        if (op == "ROL") return new RolInstruction(reg, amt);
        return new RorInstruction(reg, amt);
    }

    Instruction* parseIO(const string& op,
                          const string& op1,
                          const string& op2) {
        if (op == "INPUT")   return new InputInstruction(extractReg(op1));
        if (op == "DISPLAY") return new DisplayInstruction(extractReg(op1));
        if (op == "LOAD") {
            int dest = extractReg(op1);
            if (op2[1] == 'R')
                return new LoadMemInstruction(dest, extractIndirectReg(op2));
            return new LoadRegInstruction(dest, extractAddr(op2));
        }
        // STORE
        if (op1[0] == '[')
            return new StoreMemInstruction(extractIndirectReg(op1),
                                           extractReg(op2));
        return new StoreRegInstruction(extractNum(op1), extractReg(op2));
    }

    Instruction* parseStack(const string& op, const string& op1) {
        int reg = extractReg(op1);
        if (op == "PUSH") return new PushInstruction(reg);
        return new PopInstruction(reg);
    }

public:
    // ── Phase 1: Read .asm file → rawLines ──────────────────
    // Commas replaced with spaces so stringstream handles all
    // spacing variants cleanly (e.g. "R0 , R1", "R0,R1").
    void loadProgram(const string& filename) {
        ifstream input(filename.c_str());
        if (input.fail()) {
            cerr << "Error: cannot open file: " << filename << endl;
            exit(1);
        }
        string line;
        while (getline(input, line)) {
            for (int i = 0; i < (int)line.size(); i++)
                if (line[i] == ',') line[i] = ' ';
            stringstream check(line);
            string word;
            if (!(check >> word)) continue; // skip blank lines
            rawLines.push_back(line);
        }
        input.close();
    }

    // ── Phase 2: Parse rawLines → Instruction* queue ────────
    // ss >> op1 >> op2 silently leaves them empty for
    // 0-or-1 operand instructions — safe because parseArith
    // handles INC/DEC before touching op2.
    void parseProgram() {
        for (int i = 0; i < rawLines.getSize(); i++) {
            stringstream ss(rawLines.get(i));
            string opcode, op1, op2;
            ss >> opcode >> op1 >> op2;
            Instruction* instr = nullptr;

            if (opcode == "MOV" || opcode == "RESET")
                instr = parseMoveGroup(opcode, op1, op2);
            else if (opcode=="ADD" || opcode=="SUB" || opcode=="MUL"
                  || opcode=="DIV" || opcode=="INC" || opcode=="DEC")
                instr = parseArith(opcode, op1, op2);
            else if (opcode=="SHL" || opcode=="SHR"
                  || opcode=="ROL" || opcode=="ROR")
                instr = parseShift(opcode, op1, op2);
            else if (opcode=="LOAD"  || opcode=="STORE"
                  || opcode=="INPUT" || opcode=="DISPLAY")
                instr = parseIO(opcode, op1, op2);
            else if (opcode == "PUSH" || opcode == "POP")
                instr = parseStack(opcode, op1);
            else {
                cerr << "Unknown instruction: " << opcode << endl;
                exit(1);
            }
            program.enqueue(instr);
        }
    }

    // ── Phase 3: Execute queue, dump state ──────────────────
    void run() {
        while (!program.isEmpty()) {
            Instruction* inst = program.dequeue();
            inst->execute(vm);
            vm.getCPU().incrementPC();
        }
        vm.printState();
    }

    ~Runner() {
        // Queue fully consumed by run() — nothing left to delete
    }
};

// ============================================================
// P1 execute() implementations — MOV x3, RESET
// ============================================================

void MovRegRegInstruction::execute(VirtualMachine& vm) {
    int val = vm.getRegister(rSrc).getValue();
    vm.getRegister(rDest).setValue(val);
    vm.getFlags().setFlags(val, 0, 0);
}

void MovImmInstruction::execute(VirtualMachine& vm) {
    vm.getRegister(rDest).setValue(immediateValue);
    vm.getFlags().setFlags(immediateValue, 0, 0);
}

void MovIndirectInstruction::execute(VirtualMachine& vm) {
    int addr = vm.getRegister(rSrc).getValue();
    int val  = vm.getMemory().read(addr);
    vm.getRegister(rDest).setValue(val);
    vm.getFlags().setFlags(val, 0, 0);
}

void ResetInstruction::execute(VirtualMachine& vm) {
    vm.getFlags().resetFlag(static_cast<FlagCode>(flagTarget));
}

// ============================================================
// Stub execute() bodies — other teams fill these in
// ============================================================

// P4 fills:
void AddInstruction::execute(VirtualMachine& vm) { /* P4 */ }
void SubInstruction::execute(VirtualMachine& vm) { /* P4 */ }
void MulInstruction::execute(VirtualMachine& vm) { /* P4 */ }
void DivInstruction::execute(VirtualMachine& vm) { /* P4 */ }
void IncInstruction::execute(VirtualMachine& vm) { /* P4 */ }
void DecInstruction::execute(VirtualMachine& vm) { /* P4 */ }

// P2 fills:
void ShlInstruction::execute(VirtualMachine& vm) {
    int original = vm.getRegister(registerNum).getValue();
    int shifted  = original << amount;
    vm.getRegister(registerNum).setValue(shifted);
    vm.getFlags().setFlags(shifted, original, 0);
}
void ShrInstruction::execute(VirtualMachine& vm) {
    int original = vm.getRegister(registerNum).getValue();
    uint8_t asUnsigned = static_cast<uint8_t>(original);
    int shifted        = static_cast<int>(asUnsigned >> amount);
    vm.getRegister(registerNum).setValue(shifted);
    vm.getFlags().setFlags(shifted, original, 0);
}
void RolInstruction::execute(VirtualMachine& vm) {
    int     original = vm.getRegister(registerNum).getValue();
    uint8_t uval     = static_cast<uint8_t>(original);
    int     n        = amount % 8;
    uint8_t rotated  = (n == 0)
        ? uval
        : static_cast<uint8_t>((uval << n) | (uval >> (8 - n)));
    int result = static_cast<int>(static_cast<int8_t>(rotated));
    vm.getRegister(registerNum).setValue(result);
    vm.getFlags().setFlags(result, original, 0);
}
void RorInstruction::execute(VirtualMachine& vm) {
    int     original = vm.getRegister(registerNum).getValue();
    uint8_t uval     = static_cast<uint8_t>(original);
    int     n        = amount % 8;
    uint8_t rotated  = (n == 0)
        ? uval
        : static_cast<uint8_t>((uval >> n) | (uval << (8 - n)));
    int result = static_cast<int>(static_cast<int8_t>(rotated));
    vm.getRegister(registerNum).setValue(result);
    vm.getFlags().setFlags(result, original, 0);
}
void PushInstruction::execute(VirtualMachine& vm) {
    int valueToSave = vm.getRegister(registerNum).getValue();
    vm.getCPU().push(valueToSave);
}
void PopInstruction::execute(VirtualMachine& vm) {
    int poppedValue = vm.getCPU().pop();
    vm.getRegister(registerNum).setValue(poppedValue);
    vm.getFlags().setFlags(poppedValue, 0, 0);
}
void FlagRegister::setFlags(int result, int op1, int op2) {
    int8_t narrowed = static_cast<int8_t>(result);
    ZF = (narrowed == 0);
    OF = (result > 127);
    UF = (result < -128);
    unsigned int unsignedSum =
        static_cast<unsigned int>(static_cast<uint8_t>(op1)) +
        static_cast<unsigned int>(static_cast<uint8_t>(op2));
    CF = (unsignedSum > 0xFF);
}

// P3 fills:
// Prompts user, validates range  -128..127, set flags.
void InputInstruction::execute(VirtualMachine& vm)   { 
    int val;
    cout << "?" << endl;
    cin >> val;

    // Clamp & flag
    if (val > 127) {
        vm.getFlags().setFlags(val, 0, 0); // trigger OF
        val = 127;
    } else if (val < -128 ) {
        vm.getFlags().setFlags(val, 0, 0); // trigger UF
        val = -128;
    } else {
        vm.getFlags().setFlags(val, 0, 0); // handles ZF
    }
    vm.getRegister(registerNum).setValue(val);
}

// Display <Rn>
// Prints the signed decimal value in the register.
void DisplayInstruction::execute(VirtualMachine& vm) {
    cout << vm.getRegister(registerNum).getValue() << endl;
}

// LOAD R1, [20] - direct address
void LoadRegInstruction::execute(VirtualMachine& vm) {
    if (addr < 0 || addr > 63) {
        cerr << "LOAD: address " << addr << " out of range" << endl;
        exit(1);
    }
    int val = vm.getMemory().read(addr);
    vm.getRegister(registerNum).setValue(val);
    vm.getFlags().setFlags(val, 0, 0);
}

// LOAD R1, [R2] - register-indirect address
void LoadMemInstruction::execute(VirtualMachine& vm) {
    int addr = vm.getRegister(addrReg).getValue();
    if (addr < 0 || addr > 63) {
        cerr << "LOAD: address " << addr << " out of range" << endl;
        exit(1);
    }
    int val = vm.getMemory().read(addr);
    vm.getRegister(registerNum).setValue(val);
    vm.getFlags().setFlags(val, 0, 0);
}

// STORE 20, R3 - direct address, source register
void StoreRegInstruction::execute(VirtualMachine& vm){
    if (addr < 0 || addr > 63) {
        cerr << "STORE: address " << addr << " out of range" << endl;
        exit(1);
    }
    int val = vm.getRegister(registerNum).getValue();
    vm.getMemory().write(addr, val);
    // STORE does not update flags 
}

// STORE [R2], R1 - address-in-register, source register
void StoreMemInstruction::execute(VirtualMachine& vm){
    int addr = vm.getRegister(addrReg).getValue();
    if (addr < 0 || addr > 63) {
        cerr << "STORE: address " << addr << " out of range" << endl;
        exit(1);
    }
    int val = vm.getRegister(registerNum).getValue();
    vm.getMemory().write(addr, val);
    // STORE does not update flags
}

// P1 fills last (after all print() methods are done):
void VirtualMachine::printState() const { /* P1 fills last */ }

// ============================================================
// main  — CHIAM JUIN HOONG
// ============================================================
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <program.asm>" << endl;
        return 1;
    }
    Runner runner;
    runner.loadProgram(string(argv[1]));
    runner.parseProgram();
    runner.run();
    return 0;
}
