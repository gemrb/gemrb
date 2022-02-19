/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2017 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef ViewInterfaces_h
#define ViewInterfaces_h

class GEM_EXPORT Scrollable {
public:
	virtual void ScrollDelta(const Point& p) = 0;
	virtual void ScrollTo(const Point& p) = 0;
	
	virtual ~Scrollable() noexcept = default;
};

template <typename T>
class ActionResponder {
public:
	using Action = uint32_t;
	
	const class Responder {
		using ResponderCallback = Callback<void, T>;
		ResponderCallback callback = nullptr;

	public:
		Responder() noexcept = default;
		
		Responder(ResponderCallback cb){
			callback = std::move(cb);
		}
		
		template <typename F,
			// SFINE magic to prevent std::forward infinite recursion
			typename std::enable_if<
			   !std::is_same<typename std::decay<F>::type, Responder>::value, bool>::type = false
		>
		Responder(F&& func) {
			callback = std::forward<F>(func);
		}
		
		template <typename F>
		// SFINE magic to disambiguate the multiple assignment operators
		auto operator=(F&& func) -> typename std::enable_if<
		! std::is_same<typename std::decay<F>::type, Responder>::value,
		Responder&
		>::type {
			callback = std::forward<F>(func);
			return *this;
		}
		
		Responder& operator=(ResponderCallback cb) {
			callback = std::move(cb);
			return *this;
		}

		void operator()(T responder) const {
			assert(responder->executingResponseHandler == nullptr);
			responder->executingResponseHandler = this;
			callback(responder);
			responder->executingResponseHandler = nullptr;
		}
		
		operator bool() const { return bool(callback); }
	}* executingResponseHandler = nullptr;

	class ActionKey {
		uint32_t key;
	public:
		ActionKey(uint32_t val) : key(val) {}
		uint32_t Value() const { return key; }
		
		bool operator< (const ActionKey& ak) const {
			return key < ak.key;
		}
		
		bool operator==(const ActionKey& ak) const {
			return key == ak.key;
		}
	};
	
public:
	bool IsExecutingResponseHandler() const { return executingResponseHandler; }

	virtual void SetAction(Responder handler, const ActionKey& key) = 0;
	virtual bool PerformAction(const ActionKey& action) = 0;
	virtual bool SupportsAction(const ActionKey& action) = 0;
	
	virtual ~ActionResponder() {
		assert(executingResponseHandler == nullptr);
	}
};

#endif /* ViewInterfaces_h */
