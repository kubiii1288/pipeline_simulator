//
// Created by kubiii on 10/18/26.
//

#ifndef PIPELINE_SIMULATOR_MEMORY_H
#define PIPELINE_SIMULATOR_MEMORY_H
#include <cstddef>
#include <cstdint>
#include <vector>

namespace pipeline_sim {
    class Memory {
    public:
        // explict force using the constructor to create an object
        explicit Memory(std::size_t size);
        std::uint8_t load_byte(std::uint32_t address) const;
        void store_byte(std::uint32_t address, std::uint8_t value);
        std::uint32_t load_word(std::uint32_t address) const;
        void store_word(std::uint32_t address, std::uint32_t value);
        std::size_t size() const noexcept;
    private:
        std::vector<std::uint8_t> bytes_;
    };
} // pipeline_sim

#endif //PIPELINE_SIMULATOR_MEMORY_H
