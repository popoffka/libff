#include <cstdio>
#include <vector>

#include "algebra/curves/bn128/bn128_pp.hpp"
#include "algebra/scalar_multiplication/multiexp.hpp"

#include "common/rng.hpp"

#include "common/profiling.hpp"

using namespace libff;

template <typename GroupT>
using run_result_t = std::pair<long long, std::vector<GroupT> >;

template <typename T>
using test_instances_t = std::vector<std::vector<T> >;

template<typename GroupT>
test_instances_t<GroupT> generate_group_elements(size_t count, size_t size)
{
    // generating a random group element is expensive,
    // so for now we only generate a single one and repeat it
    test_instances_t<GroupT> result(count);

    for (size_t i = 0; i < count; i++) {
        GroupT x = GroupT::random_element();
        for (size_t j = 0; j < size; j++) {
            result[i].push_back(x);
        }
    }

    return result;
}

template<typename FieldT>
test_instances_t<FieldT> generate_scalars(size_t count, size_t size)
{
    // we use SHA512_rng because it is much faster than
    // FieldT::random_element()
    test_instances_t<FieldT> result(count);

    for (size_t i = 0; i < count; i++) {
        for (size_t j = 0; j < size; j++) {
            result[i].push_back(SHA512_rng<FieldT>(i * size + j));
        }
    }

    return result;
}

template<typename GroupT, typename FieldT>
run_result_t<GroupT> profile_multiexp(
    test_instances_t<GroupT> group_elements,
    test_instances_t<FieldT> scalars,
    bool use_multiexp)
{
    long long start_time = get_nsec_time();

    std::vector<GroupT> answers;
    for (size_t i = 0; i < group_elements.size(); i++) {
        answers.push_back(multi_exp<GroupT, FieldT>(
            group_elements[i].cbegin(), group_elements[i].cend(),
            scalars[i].cbegin(), scalars[i].cend(),
            1, use_multiexp));
    }

    long long time_delta = get_nsec_time() - start_time;

    return run_result_t<GroupT>(time_delta, answers);
}

template<typename GroupT, typename FieldT>
void print_performance_csv(
    size_t expn_start,
    size_t expn_end_fast,
    size_t expn_end_naive,
    bool compare_answers)
{
    for (size_t expn = expn_start; expn <= expn_end_fast; expn++) {
        printf("%ld", expn);

        test_instances_t<GroupT> group_elements =
            generate_group_elements<GroupT>(10, 1 << expn);
        test_instances_t<FieldT> scalars =
            generate_scalars<FieldT>(10, 1 << expn);

        run_result_t<GroupT> result_fast =
            profile_multiexp<GroupT, FieldT>(
                group_elements, scalars, true);
        printf("\t%lld", result_fast.first);

        if (expn <= expn_end_naive) {
            run_result_t<GroupT> result_naive =
                profile_multiexp<GroupT, FieldT>(
                    group_elements, scalars, false);
            printf("\t%lld", result_naive.first);

            if (compare_answers && (result_fast.second != result_naive.second)) {
                fprintf(stderr, "Answers NOT MATCHING\n");
            }
        }

        printf("\n");
    }
}

int main(void)
{
    print_compilation_info();

    printf("Profiling BN128_G1\n");
    bn128_pp::init_public_params();
    print_performance_csv<G1<bn128_pp>, Fr<bn128_pp> >(2, 20, 14, true);

    printf("Profiling BN128_G2\n");
    print_performance_csv<G2<bn128_pp>, Fr<bn128_pp> >(2, 20, 14, true);

    return 0;
}
