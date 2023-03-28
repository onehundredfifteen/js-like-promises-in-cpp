#pragma once

#ifndef CONCURRENCY_ALL_INCLUDED
#define CONCURRENCY_ALL_INCLUDED

#include "./concurrency_base.h"

namespace pro 
{
	namespace concurrency
	{
		template <typename P>
		struct _promise_all : detail::_promise_concurrency_base<P>
		{
			using Result = typename P::value_type;

			template <typename PromiseContainer>
			_promise_all(PromiseContainer&& pc)
				: detail::_promise_concurrency_base<P>(pc, std::size(pc), 1) {}

			std::vector<Result> yield() override 
			{
				wait();

				if (outcome_rejections_idx.size() > 0)
				{
					auto rejection = outcome_rejections_idx.front();

					if (std::get<1>(rejection) == nullptr) {
						throw std::vector<Result> { outcome[std::get<0>(rejection)] };
					}
					else {
						std::rethrow_exception(std::get<1>(rejection));
					}
				}

				return outcome;
			}
		};

		template <>
		struct _promise_all<Promise<void>> 
			: detail::_promise_concurrency_base<Promise<void>>
		{
			template <typename PromiseContainer>
			_promise_all(PromiseContainer&& pc)
				: detail::_promise_concurrency_base<Promise<void>>(pc, std::size(pc), 1) {}

			void yield() override 
			{
				wait();

				if (outcome_rejections_idx.size() > 0) {
					std::rethrow_exception(std::get<1>(outcome_rejections_idx.front()));
				}
			}
		};
	}
}

#endif //CONCURRENCY_ALL_INCLUDED