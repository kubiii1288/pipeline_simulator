//
// Created by kubiii on 10/18/26.
//

#include "Memory.h"

#include <stdexcept>

namespace pipeline_sim {
    //Constructor of Memory

    Memory::Memory(std::size_t size) : bytes_(size, std::uint8_t{0}) {
    }

    std::uint8_t Memory::load_byte(std::uint32_t address) const {
        if (address >= this->size()) {
            throw std::out_of_range("memory address is out of range");
        }
        return bytes_[address];
    }

    void Memory::store_byte(std::uint32_t address, std::uint8_t value) {
        if (address >= this->size()) {
            throw std::out_of_range("memory address is out of range");
        }
        bytes_[address] = value;
    }

    std::uint32_t Memory::load_word(std::uint32_t address) const {
        if (address % 4) {
            throw std::invalid_argument("address it not divisible by 4");
        }
        if (address + 3 >= this->size()) {
            throw std::out_of_range("Memory is full: fewer than 4 bytes remain");
        }
        std::uint32_t rs{0};
        rs = static_cast<std::uint32_t>(bytes_[address])
        | (static_cast<std::uint32_t>(bytes_[address + 1]) << 8)
        | (static_cast<std::uint32_t>(bytes_[address + 2]) << 16 )
        | (static_cast<std::uint32_t>(bytes_[address + 3]) << 24);
        return rs;
    }

    void Memory::store_word(std::uint32_t address, std::uint32_t value) {
        if (address % 4) {
            throw std::invalid_argument("address it not divisible by 4");
        }
        if (address + 3 >= this->size()) {
            throw std::out_of_range("Memory is full: fewer than 4 bytes remain");
        }
        bytes_[address] = static_cast<std::uint8_t>(value & 0xFFu);
        bytes_[address+1] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
        bytes_[address+2] = static_cast<std::uint8_t>((value >> 16) & 0xFFu);
        bytes_[address+3] = static_cast<std::uint8_t>((value >> 24) & 0xFFu);
    }

    std::size_t Memory::size() const noexcept {
        return bytes_.size();
    }
} // pipeline_sim
