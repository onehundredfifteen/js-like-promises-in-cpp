#pragma once

#ifndef UTILS_INCLUDED
#define UTILS_INCLUDED

#include "./utils/type_utils.h"

#include "./utils/concurrency_all.h"
#include "./utils/concurrency_pack.h"

namespace pro {

	template<class Container, typename CT = promise_type_utils::CollectionTypeTraits<Container> >
		typename std::enable_if_t<type_utils::is_container<Container>::value,
	Promise<typename CT::ReturnType>>
	PromiseAll(Container& container)
	{
		return make_promise<CT::ReturnType>(
			concurrency::concurrency_call_wrapper<concurrency::_promise_all<CT::PromiseType>, Container>::call,
			std::move(container)
		);
	}

	template<typename... Args>
		typename std::enable_if_t<promise_type_utils::is_all_promise<Args...>::value,
	Promise<std::tuple<typename Args::value_type...>> >
	PromiseAll(Args &... tail)
	{
		return make_promise<std::tuple<typename Args::value_type...>>(
			[](auto pc) {
				std::exception_ptr eptr;
				try {
					auto ppp = pc.settleAll();
					return ppp;
				}
				catch (...) {
					eptr = std::current_exception();
				}

				if (eptr) {
					std::rethrow_exception(eptr);
				}
				else throw std::logic_error("PromiseAll<Args...> unhandled control path");
			},
			concurrency_pack::detail::_promise_collection<Args...>(tail...)
		);
	}
}

#endif //UTILS_INCLUDED