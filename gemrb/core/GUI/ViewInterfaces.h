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

template<typename T>
class ActionResponder {
public:
	using Action = uint32_t;

	class Responder {
		using ResponderCallback = Callback<void, T>;
		ResponderCallback callback = nullptr;

	public:
		Responder() noexcept = default;

		Responder(ResponderCallback cb)
		{
			callback = std::move(cb);
		}

		template<typename F,
			 // SFINE magic to prevent std::forward infinite recursion
			 std::enable_if_t<
				 !std::is_same<std::decay_t<F>, Responder>::value, bool> = false>
		Responder(F&& func)
		{
			callback = std::forward<F>(func);
		}

		template<typename F>
		// SFINE magic to disambiguate the multiple assignment operators
		auto operator=(F&& func) -> std::enable_if_t<
						 !std::is_same<std::decay_t<F>, Responder>::value,
						 Responder&>
		{
			callback = std::forward<F>(func);
			return *this;
		}

		Responder& operator=(ResponderCallback cb)
		{
			callback = std::move(cb);
			return *this;
		}

		void operator()(T responder) const
		{
			assert(responder->responderStack.empty() || responder->responderStack.back() != this);
			responder->responderStack.push_back(this);
			callback(responder);
			responder->responderStack.pop_back();
		}

		operator bool() const { return bool(callback); }
	};

	std::vector<const Responder*> responderStack;

	class ActionKey {
		uint32_t key;

	public:
		ActionKey(uint32_t val)
			: key(val) {}
		uint32_t Value() const { return key; }

		bool operator<(const ActionKey& ak) const
		{
			return key < ak.key;
		}

		bool operator==(const ActionKey& ak) const
		{
			return key == ak.key;
		}
	};

public:
	bool IsExecutingResponseHandler() const { return !responderStack.empty(); }

	virtual void SetAction(Responder handler, const ActionKey& key) = 0;
	virtual bool PerformAction(const ActionKey& action) = 0;
	virtual bool SupportsAction(const ActionKey& action) = 0;

	virtual ~ActionResponder()
	{
		assert(responderStack.empty());
	}
};

#endif /* ViewInterfaces_h */
