#pragma once
#ifndef READY_PROMISE_INCLUDED
#define READY_PROMISE_INCLUDED

#include "./promise.h"

namespace pro
{
	template<typename T>
	//, std::enable_if_t<std::is_void<T>::value, bool> = false>
	class ReadyPromise : public detail::_promise_base<T> {
	public:
		template<typename Function, typename... Args>
		ReadyPromise(Function&& fun, Args&&... args) :
			detail::_promise_base<T>(std::forward<Function>(fun), std::forward<Args>(args)...) {
	
			auto resolveBound = std::bind(&ReadyPromise<T>::_resolve, this, std::placeholders::_1);
			auto rejectBound = std::bind(&ReadyPromise<T>::_reject, this, std::placeholders::_1);
			auto rejectExBound = std::bind(&ReadyPromise<T>::_reject_ex, this, std::placeholders::_1);

			this->continuation = std::make_unique<Promise<void>>(
				[future = this->future.share(), callback = resolveBound, rejectCallback = rejectBound, exceptionCallback = rejectExBound]() {
					std::exception_ptr eptr;
					try {
						T result = future.get();
						try {
							return callback(std::move(result));
						}
						catch (...) {
							eptr = std::current_exception();
						}
					}
					catch (T& ex) {
						return rejectCallback(std::move(ex));
					}
					catch (...) {
						eptr = std::current_exception();
					}

					if (eptr) {
						return exceptionCallback(std::move(eptr));
					}
				}
			);
		}
		
		virtual bool valid() const noexcept override {
			return continuation.get()->valid();
		}

		bool ready() const {
			return state.get_state() != detail::_promise_state<T>::_state::pPending;
		}
		bool resolved() const {
			return state.get_state() == detail::_promise_state<T>::_state::pResolved;
		}
		bool rejected() const {
			return state.get_state() == detail::_promise_state<T>::_state::pRejected;
		}

		T get() const {
			if (false == this->ready()) {
				throw std::future_error(make_error_code(std::future_errc::no_state));
			}
			else {
				return state.get_value();
			}
		}

		template<typename Cb, typename Result = std::invoke_result_t<Cb>>
		Promise<Result> then(Cb&& callback) {
			return continuation.get()->then(std::forward<Cb>(callback));
		}
		
	private:
		void _resolve(T value) {
			state.set_resolved(value);
		}

		void _reject(T value) {
			state.set_rejected(value);
		}

		void _reject_ex(std::exception_ptr eptr) {
			state.set_rejected(eptr);
		}

		detail::_promise_state<T> state;
		std::unique_ptr<Promise<void>> continuation;
	};
}

#endif //READY_PROMISE_INCLUDED