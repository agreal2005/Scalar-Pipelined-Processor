# Scalar-Pipelined-Processor
Objective: Simulation of a scalar pipelined processor in C++.
Description: 
This project consists of a cpp file written to simulate the working of a Five-Stage Scalar Processor. In the `main.cpp` file, there are paths specified to the Instruction and Data Caches. The I-Cache as well as the D-Cache comprise of 256B each. They are direct mapped, and have a read and write port each. There is also a register file which contains 16 8-bit registers.
There are five main stages:
### 1. IF - Instruction Fetch Stage:
The instruction corresponding to the Program Counter (PC) value is fetched.
### 2. ID - Instruction Decode:
Instruction fetched in the previous cycle is decoded. Note that at the end of the ID stage, register file is read.
### 3. EX - Execute Stage:
Values fetched from the register file (or) immediate values generated are processed in the ALU, which performs basic arithmetic and logical operations.
### 4. MEM - Memory I/O Stage:
Memory Loads and Memory Stores are supported. In case of a Memory Store Operation, value stored in the register read in the corresponding ID stage is written to memory at a specified address. In case of a Memory Load, data is fetched from memory and is written to the required register.
### 5. WB - Write Back Stage:
The final stage - it involves writing back the data/output from the previous stages (the EX & MEM stages) to the required register.

---
## Handling Hazards
### Handling RAW Hazards:
RAW is a data hazard, which indicates the reading of a register just after writing to it. This problem occurs when, before the write back stage is complete for a register, data is read from it in another stage that should occur after writing to it. If this were to occur, then the data being read will not be the updated one.

To prevent this, the instruction fetch is stalled until the required write back has occured.

### Handling Control Hazards:
Control Hazards denote a change in the PC value - a change in the control flow of a program. This could occur in two cases: In case of a Jump Instruction, which arbitrarily jumps to an instruction specified by a relative offset in the PC value. And the other case is a Branching Instruction - where the condition specified has to be evaluated. If true or false then the PC value will change accordingly, based on the type of Branch Instruction.

To handle Control Hazards, we let the instructions flow upto the point where this instruction reaches the EX stage. In the event of jumping to another PC value, the pipeline is flushed and the instructions are fetched again, from the new PC value.

---
## Instruction Set
We consider a modified Reduced Instruction Set Computer (RISC) processor. This architecture has the following
instruction set, comprising 16 instructions.

### • Arithmetic Instructions

– ADD rd rs1 rs2 Addition : rd ← rs1 + rs2

– SUB rd rs1 rs2 Subtraction : rd ← rs1 - rs2

– MUL rd rs1 rs2 Multiplication: rd ← rs1 * rs2

– INC r Increment : r ← r + 1

In the increment instruction, the last 8 bits are discarded.

### • Logical Instructions

– AND rd rs1 rs2 Bit-wise AND : rd ← rs1 & rs2

– OR rd rs1 rs2 Bit-wise OR : rd ← rs1 | rs2

– XOR rd rs1 rs2 Bit-wise XOR : rd ← rs1 ⊕ rs2

– NOT rd rs Bit-wise NOT : rd ← ∼ rs

### • Shift Instructions

– SLLI rd rs1 imm(4) Shift Logical Left Immediate: rd ← rs1 << imm(4)

– SRLI rd rs1 imm(4) Shift Logical Right Immediate: rd ← rs1 >> imm(4)

### • Memory Instructions

– LI rd imm(8) Load Immediate : rd ← imm(8)

– LD rd rs1 imm(4) Load Memory : rd ← [rs1 + imm(4)]

– ST rd rs1 imm(4) Store Memory : [rs1 + imm(4)] ← rd

Note: imm(4) is a 4-bit immediate value and imm(8) is a 8-bit immediate value, both signed

### • Control Instructions

– JMP L1 Unconditional jump by L1 instructions, last 4 bits are discarded

– BEQZ rs L1 Jump by L1 instructions if rs is zero (0).

L1 is offset from the current program counter (PC). This is called PC-relative addressing. L1 is an 8-bit
number represented in 2’s complement format.

### • Halt instruction
– HLT Program terminates, and the last 12 bits are discarded.

---

## Commands required for Execution:
In the terminal, run the following command:
`g++ -o main main.cpp`

After compilation, run `./main` to run the program.
The output generated will show various statistics of the program, post execution. These include Cycles per Instruction, number of stalls, and the types of instructions encountered.

---
