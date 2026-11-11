//
// Created by kubiii on 11/10/26.
//

#include "Decoder.h"
/*
 * R-type instruction format
 *
 * 31       25 24    20 19    15 14  12 11     7 6       0
 * +----------+--------+--------+------+---------+---------+
 * | funct7   |  rs2   |  rs1   |funct3|   rd    | opcode  |
 * +----------+--------+--------+------+---------+---------+
 */

/*
 * I-type instruction format
 *
 * 31                 20 19    15 14  12 11     7 6       0
 * +--------------------+--------+------+---------+---------+
 * | immediate[11:0]    |  rs1   |funct3|   rd    | opcode  |
 * +--------------------+--------+------+---------+---------+
 */

/*
 * S-type instruction format
 *
 * 31       25 24    20 19    15 14  12 11      7 6       0
 * +----------+--------+--------+------+----------+---------+
 * | imm[11:5]|  rs2   |  rs1   |funct3| imm[4:0] | opcode  |
 * +----------+--------+--------+------+----------+---------+
 */


/*
 * B-type instruction format
 *
 * 31      30      25 24    20 19    15 14  12 11       8 7      6       0
 * +---------+-------+--------+--------+------+-----------+--------+---------+
 * | imm[12] |imm[10:5]| rs2  |  rs1   |funct3| imm[4:1] |imm[11] | opcode |
 * +---------+-------+--------+--------+------+-----------+--------+---------+
 */
namespace pipeline_sim
{
    DecodedInstruction decode(std::uint32_t raw)
    {
        DecodedInstruction di;
        di.raw = raw;

        // bits 11-7
        di.rd = static_cast<std::uint8_t>((raw >> 7) & 0x1F);
        // bits 19-15
        di.rs1 = static_cast<std::uint8_t>((raw >> 15) & 0x1F);
        // bits 24-20
        di.rs2 = static_cast<std::uint8_t>((raw >> 20) & 0x1F);

        const std::uint32_t opcode = raw & 0x7Fu;
        // get 3 bits
        const std::uint32_t funct3 = (raw >> 12) & 0x07u;
        // get 7 bits
        const std::uint32_t funct7 = (raw >> 25) & 0x07Fu;

        if (opcode == 0x33 && funct3 == 0) {
            if (funct7 == 0x00) {
                di.operation = Operation::Add;
            } else if (funct7 == 0x20) {
                di.operation = Operation::Sub;
            }
        }

        // get 12 bits
        const std::uint32_t immediate_12 = (raw >> 20) & 0xFFFu;
        if (opcode == 0x13 && funct3 == 0) {
            di.operation = Operation::Addi;
            if (immediate_12 & 0x800u) {
                di.immediate = static_cast<std::int32_t>(immediate_12) - 4096;
            } else di.immediate = static_cast<std::int32_t>(immediate_12);
        }

        if (opcode == 0x03 && funct3 == 0x02) {
            di.operation = Operation::Lw;
            if (immediate_12 & 0x800u) {
                di.immediate = static_cast<std::int32_t>(immediate_12) - 4096;
            } else di.immediate = static_cast<std::int32_t>(immediate_12);
        }

        const std::uint32_t immediate_high = (raw >> 25) & 0x7Fu;
        const std::uint32_t immediate_low = (raw >> 7) & 0x1Fu;
        const std::uint32_t store_immediate_12 = (immediate_high << 5) | immediate_low;

        if (opcode == 0x23 && funct3 == 0x02) {
            di.operation = Operation::Sw;
            if (store_immediate_12 & 0x800u) {
                di.immediate = static_cast<std::int32_t>(store_immediate_12) - 4096;
            } else di.immediate = static_cast<std::int32_t>(store_immediate_12);
        }

        const std::uint32_t branch_immediate =
                (((raw >> 31) & 0x01u) << 12)
                | (((raw >> 7) & 0x01u) << 11)
                | (((raw >> 25) & 0x3Fu) << 5)
                | (((raw >> 8) & 0x0Fu) << 1);
        if (opcode == 0x63 && funct3 == 0x00) {
            di.operation = Operation::Beq;
            if (branch_immediate & 0x1000u) {
                di.immediate = static_cast<std::int32_t>(branch_immediate) - 8192;
            } else di.immediate = static_cast<std::int32_t>(branch_immediate);
        }
        return di;
    }
} // pipeline_sim
