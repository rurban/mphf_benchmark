#pragma once

#include <fcntl.h>
#include <sstream>

#include "../external/pthash/include/pthash.hpp"
#include "../external/pthash/include/encoders/encoders.hpp"
#include "../external/pthash/include/utils/hasher.hpp"

namespace mphf {

template <bool partitioned, typename Encoder>
struct PTHashWrapper {
    struct Builder {
        Builder(float c, float alpha, uint64_t num_threads = 1, uint64_t partitions = 0)
            : m_c(c), m_alpha(alpha), m_num_threads(num_threads),
              m_partitions(partitions == 0 ? num_threads : partitions)
        {
            if (c < 1.45) { throw std::invalid_argument("`c` must be greater or equal to 1.45"); }
            if (alpha <= 0 || 1 < alpha) {
                throw std::invalid_argument(
                    "`alpha` must be between 0 (excluded) and 1 (included)");
            }

            std::stringstream ss;
            ss << "PTHash(encoder=" << Encoder::name();
            ss << ", c=" << c;
            ss << ", alpha=" << alpha;
            if (partitioned) {
                ss << ", threads=" << num_threads;
                ss << ", partitions=" << m_partitions;
            }
            ss << ")";
            m_name = ss.str();
        }

        template <typename T>
        PTHashWrapper build(const std::vector<T>& keys, uint64_t seed = 0,
                            bool verbose = false) const {
            PTHashWrapper pthash_wrapper;
            build(pthash_wrapper, keys, seed, verbose);
            return pthash_wrapper;
        }

        template <typename T>
        void build(PTHashWrapper& pthash_wrapper, const std::vector<T>& keys, uint64_t seed = 0,
                   bool verbose = false) const {
            pthash::build_configuration config;
            config.c = m_c;
            config.alpha = m_alpha;
            config.num_threads = m_num_threads;
            config.num_partitions = m_partitions;
            config.verbose_output = verbose;
            config.seed = seed;

            pthash_wrapper.m_pthash.build_in_internal_memory(keys.begin(), keys.size(), config);
        }

        std::string name() const {
            return m_name;
        }

    private:
        float m_c, m_alpha;
        std::string m_name;
        uint64_t m_num_threads;
        uint64_t m_partitions;
    };

    template <typename T>
    inline uint64_t operator()(
        const T& key) const {  // this should be const but `lookup` is not const
        return m_pthash(key);
    }

    inline size_t num_bits() const {
        return m_pthash.num_bits();
    }

private:
    std::conditional_t<partitioned,
        pthash::partitioned_mphf<pthash::murmurhash2_64, Encoder>,
        pthash::single_mphf<pthash::murmurhash2_64, Encoder>
    > m_pthash;
};

}  // namespace mphf
