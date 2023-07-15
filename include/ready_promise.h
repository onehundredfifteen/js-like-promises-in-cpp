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
			onResolve(std::bind(&ReadyPromise<T>::_resolve, this, std::placeholders::_1));
			onReject(std::bind(&ReadyPromise<T>::_reject, this, std::placeholders::_1));
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
		
		
		/*
		T get() const {
			if (false == this->ready()) {
				throw std::future_error(make_error_code(std::future_errc::no_state));
			}
			else {
				return state.get_value();
			}
		}*/

		template<typename Cb, typename Result = std::invoke_result_t<Cb, T, std::exception_ptr>>
		Promise<Result> then(Cb&& callback) {
			return Promise<Result>(
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
					else throw std::logic_error("ReadyPromise<T>.then(cb) unhandled control path");
				}
			);
		}


		/*
		template<typename Cb, typename RCb, typename ExCb, typename Result = std::invoke_result_t<Cb>>
		//typename = std::enable_if_t<std::is_same<Result, std::invoke_result_t<RCb>>::value>,
		//typename = std::enable_if_t<std::is_same<Result, std::invoke_result_t<ExCb, std::exception_ptr>>::value >>
		auto then(Cb&& callback, RCb&& rejectCallback, ExCb&& exceptionCallback) -> decltype(Promise<Result>) {
			auto resolveBound = std::bind(&ReadyPromise<T>::_resolve, this, std::placeholders::_1);
			auto rejectBound = std::bind(&ReadyPromise<T>::_reject, this, std::placeholders::_1);
			auto rejectExBound = std::bind(&ReadyPromise<T>::_reject_ex, this, std::placeholders::_1);

			auto rb = std::bind(&callback, resolveBound);


			return Promise<T>::then(std::forward<Cb>(rb), std::forward<RCb>(rejectCallback), std::forward<ExCb>(exceptionCallback));
		}
		
		template<typename Cb, typename Result = std::invoke_result_t<Cb>>
		//typename = std::enable_if_t<std::is_same<Result, std::invoke_result_t<RCb>>::value>,
		//typename = std::enable_if_t<std::is_same<Result, std::invoke_result_t<ExCb, std::exception_ptr>>::value >>
		Promise<Result> then(Cb&& callback) {
			auto resolveBound = std::bind(&ReadyPromise<T>::_resolve, this, std::placeholders::_1);
			//auto rejectBound = std::bind(&ReadyPromise<T>::_reject, this, std::placeholders::_1);
			//auto rejectExBound = std::bind(&ReadyPromise<T>::_reject_ex, this, std::placeholders::_1);
			
			//auto rb = std::bind(&callback, resolveBound);


			return Promise<T>::then(std::forward<Cb>(callback));
		}*/

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