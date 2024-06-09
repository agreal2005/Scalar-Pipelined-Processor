#include <iostream>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <vector>
#include <queue>
using namespace std;
void us_to_s(int &k) // dealing with signed/unsignedness
{
    k += 128;
    k%=256;
    k-=128;
}
int hd(char c) // hex to decimal
{
    switch (c)
    {
        case '0':
        return 0;
        case '1':
        return 1;
        case '2':
        return 2;
        case '3':
        return 3;
        case '4':
        return 4;
        case '5':
        return 5;
        case '6':
        return 6;
        case '7':
        return 7;
        case '8':
        return 8;
        case '9':
        return 9;
        case 'a': case 'A':
        return 10;
        case 'b': case 'B':
        return 11;
        case 'c': case 'C':
        return 12;
        case 'd': case 'D':
        return 13;
        case 'e': case 'E':
        return 14;
        case 'f': case 'F':
        return 15;
    }
}
int main()
{
    ifstream icache, dcache, rf;
    icache.open("./input/ICache.txt");
    dcache.open("./input/DCache.txt");
    rf.open("./input/RF.txt");
    vector<int> ins, data, reg; // Contents of the files will be read here
    string str = ""; // Will take in input from each file
    while (icache >> str) {
        int num = (hd(str[0])*16) + hd(str[1]);
        ins.push_back(num);
    }
    while (dcache >> str) {
        int num = (hd(str[0])*16) + hd(str[1]);
        data.push_back(num);
    }
    while (rf >> str) {
        int num = (hd(str[0])*16) + hd(str[1]);
        reg.push_back(num);
    }
    reg[0] = 0; // Enforcing that R0 always has 0
    int PC = 0; // Program Counter
    vector<int> reg_taken(16); // If a reg is being updated, then in case of RAW, we need to stall it
    /*
    Buffers: IF x1 ID x2 EX x3 MEM x4 WB
    No Operand forwarding
    If ID decodes dependencies, then stall: let the ins remain in x1 itself. Don't send to x2
    If ID decodes BEQ run till BEQ completes EX. After that flush the pipeline and load the req ins from the new PC value
    If ID decodes HLT, then no instructions are fetched.
    */
    struct _ins_count
    {
        int ar_ins = 0, log_ins = 0, shift_ins = 0, mem_ins = 0, li_ins = 0, ctrl_ins = 0, hlt_ins = 0;
        void print_output(int total_cycles, int stalled_cycles, int data_stalls = 0)
        {
            // Finalizing the output
            // cout << total_cycles << endl;
            int total_ins = ar_ins + log_ins + shift_ins + mem_ins + li_ins + ctrl_ins + hlt_ins;
            cout << setw(45) << left << "Total number of instructions executed ";
            cout << ": " << total_ins << endl;
            cout << "Number of instructions in each class"<<endl;
            cout << setw(45) << "Arithmetic instructions";
            cout << ": " << ar_ins << endl;
            cout << setw(45) << "Logical instructions";
            cout << ": " << log_ins << endl;
            cout << setw(45) << "Shift instructions";
            cout << ": " << shift_ins << endl;
            cout << setw(45) << "Memory instructions";
            cout << ": " << mem_ins << endl;
            cout << setw(45) << "Load immediate instructions";
            cout << ": " << li_ins << endl;
            cout << setw(45) << "Control instructions";
            cout << ": " << ctrl_ins << endl;
            cout << setw(45) << "Halt instructions";
            cout << ": " << hlt_ins << endl;
            cout << setw(45) << "Cycles Per Instruction";
            cout << ": " << (float)total_cycles/total_ins << endl;
            cout << setw(45) << "Total number of stalls";
            cout << ": " << stalled_cycles << endl;
            cout << setw(45) << "Data stalls (RAW)";
            cout << ": " << data_stalls << endl;
            cout << setw(45) << "Control stalls";
            cout << ": " << stalled_cycles - data_stalls << endl;
        }
    } ics;
    struct _flags
    {
        bool halt = false;
        bool stall = false;
        bool ctrl = false;
    } flags;
    struct x1bf
    {
        long long ins;
        void fetch(int &PC, _flags &f, vector<int> &instr, int &stalled_cycles, int &data_stalls)
        {
            if (f.halt)
            {
                return;
            }
            else if (!f.stall)
            {
                ins = (instr[PC] << 8) + instr[PC+1];
                PC += 2;
            }
            else
            {
                if (!f.ctrl) {
                    data_stalls++;
                }
                stalled_cycles++;
            }
        }
        void change_PC(int pc, int &PC) // To be used when jmp or beq reaches execute
        {
            PC += pc; 
        }
    } buff1;
    struct x2bf
    {
        bool empty = true, set_rs1 = false, set_rs2 = false, set_imm = false, set_rd = false, halt = false;
        bool jmp = false, beq = false, load = false, store = false;
        int opcode;
        int rd, rs1, rs2, imm;
        void decode(int ins, _flags &f, vector<int> &reg_taken)
        {
            opcode = (ins >> 12);
            switch (opcode)
            {
                case 0: case 1: case 2:
                // Arithmetic Instructions
                rd = (ins>>8)&15;
                rs1 = (ins>>4)&15;
                rs2 = ins&15;
                set_rs1 = set_rs2 = true;
                set_rd = true;
                set_imm = false;
                break;
                case 3:
                // INC r instruction
                rd = (ins >> 8)&15;
                rs1 = rd;
                imm = 1;
                set_rs1 = true;
                set_rs2 = false;
                set_rd = true;
                set_imm = true;
                break;
                case 4: case 5: case 6:
                // Logical Instructions
                rd = (ins>>8)&15;
                rs1 = (ins>>4)&15;
                rs2 = ins&15;
                set_rs1 = set_rs2 = true;
                set_rd = true;
                set_imm = false;
                break;
                case 7:
                // NOT instruction
                rd = (ins>>8)&15;
                rs1 = (ins>>4)&15;
                set_rs1 = true;
                set_rs2 = false;
                set_rd = true;
                set_imm = false;
                break;
                case 8: case 9:
                // Shift Instructions
                rd = (ins>>8)&15;
                rs1 = (ins>>4)&15;
                imm = ins&15;
                set_rs1 = true;
                set_rs2 = false;
                set_rd = true;
                set_imm = true;
                if (imm >= 8) imm -= 16;
                break;
                case 10:
                // Load Immediate instruction
                rd = (ins >> 8)&15;
                imm = ins&255;
                set_rs1 = set_rs2 = false;
                set_rd = true;
                set_imm = true;
                if (imm > 255) imm -= 256;
                break;
                case 11: case 12:
                // Load/Store Mem Instructions
                rd = (ins >> 8)&15;
                rs1 = (ins >> 4)&15;
                imm = ins&15;
                set_rs1 = true;
                set_rs2 = false;
                set_rd = true;
                set_imm = true;
                if (imm > 15) imm -= 16;
                if (opcode == 11) load = true;
                else store = true;
                break;
                case 13:
                // JMP instruction
                imm = (ins>>4)&255;
                set_imm = true;
                set_rs1 = set_rs2 = set_rd = false;
                jmp = true;
                if (imm > 127) imm -= 256;
                break;
                case 14:
                // BEQ instruction
                rs1 = (ins>>8)&15;
                imm = ins&255;
                set_rs1 = true;
                set_rs2 = set_rd = false;
                set_imm = true;
                beq = true;
                if (imm > 127) imm -= 256;
                break;
                case 15:
                // HLT instruction
                f.halt = true;
                halt = true;
                set_rs1 = set_rs2 = set_rd = set_imm = false;
                break;
            }
            if (opcode != 11 && opcode != 12)
            {
                load = store = false;
            }
            if (opcode != 13) jmp = false;
            if (opcode != 14) beq = false;
        }
        void check_raw(vector<int>&reg_taken, _flags &f)
        {
            if (halt) return;
            // If rs1 or rs2 is in use cause a stall. I.e. stall = true;
            if (set_rs1 == true && reg_taken[rs1]>0) {
                f.stall = true;
                empty = true;
                set_rd = false;
                load = store = false;
                beq = false;
                jmp = false;
            }
            else if (set_rs2 == true && reg_taken[rs2]>0) {
                f.stall = true;
                empty = true;
                set_rd = false;
                load = store = false;
                beq = false;
                jmp = false;
            }
            else if (store && reg_taken[rd]>0)
            {
                f.stall = true;
                empty = true;
                store = false;
                set_rd = false;
                beq = false;
                jmp = false;
            }
            else if (set_rd && !store)
            {
                reg_taken[rd]++;
            }
        }
        void check_jmp_beq(int &PC, x1bf &buff1, _flags &f)
        {
            if (halt) return;
            if ((jmp || beq)) {
                f.stall = true;
                f.ctrl = true;
                jmp = beq = false;
            }
        }
    } buff2;
    struct x3bf
    {
        bool empty = true, set_rs1 = false, set_rs2 = false, set_imm = false, set_rd = false, halt = false;
        bool jmp = false, beq = false, load = false, store = false;
        int opcode;
        int rd, rs1, rs2, imm;
        int alu_output;
        void getdata(struct x2bf s)
        {
            halt = s.halt;
            empty = s.empty;
            set_rs1 = s.set_rs1;
            set_rs2 = s.set_rs2;
            set_imm = s.set_imm;
            set_rd = s.set_rd;
            jmp = s.jmp;
            beq = s.beq;
            load = s.load;
            store = s.store;
            rd = s.rd;
            rs1 = s.rs1;
            rs2 = s.rs2;
            imm = s.imm;
            opcode = s.opcode;
        }
        void execute(vector<int>& reg, x1bf &ifbf, _ins_count &ics, _flags &f, int &PC)
        {
            if (empty) return;
            switch (opcode)
            {
                case 0:
                alu_output = reg[rs1] + reg[rs2];
                us_to_s(alu_output);
                ics.ar_ins++;
                break;
                case 1:
                alu_output = reg[rs1] - reg[rs2];
                us_to_s(alu_output);
                ics.ar_ins++;
                break;
                case 2:
                alu_output = reg[rs1] * reg[rs2];
                us_to_s(alu_output);
                ics.ar_ins++;
                break;
                case 3:
                alu_output = reg[rs1] + imm;
                us_to_s(alu_output);
                ics.ar_ins++;
                break;
                case 4:
                alu_output = reg[rs1] & reg[rs2];
                us_to_s(alu_output);
                ics.log_ins++;
                break;
                case 5:
                alu_output = reg[rs1] | reg[rs2];
                ics.log_ins++;
                break;
                case 6:
                alu_output = reg[rs1] ^ reg[rs2];
                ics.log_ins++;
                break;
                case 7:
                alu_output = !reg[rs1];
                ics.log_ins++;
                break;
                case 8:
                alu_output = (reg[rs1] << imm);
                alu_output %= 256;
                ics.shift_ins++;
                break;
                case 9:
                alu_output = (reg[rs1] >> imm);
                ics.shift_ins++;
                break;
                case 10:
                alu_output = imm;
                ics.li_ins++;
                break;
                case 11: case 12:
                alu_output = reg[rs1] + imm;
                ics.mem_ins++;
                break;
                case 13:
                alu_output = 2*imm;
                ifbf.change_PC(alu_output, PC);
                ics.ctrl_ins++;
                break;
                case 14:
                if (reg[rs1] == 0) {
                    alu_output=2*imm;
                    ifbf.change_PC(alu_output, PC);
                }
                ics.ctrl_ins++;
                break;
                case 15:
                ics.hlt_ins++;
                break;
            }
        }
    } buff3;
    struct x4bf
    {
        bool empty = true, set_rd = false;
        bool load = false, store = false, halt = false;
        int rd, alu_output, lmd;
        void transfer(x3bf &s, _flags &flags, int&PC)
        {
            set_rd = s.set_rd;
            empty = s.empty;
            load = s.load;
            store = s.store;
            halt = s.halt;
            rd = s.rd;
            alu_output = s.alu_output;
        }
        void memio(vector<int> &data, vector<int> &reg)
        {
            if (load == true)
            {
                lmd = data[alu_output];
            }
            else if (store == true)
            {
                data[alu_output] = reg[rd];
            }
        }
    } buff4;

    int total_cycles = 0, stalled_cycles = 0, data_stalls = 0, s = 0, ds = 0;
    
    while (1)
    {
        total_cycles++;
        if (buff4.halt == true) break;
        // Write Back
        if (!buff4.empty) {
            if (buff4.load == true)
            {
                // cout << "WB for Load: " << buff4.rd << endl;
                reg[buff4.rd] = buff4.lmd;
                reg_taken[buff4.rd]--;
                if (reg_taken[buff4.rd] == 0) flags.stall = false;
            }
            else if (buff4.set_rd == true && !buff4.store)
            {
                // cout << "WB: " << buff4.rd << endl;
                reg[buff4.rd] = buff4.alu_output;
                reg_taken[buff4.rd]--;
                if (reg_taken[buff4.rd] == 0 && !flags.ctrl) {
                    flags.stall = false;
                }
            }
        }
        // Buff-4 zone [MEM - WB]
        if (!buff3.empty){
            buff4.transfer(buff3, flags, PC);
            buff4.memio(data, reg);
            if (buff3.opcode == 13 || buff3.opcode == 14) {
                flags.stall = false;
            }
        }
        else buff4.empty = true;
        // Buff-3 zone [EX - MEM]
        if (!buff2.empty) {
            buff3.getdata(buff2);
            if (flags.ctrl)
            {
                buff2.empty = true;
                buff2.beq = buff2.jmp = buff2.store = buff2.load = false;
            }
            buff3.execute(reg, buff1, ics, flags, PC);
            // cout << "EXEC-stage: " << buff3.opcode << endl;
        }
        else {
            buff3.empty = true;
            buff2.beq = buff2.jmp = buff2.store = buff2.load = false;
        }
        // Buff-2 zone [ID - EX]
        if (PC > 0 && !flags.stall && !flags.halt && !flags.ctrl) {
            buff2.decode(buff1.ins, flags, reg_taken);
            buff2.empty = false;
        }
        else {
            buff2.empty = true;
        }
        // Stalling zone
        buff2.check_raw(reg_taken, flags);
        buff2.check_jmp_beq(PC, buff1, flags);
        if (flags.ctrl && !flags.stall) flags.ctrl = false;
        // Buff-1 zone [IF - ID]
        buff1.fetch(PC, flags, ins, stalled_cycles, data_stalls);
        // Printing Zone
        // cout << "********** IN USE *************\n";
        // cout << "WB " << buff4.empty << " MEM " << buff3.empty << " EX " << buff2.empty << " ID " << "*" << " IF\n";
        // cout << "REGS:::::::";
        // for (auto rajatsir : reg) cout << setw(3) << rajatsir << ' ';
        // cout << endl;
        // cout << "TAKEN::::::";
        // for (auto val : reg_taken) cout << setw(3) << val << ' ';
        // cout << endl;
        // cout << "HALT: " << flags.halt << " STALL: " << flags.stall << " CTRL_STALL: " << flags.ctrl << endl;
        // cout << "*************************************\n";
        // cout << "Fetched: " << PC << ' ' << ((buff1.ins >> 12)&15) << ' ' << ((buff1.ins >> 8)&15) << ' ' << ((buff1.ins >> 4)&15) << ' ' << ((buff1.ins)&15) << endl;
        // cout << "*************************************\n";
        // if (!buff2.empty) cout << "ID: " << buff2.opcode;
        // if (!buff2.empty && buff2.set_rd) cout << " RD: "<< buff2.rd;
        // if (!buff2.empty && buff2.set_rs1) cout << " RS1: " << buff2.rs1;
        // if (!buff2.empty && buff2.set_rs2) cout << " RS2: " << buff2.rs2; 
        // if (!buff2.empty && buff2.set_imm) cout << " IMM: " << buff2.imm;
        // cout << endl;
        // getchar();
    }
    /*-------------------------*/
    // The Final Output
    ics.print_output(total_cycles, stalled_cycles, data_stalls);
}