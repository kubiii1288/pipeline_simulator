//
// Created by kubiii on 10/18/26.
//

#include "RegisterFile.h"
#include <stdexcept>

namespace pipeline_sim {

    // read value of register index-th
    std::uint32_t RegisterFile::read(std::size_t index) const {
        if (index >= kRegisterCount)
            throw std::out_of_range("register index must be in range [0,31]");
        if (index == 0)
            return 0;
        return registers_[index];
    }

    // write value to index-th register
    void RegisterFile::write(std::size_t index, std::uint32_t value) {
        if (index >=kRegisterCount)
            throw std::out_of_range("register index must be in range [0,31]");
        if (index == 0)
            return;
        registers_[index] = value;
    }
}
