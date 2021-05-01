#pragma once
#include <future>
#include <variant>

namespace pro
{
	namespace detail 
	{
		template<typename T>
		class _promise_base {
		public:

			template<typename Function, typename... Args>
			_promise_base(Function&& fun, Args&&... args) {
				this->future = std::async(std::launch::async, std::forward<Function>(fun), std::forward<Args>(args)...);
			}
			_promise_base(std::future<T>&& _future) : future(std::move(_future)) {
			}

			_promise_base(const _promise_base&) = delete;

			bool valid() const noexcept {
				return this->future.valid();
			}

			virtual ~_promise_base() = default;

		protected:
			std::future<T> future;
		};
	}
}