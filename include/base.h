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
			using value_type = T;

			template<typename Function, typename... Args>
			_promise_base(Function&& fun, Args&&... args) {
				this->future = std::async(std::launch::async, std::forward<Function>(fun), std::forward<Args>(args)...);
			}
			explicit _promise_base(_promise_base<T>&& _promise) :
				future(std::move(_promise.future)) {
			}
			explicit _promise_base(const std::future<T>& _future) :
				future(std::move(_future)) {
			}
			explicit _promise_base(std::future<T>&& _future) :
				future(std::forward<std::future<T>>(_future)) {
			}

			_promise_base(const _promise_base&) = delete;

			virtual ~_promise_base() = default;

			_promise_base& operator=(_promise_base&&) = default;
			_promise_base& operator=(const _promise_base&) = delete;

			virtual bool valid() const noexcept {
				return this->future.valid();
			}

			explicit operator bool() const noexcept {
				return this->valid();
			}

			auto share_future() {
				return this->future.share();
			}

		protected:
			std::future<T> future;
		};

		template<typename T>
		class _promise_state {
		public:
			_promise_state() : state(_state::pPending) {}

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

			bool is_resolved() const {
				return this->state == _state::pResolved;
			}
			bool is_rejected() const {
				return this->state == _state::pRejected;
			}
			bool is_pending() const {
				return this->state == _state::pPending;
			}

			T get_value() const {
				if (this->holds_type<T>()) {
					return std::get<T>(this->value);
				}
				else {
					std::rethrow_exception(std::get<std::exception_ptr>(this->value));
				}
			}

			template<typename H>
			bool holds_type() const {
				return std::holds_alternative<H>(this->value);
			}

		private:
			enum class _state {
				pPending,
				pResolved,
				pRejected
			};

			_state state;
			std::variant<std::monostate, T, std::exception_ptr> value;
		};
	}
}

#endif //PROMISE_BASE_INCLUDED