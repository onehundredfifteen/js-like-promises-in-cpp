#pragma once

#ifndef CONCURRENCY_ALL_INCLUDED
#define CONCURRENCY_ALL_INCLUDED

#include "./concurrency_base.h"

namespace pro 
{
	namespace concurrency
	{
		template <typename P>
		struct _promise_all : private detail::_promise_concurrency_base<P, true>
		{
			using YieldType = typename detail::_promise_concurrency_base<P, true>::YieldType;
			using Result = typename detail::_promise_concurrency_base<P, true>::Result;

			template <typename PromiseContainer>
			_promise_all(PromiseContainer&& pc)
				: detail::_promise_concurrency_base<P, true>(pc, std::size(pc), 1) {}
			
			YieldType yield() override
			{
				wait();

				if (rejection_results.size() > 0)
				{
					auto rejection = rejection_results.pop();

					if (std::get<2>(rejection) == nullptr) {
						throw YieldType{ std::get<1>(rejection) };
					}
					else {
						std::rethrow_exception(std::get<2>(rejection));
					}
				}

				return resultsToVector();
			}

			std::vector<Result> resultsToVector() {
				std::vector<Result> vec(results.size());

				while (false == results.empty()) {
					auto el = results.pop();
					vec[std::get<0>(el)] = std::move(std::get<1>(el));
				}

				return vec;
			}
		};

		template <>
		struct _promise_all<promise<void>> 
			: detail::_promise_concurrency_base<promise<void>, false>
		{
			template <typename PromiseContainer>
			_promise_all(PromiseContainer&& pc)
				: detail::_promise_concurrency_base<promise<void>, false>(pc, std::size(pc), 1) {}

			void yield() override 
			{
				wait();

				if (rejection_results.size() > 0) {
					std::rethrow_exception(std::get<1>(rejection_results.pop()));
				}
			}
		};
	}
}

#endif //CONCURRENCY_ALL_INCLUDED