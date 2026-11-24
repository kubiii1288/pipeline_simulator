//
// Created by kubiii on 11/23/26.
//
#include "Instruction.h"

namespace pipeline_sim
{
    bool uses_rs1(Operation op) noexcept
    {
        switch (op) {
            case Operation::Add:
            case Operation::Sub:
            case Operation::Addi:
            case Operation::Lw:
            case Operation::Sw:
            case Operation::Beq:
            return true;

            case Operation::Invalid:
                return false;
        }
        return false;
    }

    bool uses_rs2(Operation op) noexcept
    {
        switch (op) {
            case Operation::Add:
            case Operation::Sub:
            case Operation::Sw:
            case Operation::Beq:
                return true;

            case Operation::Addi:
            case Operation::Lw:
            case Operation::Invalid:
                return false;
        }
        return false;
    }

    bool writes_register(Operation op) noexcept
    {
        switch (op) {
            case Operation::Add:
            case Operation::Sub:
            case Operation::Addi:
            case Operation::Lw:
                return true;

            case Operation::Sw:
            case Operation::Beq:
            case Operation::Invalid:
                return false;
        }
        return false;
    }

    bool is_load(Operation op) noexcept
    {
        switch (op) {
            case Operation::Lw:
                return true;
            case Operation::Add:
            case Operation::Sub:
            case Operation::Addi:
            case Operation::Sw:
            case Operation::Beq:
            case Operation::Invalid:
                return false;
        }
        return false;
    }
}
