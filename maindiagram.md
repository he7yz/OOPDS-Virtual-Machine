classDiagram
    direction TB

    class Register {
        <<abstract>>
        # int value
        + getValue() int
        + setValue(int v) void
    }
    class GeneralRegister {
        - string name
        + GeneralRegister(string n)
        + getValue() int
        + setValue(int v) void
        + print() void
    }
    class FlagRegister {
        - bool CF
        - bool ZF
        - bool OF
        - bool UF
        + setFlags(int result, int op1, int op2) void
        + resetFlag(FlagCode f) void
        + getCF() bool
        + getZF() bool
        + getOF() bool
        + getUF() bool
        + print() void
    }

    class Memory {
        - int8_t mem[64]
        + read(int addr) int
        + write(int addr, int val) void
        + print() void
    }

    class CustomVector~T~ {
        - T* data
        - int sz
        - int cap
        + push_back(T item) void
        + get(int i) T
        + operator[](int i) T
        + getSize() int
        + isEmpty() bool
    }

    class CustomStack~T~ {
        - T data[8]
        - int top
        + push(T val) void
        + pop() T
        + peek() T
        + isEmpty() bool
    }

    class CustomQueue~T~ {
        - T* data
        - int head
        - int tail
        - int sz
        - int cap
        + enqueue(T item) void
        + dequeue() T
        + isEmpty() bool
    }

    class CPU {
        - int pc
        + getPC() int
        + incrementPC() void
        + push(int val) void
        + pop() int
        + peek() int
        + getMemory() Memory&
        + getFlags() FlagRegister&
        + print() void
    }

    class VirtualMachine {
        - CPU cpu
        - CustomVector~GeneralRegister~ registers
        + getCPU() CPU&
        + getRegister(int i) GeneralRegister&
        + getFlags() FlagRegister&
        + getMemory() Memory&
        + printState() void
    }

    class Instruction {
        <<abstract>>
        + execute(VirtualMachine& vm) void
        + getName() string
    }

    class ArithmeticInstruction {
        <<abstract>>
        # int destReg
        # int srcVal
        # bool isNum
    }
    class AddInstruction
    class SubInstruction
    class MulInstruction
    class DivInstruction
    class IncInstruction
    class DecInstruction

    class ShiftInstruction {
        <<abstract>>
        # int registerNum
        # int amount
    }
    class ShlInstruction
    class ShrInstruction
    class RolInstruction
    class RorInstruction

    class IOInstruction {
        <<abstract>>
        # int registerNum
    }
    class InputInstruction
    class DisplayInstruction
    class LoadRegInstruction {
        - int addr
    }
    class LoadMemInstruction {
        - int addrReg
    }
    class StoreRegInstruction {
        - int addr
    }
    class StoreMemInstruction {
        - int addrReg
    }

    class MoveInstruction {
        <<abstract>>
        # int rDest
    }
    class MovRegRegInstruction {
        - int rSrc
    }
    class MovImmInstruction {
        - int immediateValue
    }
    class MovIndirectInstruction {
        - int rSrc
    }

    class StackInstruction {
        <<abstract>>
        # int registerNum
    }
    class PushInstruction
    class PopInstruction

    class ResetInstruction {
        - int flagTarget
    }

    class Runner {
        - VirtualMachine vm
        - CustomVector~string~ rawLines
        - CustomQueue~Instruction*~ program
        + loadProgram(string filename) void
        + parseProgram() void
        + run() void
    }

    Register <|-- GeneralRegister
    Register <|-- FlagRegister

    CPU *-- Memory : composition
    CPU *-- FlagRegister : composition
    CPU *-- CustomStack~int~ : composition

    VirtualMachine *-- CPU : composition
    VirtualMachine *-- CustomVector~GeneralRegister~ : composition

    Instruction <|-- ArithmeticInstruction
    Instruction <|-- ShiftInstruction
    Instruction <|-- IOInstruction
    Instruction <|-- MoveInstruction
    Instruction <|-- StackInstruction
    Instruction <|-- ResetInstruction

    ArithmeticInstruction <|-- AddInstruction
    ArithmeticInstruction <|-- SubInstruction
    ArithmeticInstruction <|-- MulInstruction
    ArithmeticInstruction <|-- DivInstruction
    ArithmeticInstruction <|-- IncInstruction
    ArithmeticInstruction <|-- DecInstruction

    ShiftInstruction <|-- ShlInstruction
    ShiftInstruction <|-- ShrInstruction
    ShiftInstruction <|-- RolInstruction
    ShiftInstruction <|-- RorInstruction

    IOInstruction <|-- InputInstruction
    IOInstruction <|-- DisplayInstruction
    IOInstruction <|-- LoadRegInstruction
    IOInstruction <|-- LoadMemInstruction
    IOInstruction <|-- StoreRegInstruction
    IOInstruction <|-- StoreMemInstruction

    MoveInstruction <|-- MovRegRegInstruction
    MoveInstruction <|-- MovImmInstruction
    MoveInstruction <|-- MovIndirectInstruction

    StackInstruction <|-- PushInstruction
    StackInstruction <|-- PopInstruction

    Runner *-- VirtualMachine : composition
    Runner *-- CustomVector~string~ : composition
    Runner *-- CustomQueue~Instruction*~ : composition
    Runner o-- Instruction : uses (aggregation)