//put this in type_utils.hpp 
#ifndef commn_utils_type_utils_hpp
#define commn_utils_type_utils_hpp

#include <type_traits>
#include <valarray>

namespace pro {
    namespace type_utils {
        //from: https://raw.githubusercontent.com/louisdx/cxx-prettyprint/master/prettyprint.hpp
        //also see https://gist.github.com/louisdx/1076849
        namespace detail
        {
            // SFINAE type trait to detect whether T::const_iterator exists.

            struct sfinae_base
            {
                using yes = char;
                using no = yes[2];
            };

            template <typename T>
            struct has_const_iterator : private sfinae_base
            {
            private:
                template <typename C> static yes& test(typename C::const_iterator*);
                template <typename C> static no& test(...);
            public:
                static const bool value = sizeof(test<T>(nullptr)) == sizeof(yes);
                using type = T;

                void dummy(); //for GCC to supress -Wctor-dtor-privacy
            };

            template <typename T>
            struct has_begin_end : private sfinae_base
            {
            private:
                template <typename C>
                static yes& f(typename std::enable_if<
                    std::is_same<decltype(static_cast<typename C::const_iterator(C::*)() const>(&C::begin)),
                    typename C::const_iterator(C::*)() const>::value>::type*);

                template <typename C> static no& f(...);

                template <typename C>
                static yes& g(typename std::enable_if<
                    std::is_same<decltype(static_cast<typename C::const_iterator(C::*)() const>(&C::end)),
                    typename C::const_iterator(C::*)() const>::value, void>::type*);

                template <typename C> static no& g(...);

            public:
                static bool const beg_value = sizeof(f<T>(nullptr)) == sizeof(yes);
                static bool const end_value = sizeof(g<T>(nullptr)) == sizeof(yes);

                void dummy(); //for GCC to supress -Wctor-dtor-privacy
            };

        }  // namespace detail

        // Basic is_container template; specialize to derive from std::true_type for all desired container types

        template <typename T>
        struct is_container : public std::integral_constant<bool,
            detail::has_const_iterator<T>::value&&
            detail::has_begin_end<T>::beg_value&&
            detail::has_begin_end<T>::end_value> { };

        template <typename T, std::size_t N>
        struct is_container<T[N]> : std::true_type { };

        template <std::size_t N>
        struct is_container<char[N]> : std::false_type { };

        template <typename T>
        struct is_container<std::valarray<T>> : std::true_type { };

        template <typename T1, typename T2>
        struct is_container<std::pair<T1, T2>> : std::true_type { };

        template <typename ...Args>
        struct is_container<std::tuple<Args...>> : std::true_type { };

    }

    namespace promise_type_utils {

        template <typename... Args>
        using is_all_promise = std::integral_constant<bool, (std::is_same_v<Args, pro::promise<typename Args::value_type>> && ...)>;

        template <typename Container>
        struct collection_type_traits {
            using PromiseType = typename Container::value_type;
            using ValueType = typename PromiseType::value_type;
            using ReturnType = typename std::conditional<!std::is_same<ValueType, void>::value, std::vector<ValueType>, void>::type;
        };

        //template <typename Res>
        //using ReturnType = typename std::conditional<!std::is_same<Res, void>::value, std::vector<Res>, void>::type;
    }


}  //namespace
#endif