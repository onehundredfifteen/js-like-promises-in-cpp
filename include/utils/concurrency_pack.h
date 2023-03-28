#pragma once

#ifndef CONCURRENCY_PACK_INCLUDED
#define CONCURRENCY_PACK_INCLUDED

#include <tuple>

#include "./../promise.h"

namespace pro 
{
	namespace concurrency_pack
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

				_promise_collection(P& first, _Promises& ...rest)
					: promise(std::move(first))
					, rest(rest...)
				{}

				P promise;
				_promise_collection<_Promises...> rest;
				pro::detail::_promise_state<Result> promise_state;

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

					auto resolveBound = std::bind(&_promise_collection<P>::_resolve, &collection, std::placeholders::_1);
					auto rejectBound = std::bind(&_promise_collection<P>::_reject, &collection, std::placeholders::_1);
					auto rejectExBound = std::bind(&_promise_collection<P>::_reject_ex, &collection, std::placeholders::_1);

					if (false == collection.promise.valid()) {
						return std::make_tuple(P::value_type());
					}

					collection.promise.then(resolveBound, rejectBound, rejectExBound);

					if (collection.promise_state.is_rejected()) {
						//if rejected and holds no exception type, throw tuple 
						/*if (collection.promise_state.holds_type<P::value_type>()) {
							throw std::make_tuple(collection.promise_state.get_value());
						}*/
						//otherwise, throw an exception
						throw collection.promise_state.get_value();
					}

					return std::make_tuple(collection.promise_state.get_value());
				}
			};

			template<size_t idx, typename P, typename... _Promises>
			struct _get_helper<idx, _promise_collection<P, _Promises...>> {

				static auto get(_promise_collection<P, _Promises...>& collection) {
					return _get_helper<idx - 1, _promise_collection<_Promises ...>>::get(collection.rest);
				}
				static std::tuple<typename P::value_type, typename _Promises::value_type...>
					settle(_promise_collection<P, _Promises...>& collection) {

					auto resolveBound = std::bind(&_promise_collection<P, _Promises...>::_resolve, &collection, std::placeholders::_1);
					auto rejectBound = std::bind(&_promise_collection<P, _Promises...>::_reject, &collection, std::placeholders::_1);
					auto rejectExBound = std::bind(&_promise_collection<P, _Promises...>::_reject_ex, &collection, std::placeholders::_1);

					if constexpr (std::is_same<P, ReadyPromise<P::value_type>>::value) {
						return std::tuple_cat(std::make_tuple(P::value_type()), _get_helper<idx - 1, _promise_collection<_Promises ...>>::settle(collection.rest));
					}
					//an invalid Promise will set a default value
					if (false == collection.promise.valid()) {
						return std::tuple_cat(std::make_tuple(P::value_type()), _get_helper<idx - 1, _promise_collection<_Promises ...>>::settle(collection.rest));
					}

					auto o = collection.promise.then(resolveBound, rejectBound, rejectExBound);

					if (collection.promise_state.is_rejected()) {
						//if rejected and holds no exception type, throw tuple 
						/*if (collection.promise_state.holds_type<P::value_type>()) {
							throw std::tuple_cat(std::make_tuple(collection.promise_state.get_value()), _get_helper<idx - 1, _promise_collection<_Promises ...>>::settle(collection.rest));
						}*/
						//otherwise, throw an exception
						throw collection.promise_state.get_value();
					}

					return std::tuple_cat(std::make_tuple(collection.promise_state.get_value()), _get_helper<idx - 1, _promise_collection<_Promises ...>>::settle(collection.rest));
				}
			};
		}

		template <typename Collection, typename... Args>
		struct concurrency_call_wrapper 
		{
			static std::tuple<typename Args::value_type...> call(Collection && collection) {				
				std::exception_ptr eptr;
				try {
					auto ppp = collection.settleAll();
					return ppp;
				}
				catch (...) {
					eptr = std::current_exception();
				}

				if (eptr) {
					std::rethrow_exception(eptr);
				}
				else throw std::logic_error("PromiseAll unhandled control path");
			}
		};
	}
}

#endif //CONCURRENCY_PACK_INCLUDED