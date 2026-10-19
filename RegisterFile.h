//
// Created by kubiii on 10/18/26.
//

#ifndef PIPELINE_SIMULATOR_REGISTERFILE_H
#define PIPELINE_SIMULATOR_REGISTERFILE_H
#include <array>
#include <cstddef>
#include <cstdint>

namespace pipeline_sim {
    class RegisterFile {
    public:
        std::uint32_t read(std::size_t index) const;
        void write(std::size_t index, std::uint32_t value);
    private:
        static constexpr std::size_t kRegisterCount = 32;
        std::array<std::uint32_t,kRegisterCount> registers_{};
    };
}
#endif //PIPELINE_SIMULATOR_REGISTERFILE_H
