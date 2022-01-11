#pragma once
#ifndef PROMISE_BASE_INCLUDED
#define PROMISE_BASE_INCLUDED

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
			explicit _promise_base(const std::future<T>& _future) :
				future(std::move(_future)) {
			}
			explicit _promise_base(std::future<T>&& _future) :
				future(std::forward<std::future<T>>(_future)) {
			}

			_promise_base(const _promise_base&) = delete;

			virtual bool valid() const noexcept {
				return this->future.valid();
			}

			auto share_future() {
				return this->future.share();
			}

			virtual ~_promise_base() = default;

		protected:
			std::future<T> future;
		};

		template<typename T>
		class _promise_state {
		public:

			_promise_state() : state(_state::pPending) {
			}

		public:
			enum class _state {
				pPending,
				pResolved,
				pRejected
			};

			void set_resolved(T& value) {
				this->value = value;
				this->state = _state::pResolved;
			}

			void set_rejected(T& value) {
				this->value = value;
				this->state = _state::pRejected;
			}

			void set_rejected(std::exception_ptr value) {
				this->value = value;
				this->state = _state::pRejected;
			}

			T get_value() const {
				if (auto pval = std::get_if<T>(&this->value)) {
					return *pval;
				}
				else {
					std::rethrow_exception(std::get<std::exception_ptr>(this->value));
				}
			}

			_state get_state() const {
				return this->state;
			}

		private:

			_state state;
			std::variant<std::monostate, T, std::exception_ptr> value;
		};
	}
}

#endif //PROMISE_BASE_INCLUDED