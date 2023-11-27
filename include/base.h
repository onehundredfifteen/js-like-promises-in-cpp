#pragma once
#ifndef PROMISE_BASE_INCLUDED
#define PROMISE_BASE_INCLUDED

#include <future>
#include <variant>
#include "./pool.h"

namespace pro
{
	namespace detail 
	{
		template<typename T>
		class _promise_base {
		public:
			using value_type = T;

			template<typename Function, typename... Args,
				typename = std::enable_if_t<std::is_invocable_r_v<T, Function, Args...>> >
			_promise_base(Function&& fun, Args&&... args):
				future(std::async(std::launch::async, std::forward<Function>(fun), std::forward<Args>(args)...)) {
			}		
			_promise_base(_promise_base<T>&& _promise) noexcept :
				future(std::move(_promise.future)) {
			}
			_promise_base(std::future<T>&& _future) :
				future(std::move(_future)) {
			}
			template <typename U = T, std::enable_if_t<!std::is_same<U, void>::value, bool> = true,
				typename = std::enable_if_t<!std::is_invocable_v<U>>>
			constexpr _promise_base(U value) {
				std::promise<T> _promise;
				future = _promise.get_future();
				_promise.set_value(std::move(value));
			}
			_promise_base(std::exception_ptr rejection_value) {
				std::promise<T> _promise;
				future = _promise.get_future();
				_promise.set_exception(std::move(rejection_value));
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

			operator std::future<T>() {
				return std::move(future);
			}

			auto share_future() const {
				return this->future.share();
			}

			void async() {
				pool_container::instance().submit<T>(*this);
			}

			template <typename U = T, std::enable_if_t<!std::is_same<U, void>::value, bool> = true>
			constexpr void resolve(U value) {
				detach_and_reset().set_value(std::move(value));
			}
			
			template <typename U = T, std::enable_if_t<std::is_same<U, void>::value, bool> = true>
			constexpr void resolve() {
				detach_and_reset().set_value();
			}

			template <typename U = T, std::enable_if_t<!std::is_same<U, void>::value, bool> = true>
			constexpr void reject(U value) {
				detach_and_reset().set_exception(std::make_exception_ptr(std::move(value)));
			}

			void reject(std::exception_ptr eptr) {	
				detach_and_reset().set_exception(std::move(eptr));
			}

		private:
			std::promise<T> detach_and_reset() {
				std::promise<T> _promise;
				_promise_base<T> _pb(std::move(this->future));
				_pb.async();
				this->future = _promise.get_future();
				return _promise;
			}

		protected:
			std::future<T> future;
		};

		template<typename T>
		class _promise_state {
		public:
			_promise_state() : state(_state::pPending) {}

			void set_resolved(T&& value) {
				this->value = std::move(value);
				this->state = _state::pResolved;
			}
			void set_resolved_asref(const T& value) {
				this->value = value;
				this->state = _state::pResolved;
			}

			void set_rejected(T&& value) {
				this->value = std::move(value);
				this->state = _state::pRejected;
			}
			void set_rejected_asref(const T& value) {
				this->value = value;
				this->state = _state::pRejected;
			}

			void set_rejected(std::exception_ptr&& eptr) {
				this->value = std::move(eptr);
				this->state = _state::pRejected;
			}
			void set_rejected_asref(const std::exception_ptr& eptr) {
				this->value = eptr;
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

		private:
			template<typename H>
			bool holds_type() const {
				return std::holds_alternative<H>(this->value);
			}
	
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