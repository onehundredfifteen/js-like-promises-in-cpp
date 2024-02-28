#pragma once
#ifndef READY_PROMISE_INCLUDED
#define READY_PROMISE_INCLUDED

#include "./promise.h"
#include "./state.h"

namespace pro
{
	template<typename T, typename = std::enable_if_t<!std::is_void<T>::value> >
	class readypromise : public detail::_promise_base<T> {
	public:
		template<typename Function, typename... Args>
		readypromise(Function&& fun, Args&&... args) :
			detail::_promise_base<T>(std::forward<Function>(fun), std::forward<Args>(args)...) {
			onResolve(std::bind(&readypromise<T>::_resolve, this, std::placeholders::_1));
			onReject(std::bind(&readypromise<T>::_reject, this, std::placeholders::_1));
		}
		/*
		ReadyPromise(ReadyPromise<T>&& _promise) noexcept :
			detail::_promise_base<T>(std::move(_promise)),
			state(std::move(_promise.state)),
			resolve_cb_list(std::move(_promise.resolve_cb_list)),
			reject_cb_list(std::move(_promise.reject_cb_list)){
		}*/

		typedef std::function<void(T)> _subscribed_callback;
		typedef std::function<void(std::exception_ptr)> _subscribed_callback_ex;

		template<typename Function>
		void onResolve(Function&& fun) {
			resolve_cb_list.push_back(fun);
		}

		template<typename Function>
		void onReject(Function&& fun) {
			reject_cb_list.push_back(fun);
		}

		bool pending() const {
			if (false == state.is_pending())
				return false;
			else if(true == valid())
				return !(future.wait_until(std::chrono::system_clock::time_point::min()) 
					== std::future_status::ready);
			return true;
		}
		bool resolved() const {
			return state.is_resolved();
		}
		bool rejected() const {
			return state.is_rejected();
		}

		T get() {
			if (true == valid()) {
				std::exception_ptr eptr = nullptr;
				try {
					T result = future.get();
					try {
						broadcast_resolve(result);					
					}
					catch (...) {
						eptr = std::current_exception();
					}
				}
				catch (T& ex) {
					broadcast_reject(ex);					
				}
				catch (...) {
					eptr = std::current_exception();
				}

				if (eptr) {
					state.set_rejected(std::move(eptr));
				}			
			}

			return state.get_value();
		}	

		template<typename Cb, typename Result = std::invoke_result_t<Cb, T, std::exception_ptr>>
		promise<Result> then(Cb&& callback) {
			return promise<Result>(
				[future = std::move(this->future), this, callback]() mutable {
					std::exception_ptr eptr;
					try {
						T result = future.get();
						try {
							broadcast_resolve(result);
							return callback(std::move(result), nullptr);
						}
						catch (...) {
							eptr = std::current_exception();
						}
					}
					catch (T& ex) {
						broadcast_reject(ex);
						return callback(std::move(ex), std::make_exception_ptr(ex));
					}
					catch (...) {
						eptr = std::current_exception();
					}

					if (eptr) {
						state.set_rejected_asref(eptr);
						return callback(T(), std::move(eptr));
					}
					else throw std::logic_error("readypromise<T>.then(cb) unhandled control path");
				}
			);
		} 

	private:
		void _resolve(const T& value) {
			state.set_resolved_asref(value);
		}
		void _reject(const T& value) {
			state.set_rejected_asref(value);
		}
		void broadcast(const T& value, const std::list<_subscribed_callback> &list) {
			for (const _subscribed_callback& cb : list)
				cb(value);
		}

	protected:
		void broadcast_resolve(const T& value) {
			broadcast(value, resolve_cb_list);
		}
		void broadcast_reject(const T& value) {
			broadcast(value, reject_cb_list);
		}

	private:
		detail::_promise_state<T> state;
		std::list<_subscribed_callback> resolve_cb_list;
		std::list<_subscribed_callback> reject_cb_list;
	};
}

#endif //READY_PROMISE_INCLUDED