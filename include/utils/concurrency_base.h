#pragma once

#ifndef CONCURRENCY_BASE_INCLUDED
#define CONCURRENCY_BASE_INCLUDED

#include <tuple>
#include <queue>
#include "./../promise.h"

namespace pro 
{
	namespace concurrency 
	{
		namespace detail
		{
			template <typename P>
			struct _promise_concurrency_base
			{
				using Result = typename P::value_type;

				bool yield_results;
				const unsigned int res_limit;
				const unsigned int rej_limit;

				unsigned int cb_count;

				std::vector<Result> outcome;
				std::queue<std::tuple<int, std::exception_ptr>> outcome_rejections_idx;

				template <typename PromiseContainer>
				_promise_concurrency_base(PromiseContainer&& pc, unsigned int resLimit, unsigned int rejLimit)
					: res_limit(resLimit),
					rej_limit(rejLimit),
					outcome(res_limit),
					yield_results(false),
					cb_count(0)
				{
					int n = 0;
					auto _begin = std::begin(pc);
					auto _end = std::end(pc);

					std::queue<std::unique_ptr<Promise<void>>> pp;
					for (auto it = _begin; it < _end; ++it) {
						pp.push(std::make_unique<Promise<void>>(settle(*it, n++)));
					}
				}

				void _resolve(Result value, unsigned int idx) {
					outcome[idx] = value;

					if (++cb_count >= res_limit)
						yield_results = true;
				}

				void _reject(Result value, unsigned int idx) {
					outcome[idx] = value;

					outcome_rejections_idx.push(std::make_tuple(idx, nullptr));
					if (outcome_rejections_idx.size()
						>= rej_limit)
						yield_results = true;
				}

				void _reject_ex(std::exception_ptr eptr, unsigned int idx) {
					outcome_rejections_idx.push(std::make_tuple(idx, eptr));

					if (outcome_rejections_idx.size()
						>= rej_limit)
						yield_results = true;
				}

				Promise<void> settle(P& promise, unsigned int idx) {
					auto resolveBound = std::bind(&_promise_concurrency_base<P>::_resolve, this, std::placeholders::_1, idx);
					auto rejectBound = std::bind(&_promise_concurrency_base<P>::_reject, this, std::placeholders::_1, idx);
					auto rejectExBound = std::bind(&_promise_concurrency_base<P>::_reject_ex, this, std::placeholders::_1, idx);

					if (false == promise.valid()) {
						//_reject_ex(idx, make_exception_ptr(std::invalid_argument("Invalidated promise")));
					}

					return promise.then(resolveBound, rejectBound, rejectExBound);
				}

				void wait() {
					while (false == yield_results) {
					}
				}

				virtual std::vector<Result> yield() = 0;
			};

			template <>
			struct _promise_concurrency_base<Promise<void>>
			{
				bool yield_results;
				const unsigned int res_limit;
				const unsigned int rej_limit;

				unsigned int cb_count;

				std::queue<std::tuple<int, std::exception_ptr>> outcome_rejections_idx;

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

					std::queue<std::unique_ptr<Promise<void>>> pp;
					for (auto it = _begin; it < _end; ++it) {
						pp.push(std::make_unique<Promise<void>>(settle(*it, n++)));
					}
				}

				void _resolve() {
					if (++cb_count >= res_limit)
						yield_results = true;
				}

				void _reject(unsigned int idx) {
					outcome_rejections_idx.push(std::make_tuple(idx, nullptr));

					if (outcome_rejections_idx.size()
						>= rej_limit)
						yield_results = true;
				}

				void _reject_ex(std::exception_ptr eptr, unsigned int idx) {
					outcome_rejections_idx.push(std::make_tuple(idx, eptr));

					if (outcome_rejections_idx.size()
						>= rej_limit)
						yield_results = true;
				}

				Promise<void> settle(Promise<void>& promise, unsigned int idx) {
					auto resolveBound = std::bind(&_promise_concurrency_base<Promise<void>>::_resolve, this);
					auto rejectBound = std::bind(&_promise_concurrency_base<Promise<void>>::_reject, this, idx);
					auto rejectExBound = std::bind(&_promise_concurrency_base<Promise<void>>::_reject_ex, this, std::placeholders::_1, idx);

					if (false == promise.valid()) {
						//_reject_ex(idx, make_exception_ptr(std::invalid_argument("Invalidated promise")));
					}

					return promise.then(resolveBound, rejectBound, rejectExBound);
				}

				void wait() {
					while (false == yield_results) {
					}
				}

				virtual void yield() = 0;
			};
		}

		template <typename Method, typename Container,
				 typename CT = promise_type_utils::CollectionTypeTraits<Container>>
		struct concurrency_call_wrapper 
		{
			static typename CT::ReturnType call(Container && collection) {
				if (std::size(collection) == 0) {
					if constexpr (std::is_same<CT::ValueType, void>::value)
						return;
					else return std::vector<CT::ValueType>();
				}

				Method states(collection);
				return states.yield();
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
}

#endif //CONCURRENCY_BASE_INCLUDED