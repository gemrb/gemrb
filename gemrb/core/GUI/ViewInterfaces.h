// SPDX-FileCopyrightText: 2017 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
