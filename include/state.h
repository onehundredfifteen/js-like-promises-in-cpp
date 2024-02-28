#pragma once
#ifndef PROMISE_STATE_INCLUDED
#define PROMISE_STATE_INCLUDED

#include <variant>

namespace pro
{
	namespace detail 
	{
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

#endif //PROMISE_STATE_INCLUDED