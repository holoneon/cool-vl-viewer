/**
 * @file llpluginmessage.h
 * @brief LLPluginMessage encapsulates the serialization/deserialization of messages passed to and from plugins.
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 *
 * Copyright (c) 2008-2009, Linden Research, Inc.
 * Copyright (c) 2009-2024, Henri Beauchamp.
 *
 * Second Life Viewer Source Code
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; version 2.1 of the License only.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA"
 * $/LicenseInfo$
 */

#pragma once

#include "llsd.h"

// LLPluginMessage encapsulates the serialization/deserialization of
// messages passed to and from plugins.
class LLPluginMessage final
{
protected:
	LOG_CLASS(LLPluginMessage);

public:
	LLPluginMessage() = default;
	LLPluginMessage(const LLPluginMessage& p);
	LLPluginMessage(const std::string& message_class,
					const std::string& message_name);

	// Resets the internal state
	void clear();

	// Sets the message class and name
	// Also has the side-effect of clearing any key/value pairs in the message.
	void setMessage(const std::string& message_class,
					const std::string& message_name);

	// Sets a key/value pair in the message
	void setValue(const std::string& key, const std::string& value);
	void setValueLLSD(const std::string& key, const LLSD& value);
	void setValueLLUUID(const std::string& key, const LLUUID& value);
	void setValueS32(const std::string& key, S32 value);
	void setValueU32(const std::string& key, U32 value);
	void setValueBoolean(const std::string& key, bool value);
	void setValueReal(const std::string& key, F64 value);
	void setValuePointer(const std::string& key, void* value);

	std::string getClass() const;
	std::string getName() const;

	// Returns true if the specified key exists in this message (useful for
	// optional parameters)
	bool hasValue(const std::string& key) const;

	// Gets the value of a particular key as a string. If the key does not
	// exist in the message, an empty string will be returned.
	std::string getValue(const std::string& key) const;

	// Gets the value of a particular key as LLSD. If the key does not exist in
	// the message, a null LLSD will be returned.
	LLSD getValueLLSD(const std::string& key) const;

	// Gets the value of a particular key as LLUUID. If the key does not exist
	// in the message, a null LLUUID will be returned.
	LLUUID getValueLLUUID(const std::string& key) const;

	// Gets the value of a key as a S32. If the value wasn't set as a S32,
	// behavior is undefined.
	S32 getValueS32(const std::string& key) const;

	// Gets the value of a key as a U32. Since there isn't an LLSD type for
	// this, we use a hexadecimal string instead.
	U32 getValueU32(const std::string& key) const;

	// Gets the value of a key as a Boolean.
	bool getValueBoolean(const std::string& key) const;

	// Gets the value of a key as a float.
	F64 getValueReal(const std::string& key) const;

	// Gets the value of a key as a pointer.
	void* getValuePointer(const std::string& key) const;

	LL_INLINE const LLSD& getParams() const		{ return mMessage["params"]; }

	// Flattens the message into a string
	std::string generate() const;

	// Parses an incoming message into component parts (this clears out any
	// existing state before starting the parse). Returns -1 on failure,
	// otherwise returns the number of key/value pairs in the message.
	S32 parse(const std::string& message);

private:
	LLSD mMessage;
};

// Listener for plugin messages.
class LLPluginMessageListener
{
public:
	LLPluginMessageListener() = default;
	virtual ~LLPluginMessageListener() = default;

	// Plugin receives message from plugin loader shell.
	virtual void receivePluginMessage(const LLPluginMessage& message) = 0;
};

// Dispatcher for plugin messages. Manages the set of plugin message listeners
// and distributes messages to plugin message listeners.
class LLPluginMessageDispatcher final
{
public:
	void addPluginMessageListener(LLPluginMessageListener*);
	void removePluginMessageListener(LLPluginMessageListener*);

protected:
	void dispatchPluginMessage(const LLPluginMessage& message);

protected:
	// A set of message listeners.
	typedef std::set<LLPluginMessageListener*> listener_set_t;
	// The set of message listeners.
	listener_set_t mListeners;
};
