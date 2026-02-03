#include "../../include/SimulationModules.hpp"
#include <vector>
#include <cstring> // For memcpy

void UpdateChaos(WorldBuffers& b, const NeighborGraph& graph, float diffusionRate) {
    // 1. Create a temporary buffer to avoid simulation artifacts (Double Buffering)
    // We allocate on heap because 1M floats is too big for stack
    static std::vector<float> nextChaos(b.count > 0 ? b.count : 1000000);
    if (nextChaos.size() != b.count) nextChaos.resize(b.count);

    for (uint32_t i = 0; i < b.count; ++i) {
        float current = b.chaos[i];
        float flow = 0.0f;

        int offset = graph.offsetTable[i];
        int count = graph.countTable[i];

        // Gather chaos from neighbors
        for (int n = 0; n < count; ++n) {
            int neighborIdx = graph.neighborData[offset + n];
            float neighborChaos = b.chaos[neighborIdx];

            // Diffusion Formula: Move from High to Low
            flow += (neighborChaos - current) * diffusionRate;
        }

        nextChaos[i] = current + flow;

        // Decay (Chaos naturally fades over time)
        nextChaos[i] *= 0.99f;
    }

    // 2. Write back to the main buffer
    if (b.chaos)
        std::memcpy(b.chaos, nextChaos.data(), b.count * sizeof(float));
}
