#pragma once
#ifndef _PROMISE_INCLUDED
#define _PROMISE_INCLUDED

#include "./base.h"

namespace pro
{
	template<typename T>
	class promise : public detail::_promise_base<T> {
	public:
		template<typename Function, typename... Args>
		promise(Function&& fun, Args&&... args) :
			detail::_promise_base<T>(std::forward<Function>(fun), std::forward<Args>(args)...) {
		}

		promise(const std::function<void(std::promise<T>)>& fun) :
			detail::_promise_base<T>(std::forward<std::function<void(std::promise<T>)>(fun)) {
		}
		
		template<typename Cb, typename RCb, typename ExCb, typename Result = std::invoke_result_t<Cb, T>,
		typename = std::enable_if_t<std::is_same<Result, std::invoke_result_t<RCb, T>>::value>,
		typename = std::enable_if_t<std::is_same<Result, std::invoke_result_t<ExCb, std::exception_ptr>>::value >>
		promise<Result> then(Cb&& callback, RCb&& rejectCallback, ExCb&& exceptionCallback) {
			return promise<Result>(
				[future = std::move(this->future), callback, rejectCallback, exceptionCallback]() mutable {
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
					else throw std::logic_error("promise<T>.then(cb,rcb,ecb) unhandled control path");
				}
			);
		}

		template<typename Cb, typename RCb, typename Result = std::invoke_result_t<Cb, T>,
		typename = std::enable_if_t<std::is_same<Result, std::invoke_result_t<RCb, T>>::value>>
		promise<Result> then(Cb&& callback, RCb&& rejectCallback) {
			return promise<Result>(
				[future = std::move(this->future), callback, rejectCallback]() mutable {
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
						std::rethrow_exception(eptr);
					}
					else throw std::logic_error("promise<T>.then(cb,rcb) unhandled control path");
				}
			);
		}

		template<typename Cb, typename Result = std::invoke_result_t<Cb, T>>
		promise<Result> then(Cb&& callback) {
			return promise<Result>(
				[future = std::move(this->future), callback]() mutable {
					try {
						return callback(future.get());
					}
					catch (...) {
						std::rethrow_exception(std::current_exception());
					}
				}
			);
		}

		template<typename RCb, typename ExCb, typename Result = std::invoke_result_t<RCb, T>,
		typename = std::enable_if_t<std::is_same<Result, std::invoke_result_t<ExCb, std::exception_ptr>>::value>>
		promise<Result> fail(RCb&& rejectCallback, ExCb&& exceptionCallback) {
			return promise<Result>(
				[future = std::move(this->future), rejectCallback, exceptionCallback]() mutable {
					try {
						future.get();
					}
					catch (T& ex) {
						return rejectCallback(std::move(ex));
					}
					catch (...) {
						return exceptionCallback(std::current_exception());
					}

					throw std::logic_error("promise<T>.fail unhandled control path");
				}
			);
		}

		template<typename ExCb, typename Result = std::invoke_result_t<ExCb, std::exception_ptr>>
		promise<Result> fail(ExCb&& exceptionCallback) {
			return promise<Result>(
				[future = std::move(this->future), exceptionCallback]() mutable {
				try {
					future.get();
				}
				catch (T& ex) {
					return exceptionCallback(std::make_exception_ptr(ex));
				}
				catch (...) {
					return exceptionCallback(std::current_exception());
				}

				throw std::logic_error("promise<T>.fail unhandled control path");
			});
		}
	};

	template<>
	class promise<void> : public detail::_promise_base<void> {
	public:
		template<typename Function, typename... Args>
		promise(Function&& fun, Args&&... args) :
			_promise_base(std::forward<Function>(fun), std::forward<Args>(args)...) {
		}

		template<typename Cb, typename RCb, typename ExCb, typename Result = std::invoke_result_t<Cb>,
		typename = std::enable_if_t<std::is_same<Result, std::invoke_result_t<RCb>>::value>,
		typename = std::enable_if_t<std::is_same<Result, std::invoke_result_t<ExCb, std::exception_ptr>>::value >>
		promise<Result> then(Cb&& callback, RCb&& rejectCallback, ExCb&& exceptionCallback) {
			return promise<Result>(
				[future = std::move(this->future), callback, rejectCallback, exceptionCallback]() mutable {
					std::exception_ptr eptr;
					try {
						future.get();

						try {
							return callback();
						}
						catch (...) {
							eptr = std::current_exception();
						}
					}
					catch (std::exception &ex) {
						return exceptionCallback(std::make_exception_ptr(ex));
					}
					catch (...) {
						return rejectCallback();
					}

					if (eptr) {
						return exceptionCallback(eptr);
					}
					else throw std::logic_error("promise<void>.then(cb,rcb,ecb) unhandled control path");
				}
			);
		}
		
		template<typename Cb, typename RCb, typename Result = std::invoke_result_t<Cb>,
		typename = std::enable_if_t<std::is_same<Result, std::invoke_result_t<RCb>>::value>>
		promise<Result> then(Cb&& callback, RCb&& rejectCallback) {
			return promise<Result>(
				[future = std::move(this->future), callback, rejectCallback]() mutable {
				std::exception_ptr eptr;
				try {
					future.get();

					try {
						return callback();
					}
					catch (...) {
						eptr = std::current_exception();
					}
				}
				catch (...) {
					return rejectCallback();
				}

				if (eptr) {
					std::rethrow_exception(eptr);
				}
				else throw std::logic_error("promise<void>.then(cb,rcb) unhandled control path");
			});
		}

		template<typename Cb, typename Result = std::invoke_result_t<Cb>>
		promise<Result> then(Cb&& callback) {
			return promise<Result>(
				[future = std::move(this->future), callback]() mutable {
				try {
					future.get();
					return callback();
				}
				catch (...) {
					std::rethrow_exception(std::current_exception());
				}
			});
		}

		template<typename ExCb, typename Result = std::invoke_result_t<ExCb, std::exception_ptr>>
		promise<Result> fail(ExCb&& exceptionCallback) {
			return promise<Result>(
				[future = std::move(this->future), exceptionCallback]() mutable {
				try {
					future.get();
				}
				catch (...) {
					return exceptionCallback(std::current_exception());
				}
			});
		}
	};

	template<typename T, typename Function, typename... Args,
		typename = std::enable_if_t<std::is_invocable_r_v<T, Function, Args...>>>
	constexpr promise<T> make_promise(Function&& fun, Args&&... args) {
		return promise<T>(std::forward<Function>(fun), std::forward<Args>(args)...);
	}

	template<typename T, typename = std::enable_if_t<!std::is_same<T, void>::value, bool>>
	constexpr promise<T> make_rejected_promise(T rejection_value) {
		return promise<T>(std::make_exception_ptr(std::move(rejection_value)));
	}
}

#endif //_PROMISE_INCLUDED