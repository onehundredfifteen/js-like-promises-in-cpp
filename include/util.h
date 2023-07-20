#pragma once

#ifndef UTILS_INCLUDED
#define UTILS_INCLUDED

#include "./utils/type_utils.h"

#include "./utils/concurrency_all.h"
#include "./utils/concurrency_pack.h"
#include "./utils/concurrency_race.h"
#include "./utils/concurrency_any.h"

namespace pro {

	template<class Container, typename = std::enable_if_t<type_utils::is_container<Container>::value>,
		typename CT = promise_type_utils::collection_type_traits<Container>>
	promise<typename CT::ReturnType>
	PromiseAll(Container& container)
	{
		return make_promise<CT::ReturnType>(
			concurrency::concurrency_call_wrapper<concurrency::_promise_all<CT::PromiseType>, Container>::call,
			std::move(container));
	}

	template<typename... Args, typename = std::enable_if_t<promise_type_utils::is_all_promise<Args...>::value>>
	promise<std::tuple<typename Args::value_type...>> 
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

	template<class Container, typename = std::enable_if_t<type_utils::is_container<Container>::value>,
		typename CT = promise_type_utils::collection_type_traits<Container>>
	promise<typename CT::ValueType>
	PromiseRace(Container& container)
	{
		return make_promise<CT::ValueType>(
			concurrency::concurrency_call_wrapper<concurrency::_promise_race<CT::PromiseType>, Container>::call_reduce,
			std::move(container));
	}

	template<class Container, typename = std::enable_if_t<type_utils::is_container<Container>::value>,
		typename CT = promise_type_utils::collection_type_traits<Container>>
	promise<typename CT::ValueType>
	PromiseAny(Container& container)
	{
		return make_promise<CT::ValueType>(
			concurrency::concurrency_call_wrapper<concurrency::_promise_any<CT::PromiseType>, Container>::call_reduce,
			std::move(container));
	}
}

#endif //UTILS_INCLUDED