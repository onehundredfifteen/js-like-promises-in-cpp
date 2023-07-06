#pragma once

#ifndef CONCURRENCY_RACE_INCLUDED
#define CONCURRENCY_RACE_INCLUDED

#include "./concurrency_base.h"

namespace pro 
{
	namespace concurrency
	{
		template <typename P>
		struct _promise_race : detail::_promise_concurrency_base<P, false>
		{
			using YieldType = typename detail::_promise_concurrency_base<P, false>::Result;

			template <typename PromiseContainer>
			_promise_race(PromiseContainer&& pc)
				: detail::_promise_concurrency_base<P, false>(pc, 1, 1) {}

			YieldType yield() override
			{
				wait();

				if (rejection_results.size() > 0)
				{
					auto rejection = rejection_results.pop();

					if (std::get<2>(rejection) == nullptr) {
						throw YieldType(std::get<1>(rejection));
					}
					else {
						std::rethrow_exception(std::get<2>(rejection));
					}
				}
				if (results.size() == 0) {
					throw std::logic_error("_promise_race<T>.yield() unexpected result count");
				}
				return std::get<1>(results.pop());
			}
		};

		//_promise_race on Promise<void> is not supported
	}
}

#endif //CONCURRENCY_RACE_INCLUDED