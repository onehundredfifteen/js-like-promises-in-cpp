#pragma once

#ifndef UTIL_PROMISE_INCLUDED
#define UTIL_PROMISE_INCLUDED

#include <tuple>
//#include "./ready_promise.h"
#include "./promise.h"

namespace pro 
{
	namespace detail
	{
		template<size_t idx, typename T>
		struct _get_helper;

		template<class... _Promises>
		struct _promise_collection {};

		template<typename P, typename ..._Promises>
		struct _promise_collection<P, _Promises...>
		{
			using Result = typename P::value_type;

			_promise_collection(P& first, _Promises& ... rest)
				: promise(first)
				, rest(rest...)
			{}

			P& promise;
			_promise_collection<_Promises...> rest;
			_promise_state<Result> promise_state;

			void _resolve(Result value) {
				promise_state.set_resolved(value);
			}

			void _reject(Result value) {
				promise_state.set_rejected(value);
			}

			void _reject_ex(std::exception_ptr eptr) {
				promise_state.set_rejected(eptr);
			}

			template<size_t idx>
			auto get() {
				return _get_helper<idx, _promise_collection<P, _Promises...>>::get(*this);
			}
			auto settleAll() {
				return _get_helper<sizeof...(_Promises), _promise_collection<P, _Promises...>>::settle(*this);
			}
		};

		template<typename P>
		struct _get_helper<0, _promise_collection<P>> {

			static P get(_promise_collection<P>& collection) {
				return collection.promise;
			}
			static std::tuple<typename P::value_type> 
				settle(_promise_collection<P>& collection) {
				std::exception_ptr eptr = nullptr;
				typename P::value_type val, rej;
				bool rej_invoked = false;
				
				auto cba = [&val](typename P::value_type _res) { val = _res; };
				auto cbb = [&rej, &rej_invoked](typename P::value_type _res) { rej = _res; rej_invoked = true; };
				auto cbc = [&eptr](std::exception_ptr ep) { eptr = ep; };

				collection.promise.then(cba, cbb, cbc);

				if (rej_invoked) throw rej;
				if (eptr) std::rethrow_exception(eptr);

				return std::make_tuple(val);
			}
		};

		template<size_t idx, typename P, typename... _Promises>
		struct _get_helper<idx, _promise_collection<P, _Promises...>> {

			static auto get(_promise_collection<P, _Promises...>& collection) {
				return _get_helper<idx - 1, _promise_collection<_Promises ...>>::get(collection.rest);
			}
			static std::tuple<typename P::value_type, typename _Promises::value_type...> 
				settle(_promise_collection<P, _Promises...>& collection) {
				std::exception_ptr eptr = nullptr;
				typename P::value_type val, rej;
				bool rej_invoked = false;

				auto cba = [&val](typename P::value_type _res) { val = _res; };
				auto cbb = [&rej, &rej_invoked](typename P::value_type _res) { rej = _res; rej_invoked = true; };
				auto cbc = [&eptr](std::exception_ptr ep) { eptr = ep; };

				collection.promise.then(cba, cbb, cbc);

				if (rej_invoked) throw rej;
				if (eptr) std::rethrow_exception(eptr);

				return std::tuple_cat(std::make_tuple(val), _get_helper<idx - 1, _promise_collection<_Promises ...>>::settle(collection.rest));
			}
		};

	}

	template<typename... Args>
	Promise<std::tuple<typename Args::value_type...> >
	PromiseAll(Args &... tail)
	{
		detail::_promise_collection<Args...> promise_collection(tail...);
		return Promise<std::tuple<typename Args::value_type...>>
		(
			[](auto pc) {
				std::exception_ptr eptr;
				try {
					return pc.settleAll();
				}
				catch (...) {
					eptr = std::current_exception();
				}

				if (eptr) {
					std::rethrow_exception(eptr);
				}
				else throw std::logic_error("PromiseAll unhandled control path");
			}, 
			std::forward<detail::_promise_collection<Args...>>(promise_collection)
		);
	}
	

	/*
	std::tuple<typename _Pro::value_type> all(_Pro & first) {

				typename _Pro::value_type val;
				auto cb = [&val](typename _Pro::value_type _res) mutable {val = _res; };

				first.then(cb);

				return std::make_tuple(val);

			}

			template<typename _Pro, typename... Args>
			std::tuple<typename _Pro::value_type, typename Args::value_type...>
				all(_Pro& first, Args &... tail) {

				typename _Pro::value_type val;
				auto cb = [&val](typename _Pro::value_type _res) mutable {val = _res; };

				first.then(cb);

				return std::tuple_cat(std::make_tuple(val), all(tail...));
			}
	}*/
}

#endif //PROMISE_BASE_INCLUDED