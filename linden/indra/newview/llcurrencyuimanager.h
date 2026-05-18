/**
 * @file llcurrencyuimanager.h
 * @brief LLCurrencyUIManager class definition
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 *
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

class LLPanel;

// Manages the currency purchase portion of any dialog takes control of, and
// assumes repsonsibility for several fields:
// - 'currency_action': the text "Buy L$" before the entry field.
// - 'currency_amt': the line editor for the entry amount.
// - 'currency_est': the estimated cost from the web site.
class LLCurrencyUIManager
{
public:
	LLCurrencyUIManager(LLPanel& parent);
	virtual ~LLCurrencyUIManager();

	// The amount in L$ to purchase; setting it overwrites the user's entry
	// if no_estimate is true, then no web request is made.
	void setAmount(S32 amount, bool no_estimate = false);
	S32 getAmount();

	// Sets the gray message to show when zero
	void setZeroMessage(const std::string& message);

	// The amount in US$ * 100 (in otherwords, in cents).
	void setUSDEstimate(S32 amount);
	// Use set when you get this information from elsewhere.
	S32 getUSDEstimate();

	// The estimated cost in the user's local currency, for example,
	// "US$ 10.00" or "10.00 Euros".
	void setLocalEstimate(const std::string& local_est);
	std::string getLocalEstimate() const;

	// Call once after dialog is built, from postBuild()
	void prepare();

	// Updates all UI elements, if show is false, they are all set not visible.
	// Normally, this is done automatically, but you can force it. The show/
	// hidden state is remembered.
	void updateUI(bool show = true);

	// Call periodically, for example, from draw(). Returns true if the UI
	// needs to be updated
	bool process();

	// Call to initiate the purchase
	void buy(const std::string& buy_msg);

	bool inProcess();	// Is a transaction in process ?
	bool canCancel();	// Can we cancel it (by destructing this object) ?
	bool canBuy();		// Can the user choose to buy now ?
	bool buying();		// Are we in the process of buying ?
	bool bought();		// Did the buy() transaction complete successfully ?

	void clearError();
	bool hasError();
	std::string errorMessage();
	// Error information for the user, the URI may be blank the technical
	// error details will have already been logged
	std::string errorURI();

private:
	class Impl;
	Impl& impl;
};
