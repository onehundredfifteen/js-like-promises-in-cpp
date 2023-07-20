#pragma once

#ifndef CONCURRENCY_BASE_INCLUDED
#define CONCURRENCY_BASE_INCLUDED

#include <tuple>
#include <queue>
#include "./../promise.h"
#include "./concurrent_queue.h"

namespace pro 
{
	namespace concurrency 
	{
		namespace detail
		{
			template <typename P, bool yieldArray>
			struct _promise_concurrency_base
			{
				using Result = typename P::value_type;
				using YieldType = typename std::conditional<yieldArray,
						std::vector<Result>, Result>::type;

				bool yield_results;
				const unsigned int res_limit;
				const unsigned int rej_limit;

				queue<std::tuple<int, Result>> results;
				queue<std::tuple<int, Result, std::exception_ptr>> rejection_results;

				template <typename PromiseContainer>
				_promise_concurrency_base(PromiseContainer&& pc, unsigned int resLimit, unsigned int rejLimit)
					: res_limit(resLimit),
					rej_limit(rejLimit),
					yield_results(false)
				{
					int n = 0;
					auto _begin = std::begin(pc);
					auto _end = std::end(pc);

					std::queue<std::unique_ptr<promise<void>>> pp;
					for (auto it = _begin; it < _end; ++it) {
						pp.push(std::make_unique<promise<void>>(settle(*it, n++)));
					}
				}

				void _resolve(Result value, unsigned int idx) {
					results.push(std::move(std::make_tuple(idx, std::move(value))));

					if (results.size() >= res_limit)
						yield_results = true;
				}

				void _reject(Result value, unsigned int idx) {
					rejection_results.push(std::move(std::make_tuple(idx, std::move(value), nullptr)));

					if (rejection_results.size()
						>= rej_limit)
						yield_results = true;
				}

				void _reject_ex(std::exception_ptr eptr, unsigned int idx) {
					rejection_results.push(std::move(std::make_tuple(idx, Result(), std::move(eptr))));

					if (rejection_results.size()
						>= rej_limit)
						yield_results = true;
				}

				promise<void> settle(P& _promise, unsigned int idx) {
					auto resolveBound = std::bind(&_promise_concurrency_base<P, yieldArray>::_resolve, this, std::placeholders::_1, idx);
					auto rejectBound = std::bind(&_promise_concurrency_base<P, yieldArray>::_reject, this, std::placeholders::_1, idx);
					auto rejectExBound = std::bind(&_promise_concurrency_base<P, yieldArray>::_reject_ex, this, std::placeholders::_1, idx);

					if (false == _promise.valid()) {
						//_reject_ex(idx, make_exception_ptr(std::invalid_argument("Invalidated promise")));
					}

					return _promise.then(resolveBound, rejectBound, rejectExBound);
				}

				void wait() {
					while (false == yield_results) {
						//std::chrono::system_clock::time_point::min()
						std::this_thread::sleep_for(std::chrono::milliseconds(10));
					}
				}

				virtual YieldType yield() = 0;		
			};

			template <>
			struct _promise_concurrency_base<promise<void>, false>
			{
				bool yield_results;
				const unsigned int res_limit;
				const unsigned int rej_limit;

				unsigned int cb_count;
				queue<std::tuple<int, std::exception_ptr>> rejection_results;

				template <typename PromiseVoidContainer>
				_promise_concurrency_base(PromiseVoidContainer&& pc, unsigned int resLimit, unsigned int rejLimit)
					: res_limit(resLimit),
					rej_limit(rejLimit),
					yield_results(false),
					cb_count(0)
				{
					int n = 0;
					auto _begin = std::begin(pc);
					auto _end = std::end(pc);

					std::queue<std::unique_ptr<promise<void>>> pp;
					for (auto it = _begin; it < _end; ++it) {
						pp.push(std::make_unique<promise<void>>(settle(*it, n++)));
					}
				}

				void _resolve() {
					if (++cb_count >= res_limit)
						yield_results = true;
				}

				void _reject(unsigned int idx) {
					rejection_results.push(std::make_tuple(idx, nullptr));

					if (rejection_results.size()
						>= rej_limit)
						yield_results = true;
				}

				void _reject_ex(std::exception_ptr eptr, unsigned int idx) {
					rejection_results.push(std::make_tuple(idx, eptr));

					if (rejection_results.size()
						>= rej_limit)
						yield_results = true;
				}

				promise<void> settle(promise<void>& _promise, unsigned int idx) {
					auto resolveBound = std::bind(&_promise_concurrency_base<promise<void>, false>::_resolve, this);
					auto rejectBound = std::bind(&_promise_concurrency_base<promise<void>, false>::_reject, this, idx);
					auto rejectExBound = std::bind(&_promise_concurrency_base<promise<void>, false>::_reject_ex, this, std::placeholders::_1, idx);

					if (false == _promise.valid()) {
						//_reject_ex(idx, make_exception_ptr(std::invalid_argument("Invalidated promise")));
					}

					return _promise.then(resolveBound, rejectBound, rejectExBound);
				}

				void wait() {
					while (false == yield_results) {
					}
				}

				virtual void yield() = 0;
			};
		}

		template <typename Method, typename Container,
				 typename CT = promise_type_utils::collection_type_traits<Container>>
		struct concurrency_call_wrapper 
		{
			static typename CT::ReturnType call(Container && collection) {
				if (std::size(collection) == 0) {
					if constexpr (std::is_same<CT::ValueType, void>::value)
						return;
					else 
						return std::vector<CT::ValueType>();
				}

				Method states(collection);
				return states.yield();
			}

			static typename CT::ValueType call_reduce(Container&& collection) {
				if (std::size(collection) == 0) {
					throw std::range_error("Empty collection passed");
				}

				Method states(collection);
				return states.yield();
			}
		};
	}

	class AggregateException : public std::exception {
	public:
		const std::vector<std::exception_ptr> inner_exceptions;

		template <typename ExceptionContainer>
		AggregateException(ExceptionContainer&& ec)
			: inner_exceptions(ec)
		{		
		}

		const char* what() {
			return "Aggregate exception - check inner_exceptions";
		}
	};	
}

#endif //CONCURRENCY_BASE_INCLUDED