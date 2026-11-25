//
// Created by kubiii on 11/24/26.
//

#ifndef PIPELINE_SIMULATOR_BRANCHPREDICTOR_H
#define PIPELINE_SIMULATOR_BRANCHPREDICTOR_H
#include <cstdint>
#include <array>
namespace pipeline_sim
{
    struct BranchPrediction
    {
       bool taken{false};
       std::uint32_t metadata{0};
    };

    class BranchPredictor
    {
    public:
        virtual ~BranchPredictor() = default;

        virtual BranchPrediction predict(std::uint32_t branch_pc) = 0;
        virtual void update(std::uint32_t branch_pc, BranchPrediction prediction, bool actual_taken) = 0;
    };
    class AlwaysNotTakenPredictor : public BranchPredictor
    {
    public:
        BranchPrediction predict(std::uint32_t branch_pc) override
        {
            return {};
        }
        void update(std::uint32_t branch_pc, BranchPrediction prediction, bool actual_taken) override { }
    };

    class AlwaysTakenPredictor : public BranchPredictor
    {
    public:
        BranchPrediction predict(std::uint32_t) override
        {
            return {true, 0u};
        }

        void update(std::uint32_t, pipeline_sim::BranchPrediction, bool) override
        {
        }
    };

    class BimodalPredictor : public BranchPredictor
    {
    public:
        BimodalPredictor()
        {
            counters_.fill(1);
        }
        BranchPrediction predict(std::uint32_t branch_pc) override
        {
            const std::uint32_t index = static_cast<std::uint32_t>((branch_pc >> 2) % counters_.size());
            return {counters_[index] >= 2, index};
        }
        void update(std::uint32_t branch_pc, BranchPrediction prediction, bool actual_taken) override
        {
            std::uint8_t& counter = counters_[prediction.metadata];
            if (actual_taken) {
                if (counter < 3)
                    ++counter;
            } else if (counter > 0) {
                --counter;
            }
        }
    private:
        std::array<std::uint8_t, 64> counters_{};
    };

    class GsharePredictor : public  BranchPredictor
    {
    public:
        GsharePredictor()
        {
            counters_.fill(1);
        }
        BranchPrediction predict(std::uint32_t branch_pc) override
        {
            const std::uint32_t index = ((branch_pc >> 2) ^ history_) & 0x3fu;
            return {counters_[index] >=2, index};
        }
        void update(std::uint32_t branch_pc, BranchPrediction prediction, bool actual_taken) override
        {
            std::uint8_t& counter = counters_[prediction.metadata];
            if (actual_taken) {
                if (counter < 3)
                    ++counter;
            } else if (counter > 0) {
                --counter;
            }
            history_ = ((history_ << 1) | (actual_taken ? 1u : 0u)) & 0x3fu;
        }
    private:
        std::array<std::uint8_t, 64> counters_{};
        std::uint32_t history_{0};

    };
}

#endif //PIPELINE_SIMULATOR_BRANCHPREDICTOR_H
