#pragma once

#ifndef CONCURRENCY_ANY_INCLUDED
#define CONCURRENCY_ANY_INCLUDED

#include "./concurrency_base.h"

namespace pro 
{
	namespace concurrency
	{
		template <typename P>
		struct _promise_any : detail::_promise_concurrency_base<P, false>
		{
			using YieldType = typename detail::_promise_concurrency_base<P, false>::Result;
			using Result = typename detail::_promise_concurrency_base<P, false>::Result;

			template <typename PromiseContainer>
			_promise_any(PromiseContainer&& pc)
				: detail::_promise_concurrency_base<P, false>(pc, 1, std::size(pc)) {}

			YieldType yield() override
			{
				wait();

				if (rejection_results.size() == rej_limit)
				{
					std::vector<Result> vec(rejection_results.size());
					std::vector<std::exception_ptr> ex_vec;
					if (rejectionsToVector(vec, ex_vec)) {
						throw AggregateException(ex_vec);
					}
					throw vec;					
				}
				else if (results.size() == 0) {
					throw std::logic_error("_promise_any<T>.yield() unexpected result count");
				}
				return std::get<1>(results.pop());
			}

			bool rejectionsToVector(std::vector<Result> &rejections, std::vector<std::exception_ptr>& exceptions) {
				/*
				This method converts queue to a vector<result> but if finds an exception_ptr
				instead transforms all results to ex_ptrs and continues to fill an exception vector.
				*/
				bool any_ex_ptr = false;
				while (false == rejection_results.empty()) {
					auto el = rejection_results.pop();
					if (any_ex_ptr && std::get<2>(el) != nullptr) {
						exceptions[std::get<0>(el)] = std::get<2>(el);
					}
					else if (any_ex_ptr) {
						exceptions[std::get<0>(el)] = std::make_exception_ptr(std::get<1>(el));
					}
					else if (std::get<2>(el) != nullptr) {
						any_ex_ptr = true;
						std::transform(rejections.cbegin(), rejections.cend(), std::back_inserter(exceptions), [](const YieldType& value) {
							return std::make_exception_ptr(value);
						});
						exceptions[std::get<0>(el)] = std::get<2>(el);
					}
					else {
						rejections[std::get<0>(el)] = std::get<1>(el);
					}					
				}

				return any_ex_ptr;
			}
		};

		//_promise_race on promise<void> is not supported
	}
}

#endif //CONCURRENCY_ANY_INCLUDED