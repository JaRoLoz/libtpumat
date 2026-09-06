#include <vector>
#include "libtpumat/tpu.hpp"

void tpumat::TPU::reset_dma()
{
    m_user_dev.write_value<uint32_t>(MM2S_DMACR, 0x0004);
    m_user_dev.write_value<uint32_t>(S2MM_DMACR, 0x0004);
    while (true)
    {
        const auto mm2s_ctrl = m_user_dev.read_value<uint32_t>(MM2S_DMACR);
        const auto s2mm_ctrl = m_user_dev.read_value<uint32_t>(S2MM_DMACR);
        if (!(mm2s_ctrl & 0x0004) && !(s2mm_ctrl & 0x0004))
            break;
    }
}