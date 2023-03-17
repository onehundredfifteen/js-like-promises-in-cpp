#pragma once

#ifndef UTIL_PROMISE_INCLUDED
#define UTIL_PROMISE_INCLUDED

#include <tuple>
#include <queue>
#include <type_traits>

#include "./type_utils.h"

#include "./promise.h"
#include "./ready_promise.h"


namespace pro 
{
	namespace detail::typed_collections
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

	namespace detail::collections {
		template <typename P>
		struct _promise_collection
		{
			using Result = typename P::value_type;

			bool yield_results;
			const unsigned int res_limit;
			const unsigned int rej_limit;

			unsigned int cb_count;

			std::vector<Result> outcome;
			std::queue<int> outcome_rejections_idx;
			std::queue<std::tuple<int, std::exception_ptr>> outcome_eptr_idx;

			template <typename PromiseContainer>
			_promise_collection(PromiseContainer && pc, unsigned int resLimit, unsigned int rejLimit)
				: 
				res_limit(resLimit),
				rej_limit(rejLimit),
				outcome(res_limit),
				yield_results(false),
				cb_count(0)
			{
				int n = 0;
				auto _begin = std::begin(pc);
				auto _end = std::end(pc);

				for (auto it = _begin; it < _end; ++it) {
					settle(*it, n++);
				}
			}

			void _resolve(unsigned int idx, Result value) {
				outcome[idx] = value;

				if (++cb_count >= res_limit)
					yield_results = true;
			}

			void _reject(unsigned int idx, Result value) {
				outcome[idx] = value;

				outcome_rejections_idx.push(idx);
				if (outcome_rejections_idx.size() + outcome_eptr_idx.size()
					>= rej_limit)
					yield_results = true;
			}

			void _reject_ex(unsigned int idx, std::exception_ptr eptr) override {
				outcome_eptr_idx.push(std::make_tuple(idx, eptr));

				if (outcome_rejections_idx.size() + outcome_eptr_idx.size()
					>= rej_limit)
					yield_results = true;
			}

			void settle(P& promise, int idx) override {

				auto resolveBound = std::bind(&_promise_collection<P>::_resolve, this, idx, std::placeholders::_1);
				auto rejectBound = std::bind(&_promise_collection<P>::_reject, this, idx, std::placeholders::_1);
				auto rejectExBound = std::bind(&_promise_collection<P>::_reject_ex, this, idx, std::placeholders::_1);

				if (false == promise.valid()) {
					_reject_ex(idx, make_exception_ptr(std::invalid_argument("Invalidated promise")));
				}

				promise.then(resolveBound, rejectBound, rejectExBound);
			}

			void wait() {
				while (false == yield_results) {
				}
			}

			virtual std::vector<Result> yield() = 0;			
		};

		template <typename P>
		struct _promise_collection_all : _promise_collection<P>
		{
			using Result = typename P::value_type;
			
			template <typename PromiseContainer>
			_promise_collection_all(PromiseContainer && pc)
				: _promise_collection<P>(pc, std::size(pc), 1){}

			std::vector<Result> yield() override {
				wait();

				if (outcome_rejections_idx.size() > 0) {
					throw std::vector<Result> { outcome[outcome_rejections_idx.front()] };
				}
				else if (outcome_eptr_idx.size() > 0) {
					std::rethrow_exception(std::get<1>(outcome_eptr_idx.front()));
				}
				return outcome;
			}
		};
	}

	class AggregateException : public std::exception {
	public:
		const std::exception_ptr* inner_exceptions;

		template <typename ExceptionContainer>
		AggregateException(ExceptionContainer&& ec)
			: inner_exceptions(new std::exception_ptr[std::size(ec)])
		{
			int n = 0;
			auto _begin = std::begin(ec);
			auto _end = std::end(ec);

			for (auto it = _begin; it < _end; ++it) {
				inner_exceptions[n++] = *it;
			}
		}

		const char* what() {
			return "Aggregate exception - check inner_exceptions";
		}
	};

	template<class Container, 
		typename P = Container::value_type,
		typename Res = P::value_type>
		typename std::enable_if_t<common_utils::type_utils::is_container<Container>::value,
	Promise<std::vector<Res>> >
	PromiseAll(Container & container)
	{
		return Promise<std::vector<Res>>
		(
			[](auto collection) {
				if (std::size(collection) == 0) {
					return std::vector<Res>();
				}
				detail::collections::_promise_collection_all<P> states(collection);
				return states.yield();	
			}, 
			std::move(container)
		);
	}

	template<typename... Args>
		typename std::enable_if_t<common_utils::promise_type_utils::is_all_promise<Args...>::value,
	Promise<std::tuple<typename Args::value_type...>> >
	PromiseAll(Args &... tail)
	{
		return Promise<std::tuple<typename Args::value_type...>>
			(
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
					else throw std::logic_error("PromiseAll unhandled control path");
				},
				detail::typed_collections::_promise_collection<Args...>(tail...)
			);
	}
}

#endif //PROMISE_BASE_INCLUDED