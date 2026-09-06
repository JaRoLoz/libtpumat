#pragma once

#include "libtpumat/export.hpp"
#include "libtpumem/libtpumem.hpp"

#include <cstdint>
#include <cstddef>
#include <span>
#include <mdspan>
#include <chrono>
#include <print>

namespace tpumat
{
    template <std::size_t M, std::size_t N, typename T = const std::byte>
    using MatrixView = std::mdspan<T, std::extents<std::size_t, M, N>, std::layout_right>;

    template <typename T, typename U>
    concept HasDataPtr = requires(T t) {
        { t.data() } -> std::convertible_to<U *>;
    };

    template <std::size_t M, std::size_t N, typename T>
        requires(HasDataPtr<T, std::byte>)
    MatrixView<M, N, std::byte> to_matrix_view(T &src)
    {
        return MatrixView<M, N, std::byte>{src.data()};
    }

    template <std::size_t M, std::size_t N, typename T>
        requires(HasDataPtr<T, const std::byte>)
    MatrixView<M, N, const std::byte> to_cmatrix_view(T &src)
    {
        return MatrixView<M, N, const std::byte>{src.data()};
    }

    class LIBTPUMAT_API TPU
    {
    public:
        explicit TPU(
            tpumem::DeviceNode h2c_node = s_default_h2c_node,
            tpumem::DeviceNode c2h_node = s_default_c2h_node)
            : m_user_dev{tpumem::DeviceUser::open(tpumem::DeviceNode::User)},
              m_write_dev{tpumem::DeviceH2C::open(h2c_node)},
              m_read_dev{tpumem::DeviceC2H::open(c2h_node)} {}

        template <std::size_t M, std::size_t N, bool MeasurePerf = false>
        [[nodiscard]] void mat_mul(
            std::mdspan<std::byte, std::extents<std::size_t, M, N>> &a,
            std::mdspan<std::byte, std::extents<std::size_t, M, N>> &b,
            std::mdspan<std::byte, std::extents<std::size_t, M, N>> &out)
        {
            return mat_mul<std::byte, M, N, MeasurePerf>(a, b, out);
        }

        template <typename T, std::size_t M, std::size_t N, bool MeasurePerf = false>
        [[nodiscard]] void mat_mul(
            tpumat::MatrixView<M, N, const T> a,
            tpumat::MatrixView<M, N, const T> b,
            tpumat::MatrixView<M, N, T> out);

    private:
        static const constexpr tpumem::DeviceNode s_default_h2c_node = tpumem::DeviceNode::H2C_0;
        static const constexpr tpumem::DeviceNode s_default_c2h_node = tpumem::DeviceNode::C2H_0;

        static const constexpr std::size_t DDR4_BASE_ADDR = 0x80000000;
        static const constexpr std::size_t AXI_DMA_BASE = 0x00000000;
        static const constexpr std::size_t SYSMON_BASE = 0x00010000;
        static const constexpr std::size_t XTPU_CTRL_BASE = 0x00020000;
        static const constexpr std::size_t MM2S_DMACR = AXI_DMA_BASE + 0x00;
        static const constexpr std::size_t MM2S_DMASR = AXI_DMA_BASE + 0x04;
        static const constexpr std::size_t MM2S_SA = AXI_DMA_BASE + 0x18;
        static const constexpr std::size_t MM2S_LENGTH = AXI_DMA_BASE + 0x28;
        static const constexpr std::size_t S2MM_DMACR = AXI_DMA_BASE + 0x30;
        static const constexpr std::size_t S2MM_DMASR = AXI_DMA_BASE + 0x34;
        static const constexpr std::size_t S2MM_DA = AXI_DMA_BASE + 0x48;
        static const constexpr std::size_t S2MM_LENGTH = AXI_DMA_BASE + 0x58;
        static const constexpr std::size_t XTPU_REG_CTRL = XTPU_CTRL_BASE + 0x00;
        static const constexpr std::size_t XTPU_REG_STATUS = XTPU_CTRL_BASE + 0x04;
        static const constexpr std::size_t XTPU_REG_SCALE = XTPU_CTRL_BASE + 0x08;
        static const constexpr std::size_t RAM_BASE = DDR4_BASE_ADDR;
        static const constexpr std::size_t ADDR_A = RAM_BASE + 0x400;
        static const constexpr std::size_t ADDR_B = RAM_BASE;
        static const constexpr std::size_t ADDR_C = RAM_BASE + 0x800;

    private:
        void reset_dma();

    private:
        tpumem::DeviceUser m_user_dev;
        tpumem::DeviceH2C m_write_dev;
        tpumem::DeviceC2H m_read_dev;
    };
};

template <typename T, std::size_t M, std::size_t N, bool MeasurePerf>
void tpumat::TPU::mat_mul(
    tpumat::MatrixView<M, N, const T> a,
    tpumat::MatrixView<M, N, const T> b,
    tpumat::MatrixView<M, N, T> out)
{
    std::chrono::steady_clock::time_point start;

    if constexpr (MeasurePerf)
    {
        start = std::chrono::high_resolution_clock::now();
    }

    reset_dma();
    m_write_dev.write(std::span<const T>{a.data_handle(), a.size()}, ADDR_A);
    m_write_dev.write(std::span<const T>{b.data_handle(), b.size()}, ADDR_B);
    m_write_dev.write_value<uint8_t>(ADDR_C, 0);

    m_user_dev.write_value<uint32_t>(XTPU_REG_SCALE, 0);

    m_user_dev.write_value<uint32_t>(S2MM_DA, static_cast<uint32_t>(ADDR_C));
    m_user_dev.write_value<uint32_t>(S2MM_DMACR, 0x0001);
    m_user_dev.write_value<uint32_t>(S2MM_LENGTH, static_cast<uint32_t>(M * N * sizeof(T)));

    m_user_dev.write_value<uint32_t>(MM2S_SA, static_cast<uint32_t>(ADDR_B));
    m_user_dev.write_value<uint32_t>(MM2S_DMACR, 0x0001);
    m_user_dev.write_value<uint32_t>(MM2S_LENGTH, static_cast<uint32_t>(M * N * sizeof(T)));

    while (true)
    {
        if (m_user_dev.read_value<uint32_t>(MM2S_DMASR) & 0x0002)
            break;
    }

    m_user_dev.write_value<uint32_t>(MM2S_SA, static_cast<uint32_t>(ADDR_A));
    m_user_dev.write_value<uint32_t>(MM2S_LENGTH, static_cast<uint32_t>(M * N * sizeof(T)));

    while (true)
    {
        if (m_user_dev.read_value<uint32_t>(S2MM_DMASR) & 0x0002)
            break;
    }

    if constexpr (MeasurePerf)
    {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::println("[*] tpumat::TPU::mat_mul<{}, {}, {}> completed in {:.6f}s", sizeof(T), M, N, elapsed.count());
    }

    m_read_dev.read(std::span<T>{out.data_handle(), out.size()}, ADDR_C);
};