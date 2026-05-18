/**
 * @file llnotifications.h
 * @brief Non-UI manager and support for keeping a prioritized list of
 * notifications
 * @author Q (with assistance from Richard and Coco)
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 *
 * Copyright (c) 2008-2009, Linden Research, Inc.
 * Copyright (c) 2025, Henri Beauchamp.
 *
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution.
 *
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#pragma once

// This system is intended to provide a mechanism for adding notifications to
// one of an arbitrary set of event channels.
//
// Every notification has (see code for full list):
//  - a textual name, which is used to look up its template in the XML files
//  - a payload, which is a block of LLSD.
//  - a channel, which is normally extracted from the XML files but can be
//	  overridden.
//  - a timestamp, used to order the notifications.
//  - expiration time -- if nonzero, specifies a time after which the
//	  notification will no longer be valid.
//  - a callback name and a couple of status bits related to callbacks (see
//	  below).
//
// There is a management class called LLNotifications.
// The class maintains a collection of all of the notifications received or
// processed during this session, and also manages the persistence of those
// notifications that must be persisted.
//
// We also have Channels. A channel is a view on a collection of notifications;
// The collection is defined by a filter function that controls which
// notifications are in the channel, and its ordering is controlled by a
// comparator.
//
// There is a hierarchy of channels; notifications flow down from the
// management class (LLNotifications, which itself inherits from the channel
// base class) to the individual channels. Any change to notifications (add,
// delete, modify) is automatically propagated through the channel hierarchy.
//
// We provide methods for adding a new notification, for removing one, and for
// managing channels. Channels are relatively cheap to construct and maintain,
// so in general, human interfaces should use channels to select and manage
// their lists of notifications.
//
// We also maintain a collection of templates that are loaded from the XML file
// of template translations. The system supports substitution of named
// variables from the payload into the XML file.
//
// By default, only the "unknown message" template is built into the system.
// It is not an error to add a notification that's not found in the template
// system, but it is logged.

#include <functional>
#include <type_traits>
#include <vector>

#include "hbfastmap.h"
#include "llinstancetracker.h"
#include "llsingleton.h"
#include "llstring.h"
#include "llui.h"
#include "llxmlnode.h"

typedef std::function<void(const LLSD&, const LLSD&)> notification_cb_t;

///////////////////////////////////////////////////////////////////////////////
// LLNotificationFunctorRegistry
//
// This used to be a LLFunctorRegistry template in its own llfunctorregistry.h
// header, but it was only used and included here, and only used by one functor
// template type (notification_cb_t), so I turned it into a proper class to
// keep things simpler. HB
//
// LLNotificationFunctorRegistry maintains a collection of named functors in a
// singleton. We want to be able to persist notifications with their callbacks
// across restarts of the viewer; we could not store functors that way. Using
// this registry, systems that require a functor to be maintained long term can
// register it at system startup, and then pass in the functor by name.

class LLNotificationFunctorRegistry
:	public LLSingleton<LLNotificationFunctorRegistry>
{
	friend class LLSingleton<LLNotificationFunctorRegistry>;

protected:
	LOG_CLASS(LLNotificationFunctorRegistry);

	LLNotificationFunctorRegistry() = default;

public:
	void registerFunctor(const std::string& name, notification_cb_t f);
	void unregisterFunctor(const std::string& name);
	notification_cb_t getFunctor(const std::string& name) const;

private:
	static void doNothing(const LLSD&, const LLSD& payload);

private:
	typedef flat_hmap<std::string, notification_cb_t> map_t;
	map_t mMap;
};

class LLNotificationFunctorRegistration
{
public:
	LLNotificationFunctorRegistration(const std::string& name,
									  notification_cb_t functor);
};

///////////////////////////////////////////////////////////////////////////////

class LLNotification;
typedef std::shared_ptr<LLNotification> LLNotificationPtr;

// A boost::signals2 Combiner that stops the first time a handler returns true
// We need this because we want to have our handlers return bool, so that we
// have the option to cause a handler to stop further processing. The default
// handler fails when the signal returns a value but has no slots.
struct LLStopWhenNotificationHandled
{
	typedef bool result_type;

	template<typename INPUT_ITERATOR>
	result_type operator()(INPUT_ITERATOR first, INPUT_ITERATOR last) const
	{
		for (INPUT_ITERATOR si = first; si != last; ++si)
		{
			if (*si)
			{
				return true;
			}
		}
		return false;
	}
};

typedef enum e_notification_priority
{
	NOTIFICATION_PRIORITY_UNSPECIFIED,
	NOTIFICATION_PRIORITY_LOW,
	NOTIFICATION_PRIORITY_NORMAL,
	NOTIFICATION_PRIORITY_HIGH,
	NOTIFICATION_PRIORITY_CRITICAL
} ENotificationPriority;

// Context data that can be looked up via a notification's payload by the
// display logic; derive from this class to implement specific contexts.
class LLNotificationContext : public LLInstanceTracker<LLNotificationContext, LLUUID>
{
public:
	LLNotificationContext()
	:	LLInstanceTracker<LLNotificationContext, LLUUID>(LLUUID::generateNewID())
	{
	}

	LL_INLINE LLSD asLLSD() const					{ return getKey(); }
};

// Contains notification form data, such as buttons and text fields along with
// manipulator functions
class LLNotificationForm
{
protected:
	LOG_CLASS(LLNotificationForm);

public:
	typedef enum e_ignore_type
	{
		IGNORE_NO,
		IGNORE_WITH_DEFAULT_RESPONSE,
		IGNORE_WITH_LAST_RESPONSE,
		IGNORE_SHOW_AGAIN
	} EIgnoreType;

	LLNotificationForm();
	LLNotificationForm(const LLSD& sd);
	LLNotificationForm(const std::string& name, const LLXMLNodePtr xml_node);

	LL_INLINE LLSD asLLSD() const					{ return mFormData; }

	LL_INLINE S32 getNumElements()					{ return mFormData.size(); }
	LL_INLINE LLSD getElement(U32 i)				{ return mFormData.get(i); }
	LLSD getElement(const std::string& element_name);
	bool hasElement(const std::string& element_name);
	void addElement(const std::string& type, const std::string& name,
					const LLSD& value = LLSD());
	void formatElements(const LLSD& substitutions);
	// Appends form elements from another form serialized as LLSD
	void append(const LLSD& sub_form);

	std::string getDefaultOption();

	LL_INLINE EIgnoreType getIgnoreType()			{ return mIgnore; }
	LL_INLINE std::string getIgnoreMessage()		{ return mIgnoreMsg; }

private:
	LLSD		mFormData;
	EIgnoreType	mIgnore;
	std::string	mIgnoreMsg;
};

typedef std::shared_ptr<LLNotificationForm> LLNotificationFormPtr;

// This is the class of object read from the XML file (notifications.xml, from
// the appropriate local language directory).
struct LLNotificationTemplate
{
	LLNotificationTemplate();
	// the name of the notification -- the key used to identify it Ideally, the
	// key should follow variable naming rules (no spaces or punctuation).
	std::string mName;
	// The type of the notification used to control which queue it's stored in
	std::string mType;
	// The text used to display the notification. Replaceable parameters are
	// enclosed in square brackets like this [].
	std::string mMessage;
	// The label for the notification; used for certain classes of notification
	// (those with a window and a window title).
	// Also used when a notification pops up underneath the current one.
	// Replaceable parameters can be used in the label.
	std::string mLabel;
	// The name of the icon image. This should include an extension.
	std::string mIcon;
	// This is the Highlander bit -- "There Can Be Only One"
	// An outstanding notification with this bit set is updated by an incoming
	// notification with the same name, rather than creating a new entry in the
	// queue (used for things like progress indications, or repeating warnings
	// like "the grid is going down in N minutes")
	bool mUnique;
	// if we want to be unique only if a certain part of the payload is
	// constant specify the field names for the payload. The notification will
	// only be combined if all of the fields named in the context are identical
	// in the new and the old notification; otherwise, the notification will be
	// duplicated. This is to support suppressing duplicate offers from the
	// same sender but still differentiating different offers. Example:
	// Invitation to conference chat.
	std::vector<std::string> mUniqueContext;
	// If this notification expires automatically, this value will be nonzero,
	// and indicates the number of seconds for which the notification will be
	// valid (a teleport offer, for example, might be valid for 300 seconds).
	U32 mExpireSeconds;
	// if the offer expires, one of the options is chosen automatically
	// based on its "value" parameter. This controls which one.
	// If expireSeconds is specified, expireOption should also be specified.
	U32 mExpireOption;
	// if the notification contains a url, it's stored here (and replaced
	// into the message where [_URL] is found)
	std::string mURL;
	// if there's a URL in the message, this controls which option visits
	// that URL. Obsolete this and eliminate the buttons for affected
	// messages when we allow clickable URLs in the UI
	U32 mURLOption;
	// does this notification persist across sessions? if so, it will be
	// serialized to disk on first receipt and read on startup
	bool mPersist;
	// This is the name of the default functor, if present, to be
	// used for the notification's callback. It is optional, and used only if
	// the notification is constructed without an identified functor.
	std::string mDefaultFunctor;
	// The form data associated with a given notification (buttons, text boxes,
	// etc)
	LLNotificationFormPtr mForm;
	// default priority for notifications of this type
	ENotificationPriority mPriority;
	// UUID of the audio file to be played when this notification arrives this
	// is loaded as a name, but looked up to get the UUID upon template load.
	// If null, it was not specified.
	LLUUID mSoundEffect;
};

// We want to keep a map of these by name, and it's best to manage them with
// smart pointers
typedef std::shared_ptr<LLNotificationTemplate> LLNotificationTemplatePtr;

// The class that expresses the details of a notification.
// The enable_shared_from_this flag ensures that if we construct a smart
// pointer from a notification, we will always get the same shared pointer.
class LLNotification : public std::enable_shared_from_this<LLNotification>
{
	friend class LLNotifications;

protected:
	LOG_CLASS(LLNotification);

public:
	// We make this non-copyable because we want to manage these through
	// LLNotificationPtr, and only ever create one instance of any given
	// notification.
	LLNotification(const LLNotification&) = delete;
	LLNotification& operator=(const LLNotification&) = delete;

	// Parameter object used to instantiate a new notification
	class Params : public LLParamBlock<Params>
	{
		friend class LLNotification;

	public:
		LL_INLINE Params(const std::string& _name)
		:	name(_name),
			mTemporaryResponder(false),
			functor_name(_name),
			priority(NOTIFICATION_PRIORITY_UNSPECIFIED),
			timestamp(LLDate::now())
		{
		}

		// Pseudo-param
		Params& functor(notification_cb_t fn);

	public:
		LLMandatoryParam<std::string>			name;

		// Optional
		LLOptionalParam<LLSD>					substitutions;
		LLOptionalParam<LLSD>					payload;
		LLOptionalParam<ENotificationPriority>	priority;
		LLOptionalParam<LLSD>					form_elements;
		LLOptionalParam<LLDate>					timestamp;
		LLOptionalParam<LLNotificationContext*>	context;
		LLOptionalParam<std::string>			functor_name;

	private:
		bool mTemporaryResponder;
	};

	// Constructor from a saved notification
	LLNotification(const LLSD& sd);

	// This is a string formatter for substituting into the message directly
	// from LLSD.
	static std::string format(const std::string& text, const LLSD& substitutions);

	void setfn_t(const std::string& functor_name);

	typedef enum e_response_template_type
	{
		WITHOUT_DEFAULT_BUTTON,
		WITH_DEFAULT_BUTTON
	} EResponseTemplateType;

	// Returns response LLSD filled in with default form contents and
	// (optionally) the default button selected.
	LLSD getResponseTemplate(EResponseTemplateType type = WITHOUT_DEFAULT_BUTTON);

	// Returns index of first button with value==true; usually this the button
	// the user clicked on. Returns -1 if no button clicked (e.g. form has not
	// been displayed).
	static S32 getSelectedOption(const LLSD& notification, const LLSD& response);
	// Returns name of first button with value==true
	static std::string getSelectedOptionName(const LLSD& notification);

	// After someone responds to a notification (usually by clicking a button,
	// but sometimes by filling out a little form and THEN clicking a button),
	// the result of the response (the name and value of the button clicked,
	// plus any other data) should be packaged up as LLSD, then passed as a
	// parameter to the notification's respond() method here. This will look up
	// and call the appropriate responder.
	//
	// response is notification serialized as LLSD:
	// ["name"] = notification name
	// ["form"] = LLSD tree that includes form description and any prefilled form data
	// ["response"] = form data filled in by user
	// (including, but not limited to which button they clicked on)
	// ["payload"] = transaction specific data, such as ["source_id"] (originator of notification),
	//				["item_id"] (attached inventory item), etc.
	// ["substitutions"] = string substitutions used to generate notification message
	// from the template
	// ["time"] = time at which notification was generated;
	// ["expiry"] = time at which notification expires;
	// ["responseFunctor"] = name of registered functor that handles responses to notification;
	LLSD asLLSD();

	void respond(const LLSD& sd, bool save = true);

	LL_INLINE void setIgnored(bool ignore)			{ mIgnored = ignore; }

	LL_INLINE bool isCancelled() const				{ return mCancelled; }

	LL_INLINE bool isRespondedTo() const			{ return mRespondedTo; }

	LL_INLINE bool isIgnored() const				{ return mIgnored; }

	LL_INLINE const std::string& getName() const
	{
		return mTemplatep->mName;
	}

	LL_INLINE const LLUUID& getID() const			{ return mId; }

	LL_INLINE const LLSD& getPayload() const		{ return mPayload; }

	LL_INLINE const LLSD& getSubstitutions() const	{ return mSubstitutions; }

	LL_INLINE const LLDate& getDate() const			{ return mTimestamp; }

	LL_INLINE const std::string& getType() const
	{
		return mTemplatep ? mTemplatep->mType : LLStringUtil::null;
	}

	// These use real-time formatting with mTemplatep message/label strings and
	// the current mSubstitutions.
	std::string getMessage() const;
	std::string getLabel() const;

	LL_INLINE const std::string& getURL() const
	{
		return mTemplatep ? mTemplatep->mURL : LLStringUtil::null;
	}

	LL_INLINE S32 getURLOption() const
	{
		return mTemplatep ? mTemplatep->mURLOption : -1;
	}

	LL_INLINE const LLNotificationFormPtr getForm()	{ return mForm; }

	LL_INLINE const LLDate& getExpiration() const	{ return mExpiresAt; }

	LL_INLINE ENotificationPriority getPriority() const
	{
		return mPriority;
	}

	// Comparing two notifications normally means comparing them by UUID (so we
	// can look them up quickly this way)
	LL_INLINE bool operator<(const LLNotification& rhs) const
	{
		return mId < rhs.mId;
	}

	LL_INLINE bool operator==(const LLNotification& rhs) const
	{
		return mId == rhs.mId;
	}

	LL_INLINE bool operator!=(const LLNotification& rhs) const
	{
		return mId != rhs.mId;
	}

	LL_INLINE bool isSameObjectAs(const LLNotification* rhs) const
	{
		return this == rhs;
	}

	// This object has been updated, so tell all our clients
	void update();

	void updateFrom(LLNotificationPtr other);

	// A fuzzy equals comparator. Returns true only if both notifications have
	// the same template and flagged as unique (there can be only one of these)
	// OR all required payload fields of each also exist in the other.
	bool isEquivalentTo(LLNotificationPtr that) const;

	// If the current time is greater than the expiration, the notification is
	// expired
	LL_INLINE bool isExpired() const
	{
		if (mExpiresAt.secondsSinceEpoch() == 0)
		{
			return false;
		}

		LLDate rightnow = LLDate::now();
		return rightnow > mExpiresAt;
	}

	std::string summarize() const;

	LL_INLINE bool hasUniquenessConstraints() const
	{
		return mTemplatep && mTemplatep->mUnique;
	}

	virtual ~LLNotification() = default;

private:
	void init(const std::string& template_name, const LLSD& form_elements);

	LLNotification(const Params& p);

	// This is just for making it easy to look things up in a set organized by
	// UUID: DO NOT USE IT for anything real !
	LL_INLINE LLNotification(LLUUID uuid)
	:	mId(uuid)
	{
	}

	LL_INLINE void cancel()							{ mCancelled = true; }

	bool payloadContainsAll(const std::vector<std::string>& required_fields) const;

private:
	LLUUID						mId;

	ENotificationPriority		mPriority;

	LLNotificationFormPtr		mForm;

	// a reference to the template
	LLNotificationTemplatePtr	mTemplatep;

	LLDate						mTimestamp;
	LLDate						mExpiresAt;

	LLSD						mPayload;
	LLSD						mSubstitutions;

	// We want to be able to store and reload notifications so that they can
	// survive a shutdown/restart of the client. So we can't simply pass in
	// callbacks; we have to specify a callback mechanism that can be used by
	// name rather than by some arbitrary pointer -- and then people have to
	// initialize callbacks in some useful location.
	// So we use LLNotificationFunctorRegistry to manage them.
	std::string					mfn_tName;

	bool						mCancelled;
	bool						mIgnored;

 	// Once the notification has been responded to, this becomes true
	bool						mRespondedTo;

	// In cases where we want to specify an explict, non-persisted callback,
	// we store that in the callback registry under a dynamically generated
	// key, and store the key in the notification, so we can still look it up
	// using the same mechanism.
	bool						mTemporaryResponder;
};

std::ostream& operator<<(std::ostream& s, const LLNotification& notification);

namespace LLNotificationFilters
{
	// A sample filter
	bool includeEverything(LLNotificationPtr p);

	typedef enum e_comparison
	{
		EQUAL,
		LESS,
		GREATER,
		LESS_EQUAL,
		GREATER_EQUAL
	} EComparison;

	// Generic filter functor that takes method or member variable reference
	template<typename T>
	struct filterBy
	{
		typedef std::function<T(LLNotificationPtr)> field_t;
		typedef typename std::remove_reference<T>::type value_t;

		filterBy(field_t field, value_t value, EComparison comparison = EQUAL)
		:	mField(field),
			mFilterValue(value),
			mComparison(comparison)
		{
		}

		bool operator()(LLNotificationPtr p)
		{
			switch (mComparison)
			{
				case EQUAL: 		return mField(p) == mFilterValue;
				case LESS:			return mField(p) < mFilterValue;
				case GREATER:		return mField(p) > mFilterValue;
				case LESS_EQUAL:	return mField(p) <= mFilterValue;
				case GREATER_EQUAL:	return mField(p) >= mFilterValue;
				default:			return false;
			}
		}

		field_t			mField;
		value_t			mFilterValue;
		EComparison		mComparison;
	};
};

namespace LLNotificationComparators
{
	typedef enum e_direction { ORDER_DECREASING, ORDER_INCREASING } EDirection;

	// Generic order functor that takes method or member variable reference
	template<typename T>
	struct orderBy
	{
		typedef std::function<T(LLNotificationPtr)> field_t;
		orderBy(field_t field, EDirection = ORDER_INCREASING)
		:	mField(field)
		{
		}

		LL_INLINE bool operator()(LLNotificationPtr lhs, LLNotificationPtr rhs)
		{
			if (mDirection == ORDER_DECREASING)
			{
				return mField(lhs) > mField(rhs);
			}
			return mField(lhs) < mField(rhs);
		}

		field_t		mField;
		EDirection	mDirection = ORDER_INCREASING;
	};

	struct orderByUUID : public orderBy<const LLUUID&>
	{
		orderByUUID(EDirection direction = ORDER_INCREASING)
		:	orderBy<const LLUUID&>(&LLNotification::getID, direction)
		{
		}
	};

	struct orderByDate : public orderBy<const LLDate&>
	{
		orderByDate(EDirection direction = ORDER_INCREASING)
		:	orderBy<const LLDate&>(&LLNotification::getDate, direction)
		{
		}
	};
};

// Abstract base class (interface) for a channel; also used for the master container.
// This lets us arrange channels into a call hierarchy.
//
// We maintain a hierarchy of notification channels; events are always started
// at the top and propagated through the hierarchy only if they pass a filter.
// Any channel can be created with a parent. A null parent (empty string) means
// it's tied to the root of the tree (the LLNotifications class itself).
// The default hierarchy looks like this:
//
// LLNotifications -+- Expiration -+- Mute -+- Ignore -+- Visible -+- History
//																   +-- Alerts
//																   +-- Notifications
//
// In general, new channels that want to only see notifications that pass through
// all of the built-in tests should attach to the "Visible" channel
//
class LLNotificationChannelBase : public boost::signals2::trackable
{
protected:
	LOG_CLASS(LLNotificationChannelBase);

public:
	typedef std::function<bool(LLNotificationPtr)> filter_t;
	typedef std::function<bool(LLNotificationPtr,
							   LLNotificationPtr)> comparator_t;
	typedef std::set<LLNotificationPtr, comparator_t> set_t;
	typedef std::multimap<std::string, LLNotificationPtr, std::less<> > map_t;
	// We want to have a standard signature for all signals; this way, we can
	// easily document a protocol for communicating across dlls and into
	// scripting languages someday. We want to return a bool to indicate
	// whether the signal has been handled and should NOT be passed on to other
	// listeners. Return true to stop further handling of the signal, and false
	// to continue. We take an LLSD because this way the contents of the signal
	// are independentof the API used to communicate.
	typedef boost::signals2::signal<bool(const LLSD&),
									LLStopWhenNotificationHandled> signal_t;

	LL_INLINE LLNotificationChannelBase(filter_t filter, comparator_t comp)
	:	mFilter(filter),
		mItems(comp)
	{
	}

	virtual ~LLNotificationChannelBase() = default;
	// You can also connect to a Channel, so you can be notified of
	// changes to this channel
	virtual void connectChanged(const signal_t::slot_type& slot);
	virtual void connectPassedFilter(const signal_t::slot_type& slot);
	virtual void connectFailedFilter(const signal_t::slot_type& slot);

	// Use this when items change or to add a new one
	bool updateItem(const LLSD& payload);

	LL_INLINE const filter_t& getFilter() const				{ return mFilter; }

protected:
	// These are action methods that subclasses can override to take action on
	// specific types of changes; the management of the mItems list is still
	// handled by the generic handler.
	LL_INLINE virtual void onLoad(LLNotificationPtr p)		{}
	LL_INLINE virtual void onAdd(LLNotificationPtr p)		{}
	LL_INLINE virtual void onDelete(LLNotificationPtr p)	{}
	LL_INLINE virtual void onChange(LLNotificationPtr p)	{}

	// Use a method named differently than updateItem() to avoid signature
	// detection issues with std::bind and its placeholders (it fails to
	// resolve overloads with different signatures, unlike boost::bind). HB
	bool updateItemInternal(const LLSD& payload, LLNotificationPtr notifp);

protected:
	filter_t	mFilter;
	set_t		mItems;
	signal_t	mChanged;
	signal_t	mPassedFilter;
	signal_t	mFailedFilter;
};

// Type of the pointers that we are going to manage in the NotificationQueue
// system. The shared_ptr model will ensure the notification channel will
// self-destroy on LLNotifications destruction.
class LLNotificationChannel;
typedef std::shared_ptr<LLNotificationChannel> LLNotificationChannelPtr;

// Manages a list of notifications.
// Note: LLNotificationChannel is self-registering. The *correct* way to create
// one is to do something like:
//		LLNotificationChannel::buildChannel("name", "parent"...);
// This returns an LLNotificationChannelPtr, which you can store, or you can
// then retrieve the channel by using the registry:
//		gNotifications.getChannel("name")...
class LLNotificationChannel : public LLNotificationChannelBase
{
protected:
	LOG_CLASS(LLNotificationChannel);

public:
	// We make this non-copyable because if this is ever copied around, we
	// might find ourselves with multiple copies of a queue with notifications
	// being added to different nonequivalent copies. We instead use a map of
	// shared_ptr to manage it.
	LLNotificationChannel(const LLNotificationChannel&) = delete;
	LLNotificationChannel& operator=(const LLNotificationChannel&) = delete;

	LL_INLINE std::string getName() const			{ return mName; }
	LL_INLINE std::string getParentChannelName()	{ return mParent; }

	LL_INLINE bool isEmpty() const					{ return mItems.empty(); }

	LL_INLINE set_t::iterator begin()				{ return mItems.begin(); }
	LL_INLINE set_t::iterator end()					{ return mItems.end(); }

	// Channels have a comparator to control sort order; the default sorts by
	// arrival date
	void setComparator(comparator_t comparator);

	std::string summarize();

	// Factory method for constructing these channels; since they are
	// self-registering, we want to make sure that you cannot use new to make
	// them.
	static LLNotificationChannelPtr buildChannel(const std::string& name,
												 const std::string& parent,
												 filter_t filter =
													LLNotificationFilters::includeEverything,
												 comparator_t comparator =
													LLNotificationComparators::orderByUUID());

protected:
	// Notification Channels have a filter, which determines which
	// notifications will be added to this channel. Channel filters cannot
	// change. Channels have a protected constructor so you can't make smart
	// pointers that don't come from our internal reference; call
	// NotificationChannel::build(args)
	LLNotificationChannel(const std::string& name, const std::string& parent,
						  filter_t filter, comparator_t comparator);

private:
	std::string		mName;
	std::string		mParent;
	comparator_t	mComparator;
};

class LLNotifications : public LLNotificationChannelBase
{
protected:
	LOG_CLASS(LLNotifications);

public:
	LLNotifications();

	// Must be called once before notifications are used. Call done in the
	// constructor of LLViewerWindow. HB
	void initClass();

	// Load notification descriptions from file; OK to call more than once
	// because it will reload
	bool loadTemplates();
	LLXMLNodePtr checkForXMLTemplate(LLXMLNodePtr item);

	// We provide a collection of simple add notification functions so that
	// it's reasonable to create notifications in one line
	LLNotificationPtr add(const std::string& name,
						  const LLSD& substitutions = LLSD(),
						  const LLSD& payload = LLSD());
	LLNotificationPtr add(const std::string& name, const LLSD& substitutions,
						  const LLSD& payload,
						  const std::string& functor_name);
	LLNotificationPtr add(const std::string& name, const LLSD& substitutions,
						  const LLSD& payload, notification_cb_t functor);
	LLNotificationPtr add(const LLNotification::Params& p);

	void add(const LLNotificationPtr notifp);
	void cancel(LLNotificationPtr notifp);
	void update(const LLNotificationPtr notifp);

	LLNotificationPtr find(LLUUID uuid);

	// This is all stuff for managing the templates
	// Takes the template out
	LLNotificationTemplatePtr getTemplate(const std::string& name);

	// Gets the whole collection and fills up 'names' with a list of
	// notification names
	void getTemplateNames(strings_vec_t& names) const;

	typedef std::map<std::string, LLNotificationTemplatePtr,
					 std::less<> > template_map_t;

	LL_INLINE template_map_t::const_iterator templatesBegin()
	{
		return mTemplates.begin();
	}

	LL_INLINE template_map_t::const_iterator templatesEnd()
	{
		return mTemplates.end();
	}

	// Tests for existence
	LL_INLINE bool templateExists(const std::string& name)
	{
		return mTemplates.count(name) != 0;
	}

	// Useful if you are reloading the file
	LL_INLINE void clearTemplates()					{ mTemplates.clear(); }

	void forceResponse(const LLNotification::Params& params, S32 option);

	void createDefaultChannels();

	void addChannel(LLNotificationChannelPtr channelp);
	LLNotificationChannelPtr getChannel(const std::string& chan_name);

	const std::string& getGlobalString(const std::string& key) const;

private:
	void loadPersistentNotifications();

	bool expirationFilter(LLNotificationPtr notifp);
	bool nonExpirationFilter(LLNotificationPtr notifp);
	bool expirationHandler(const LLSD& payload);
	bool uniqueFilter(LLNotificationPtr notifp);
	bool uniqueHandler(const LLSD& payload);
	bool failedUniquenessTest(const LLSD& payload);

	// Puts your template in
	bool addTemplate(const std::string& name, LLNotificationTemplatePtr temp);

private:
	std::string			mFileName;
	template_map_t		mTemplates;

	typedef std::map<std::string, LLXMLNodePtr, std::less<> > xml_template_map_t;
	xml_template_map_t	mXmlTemplates;

	typedef std::map<std::string, LLNotificationChannelPtr> channel_map_t;
	channel_map_t		mChannels;

	map_t				mUniqueNotifications;
	strings_map_t		mGlobalStrings;
};

// Let's use a global instance instead of a costly LLSingleton... HB
extern LLNotifications gNotifications;
