/**
 * @file llcurrencyuimanager.cpp
 * @brief LLCurrencyUIManager class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llcurrencyuimanager.h"

#include "llconfirmationmanager.h"
#include "lllineeditor.h"
#include "llpanel.h"
#include "lltextbox.h"
#include "llversionviewer.h"
#include "llxmlrpctransaction.h"

#include "llagent.h"
#include "llgridmanager.h"
#include "llviewercontrol.h"

// How long of a pause in typing a currency buy amount before an esimate is
// fetched from the server:
constexpr F64 CURRENCY_ESTIMATE_FREQUENCY = 2.0;

class LLCurrencyUIManager::Impl
{
public:
	Impl(LLPanel& dialog);
	~Impl();

	void updateCurrencyInfo();
	void finishCurrencyInfo();

	void startCurrencyBuy(const std::string& password);
	void finishCurrencyBuy();

	LL_INLINE void clearEstimate()
	{
		mUSDCurrencyEstimated = mLocalCurrencyEstimated = false;
		mUSDCurrencyEstimatedCost = 0;
		mLocalCurrencyEstimatedCost = "0";
	}

	LL_INLINE bool hasEstimate() const
	{
		return mUSDCurrencyEstimated || mLocalCurrencyEstimated;
	}

	std::string getLocalEstimate() const;

	enum TransactionType
	{
		TransactionNone,
		TransactionCurrency,
		TransactionBuy
	};

	void startTransaction(TransactionType type, const char* method,
						  const LLSD& params);
	// Returns true if update needed
	bool checkTransaction();

	void setError(const std::string& message, const std::string& uri);
	void clearError();

	// Returns true if update needed
	bool considerUpdateCurrency();

	void currencyKey(S32 value);
	static void onCurrencyKey(LLLineEditor* callerp, void* datap);

	void prepare();
	void updateUI();

public:
	LLPanel&				mPanel;
	LLXMLRPCTransaction*	mTransaction;
	TransactionType			mTransactionType;

	LLFrameTimer			mCurrencyKeyTimer;

	std::string				mErrorMessage;
	std::string				mErrorURI;
	std::string				mZeroMessage;
	std::string				mSiteConfirm;

	std::string				mLocalCurrencyEstimatedCost;
	S32						mUSDCurrencyEstimatedCost;
	S32						mUserCurrencyBuy;

	bool					mCurrencyChanged;
	bool					mUserEnteredCurrencyBuy;
	bool					mUSDCurrencyEstimated;
	bool					mLocalCurrencyEstimated;
	bool					mSupportsInternationalBilling;
	bool					mBought;
	bool					mHidden;
	bool					mError;
};

LLCurrencyUIManager::Impl::Impl(LLPanel& dialog)
:	mPanel(dialog),
	mTransaction(NULL),
	mTransactionType(TransactionNone),
	// Note, this is a default, real value set in llfloaterbuycurrency.cpp
	mUserCurrencyBuy(2000),
	mUserEnteredCurrencyBuy(false),
	mSupportsInternationalBilling(false),
	mHidden(false),
	mError(false),
	mBought(false),
	mCurrencyChanged(false)
{
	clearEstimate();
}

LLCurrencyUIManager::Impl::~Impl()
{
	delete mTransaction;
}

static void add_common_params(LLSD& params)
{
	params["agentId"] = gAgentID.asString();
	params["secureSessionId"] = gAgent.getSecureSessionID().asString();
	params["language"] = LLUI::getLanguage();
	params["viewerChannel"] = gSavedSettings.getString("VersionChannelName");
	params["viewerMajorVersion"] = LL_VERSION_MAJOR;
	params["viewerMinorVersion"] = LL_VERSION_MINOR;
	params["viewerPatchVersion"] = LL_VERSION_BRANCH;
	// Note: to match was LL is doing to fit very large build numbers. HB
	params["viewerBuildVersion"] = std::to_string(LL_VERSION_RELEASE);
}

void LLCurrencyUIManager::Impl::updateCurrencyInfo()
{
	clearEstimate();
	mBought = false;
	mCurrencyChanged = false;

	if (mUserCurrencyBuy == 0)
	{
		mLocalCurrencyEstimated = true;
		return;
	}

	LLSD params = LLSD::emptyMap();
	add_common_params(params);
	params["currencyBuy"] = mUserCurrencyBuy;
	startTransaction(TransactionCurrency, "getCurrencyQuote", params);
}

void LLCurrencyUIManager::Impl::finishCurrencyInfo()
{
	const LLSD& result = mTransaction->response();

	bool success = result["success"].asBoolean();
	if (!success)
	{
		setError(result["errorMessage"].asString(),
				 result["errorURI"].asString());
		return;
	}

	const LLSD& currency = result["currency"];

	// Old XML-RPC server: estimatedCost = value in US$ cents
	mUSDCurrencyEstimated = currency.has("estimatedCost");
	if (mUSDCurrencyEstimated)
	{
		mUSDCurrencyEstimatedCost = currency["estimatedCost"].asInteger();
	}

	// Newer XML-RPC server: estimatedLocalCost = local currency string
	mLocalCurrencyEstimated = currency.has("estimatedLocalCost");
	if (mLocalCurrencyEstimated)
	{
		mLocalCurrencyEstimatedCost =
			currency["estimatedLocalCost"].asString();
		mSupportsInternationalBilling = true;
	}

	S32 new_currency_buy = currency["currencyBuy"].asInteger();
	if (mUserCurrencyBuy != new_currency_buy)
	{
		mUserCurrencyBuy = new_currency_buy;
		mUserEnteredCurrencyBuy = false;
	}

	mSiteConfirm = result["confirm"].asString();
}

void LLCurrencyUIManager::Impl::startCurrencyBuy(const std::string& password)
{
	LLSD params = LLSD::emptyMap();
	add_common_params(params);
	params["currencyBuy"] = mUserCurrencyBuy;
	params["confirm"] = mSiteConfirm;
	if (mUSDCurrencyEstimated)
	{
		params["estimatedCost"] = mUSDCurrencyEstimatedCost;
	}
	if (mLocalCurrencyEstimated)
	{
		params["estimatedLocalCost"] = mLocalCurrencyEstimatedCost;
	}
	if (!password.empty())
	{
		params["password"] = password;
	}

	startTransaction(TransactionBuy, "buyCurrency", params);

	clearEstimate();
	mCurrencyChanged = false;
}

void LLCurrencyUIManager::Impl::finishCurrencyBuy()
{
	const LLSD& result = mTransaction->response();

	bool success = result["success"].asBoolean();
	if (!success)
	{
		setError(result["errorMessage"].asString(),
				 result["errorURI"].asString());
	}
	else
	{
		mUserCurrencyBuy = 0;
		mUserEnteredCurrencyBuy = false;
		mBought = true;
	}
}

void LLCurrencyUIManager::Impl::startTransaction(TransactionType type,
												 const char* method,
												 const LLSD& params)
{
	static std::string transaction_uri;
	if (transaction_uri.empty())
	{
		transaction_uri = LLGridManager::getInstance()->getHelperURI() +
						 "currency.php";
	}

	delete mTransaction;

	mTransactionType = type;
	mTransaction = new LLXMLRPCTransaction(transaction_uri, method, params);

	clearError();
}

std::string LLCurrencyUIManager::Impl::getLocalEstimate() const
{
	if (mLocalCurrencyEstimated)
	{
		return mLocalCurrencyEstimatedCost;
	}
	if (mUSDCurrencyEstimated)
	{
		return llformat("US$ %#.2f", F32(mUSDCurrencyEstimatedCost) * 0.01f);
	}
	return "";
}

bool LLCurrencyUIManager::Impl::checkTransaction()
{
	if (!mTransaction || !mTransaction->process())
	{
		return false;
	}

	if (mTransaction->status(NULL) != LLXMLRPCTransaction::StatusComplete)
	{
		setError(mTransaction->statusMessage(), mTransaction->statusURI());
	}
	else
	{
		switch (mTransactionType)
		{
			case TransactionCurrency:
				finishCurrencyInfo();
				break;

			case TransactionBuy:
				finishCurrencyBuy();
				break;

			default:
				break;
		}
	}

	delete mTransaction;
	mTransaction = NULL;
	mTransactionType = TransactionNone;

	return true;
}

void LLCurrencyUIManager::Impl::setError(const std::string& message,
										 const std::string& uri)
{
	mError = true;
	mErrorMessage = message;
	mErrorURI = uri;
}

void LLCurrencyUIManager::Impl::clearError()
{
	mError = false;
	mErrorMessage.clear();
	mErrorURI.clear();
}

bool LLCurrencyUIManager::Impl::considerUpdateCurrency()
{
	if (mCurrencyChanged && !mTransaction  &&
		mCurrencyKeyTimer.getElapsedTimeF32() >= CURRENCY_ESTIMATE_FREQUENCY)
	{
		updateCurrencyInfo();
		return true;
	}
	return false;
}

void LLCurrencyUIManager::Impl::currencyKey(S32 value)
{
	mUserEnteredCurrencyBuy = true;
	mCurrencyKeyTimer.reset();

	if (mUserCurrencyBuy == value)
	{
		return;
	}

	mUserCurrencyBuy = value;

	if (hasEstimate())
	{
		clearEstimate();

		// Cannot just simply refresh the whole UI, as the edit field will get
		// reset and the cursor will change...
		mPanel.childHide("currency_est");
		LLTextBox* textp = mPanel.getChild<LLTextBox>("getting_data", true,
													  false);
		if (textp)
		{
			textp->setVisible(true);
		}
	}

	mCurrencyChanged = true;
}

//static
void LLCurrencyUIManager::Impl::onCurrencyKey(LLLineEditor* caller, void* data)
{
	S32 value = atoi(caller->getText().c_str());
	LLCurrencyUIManager::Impl* self = (LLCurrencyUIManager::Impl*)data;
	if (self)
	{
		self->currencyKey(value);
	}
}

void LLCurrencyUIManager::Impl::prepare()
{
	LLLineEditor* lineeditp = mPanel.getChild<LLLineEditor>("currency_amt");
	if (lineeditp)
	{
		lineeditp->setPrevalidate(LLLineEditor::prevalidateNonNegativeS32);
		lineeditp->setKeystrokeCallback(onCurrencyKey);
		lineeditp->setCallbackUserData(this);
	}
}

void LLCurrencyUIManager::Impl::updateUI()
{
	if (mHidden)
	{
		mPanel.childHide("currency_action");
		mPanel.childHide("currency_amt");
		mPanel.childHide("currency_est");
		return;
	}

	mPanel.childShow("currency_action");

	LLLineEditor* lineeditp = mPanel.getChild<LLLineEditor>("currency_amt");
	if (lineeditp)
	{
		lineeditp->setVisible(true);
		lineeditp->setLabel(mZeroMessage);

		if (!mUserEnteredCurrencyBuy)
		{
			if (mUserCurrencyBuy == 0)
			{
				lineeditp->setText(LLStringUtil::null);
			}
			else
			{
				lineeditp->setText(llformat("%d", mUserCurrencyBuy));
			}
			lineeditp->selectAll();
		}
	}

	std::string estimated = "US$ 0.00";
	if (mUserCurrencyBuy)
	{
		estimated = getLocalEstimate();
	}
	mPanel.childSetTextArg("currency_est", "[LOCALAMOUNT]", estimated);
	mPanel.childSetVisible("currency_est", hasEstimate() || !mUserCurrencyBuy);

	LLTextBox* textp = mPanel.getChild<LLTextBox>("getting_data", true, false);
	if (textp &&
		(mPanel.childIsEnabled("buy_btn") ||
		 mPanel.childIsVisible("currency_est") ||
		 mPanel.childIsVisible("error_web")))
	{
		textp->setVisible(false);
	}
}

LLCurrencyUIManager::LLCurrencyUIManager(LLPanel& dialog)
:	impl(*new Impl(dialog))
{
}

LLCurrencyUIManager::~LLCurrencyUIManager()
{
	delete &impl;
}

void LLCurrencyUIManager::setAmount(S32 amount, bool no_estimate)
{
	impl.mUserCurrencyBuy = amount;
	impl.mUserEnteredCurrencyBuy = false;
	impl.updateUI();
	impl.mCurrencyChanged = !no_estimate;
}

S32 LLCurrencyUIManager::getAmount()
{
	return impl.mUserCurrencyBuy;
}

void LLCurrencyUIManager::setZeroMessage(const std::string& message)
{
	impl.mZeroMessage = message;
}

void LLCurrencyUIManager::setUSDEstimate(S32 amount)
{
	impl.mUSDCurrencyEstimatedCost = amount;
	impl.mUSDCurrencyEstimated = true;
	impl.updateUI();
	impl.mCurrencyChanged = false;
}

S32 LLCurrencyUIManager::getUSDEstimate()
{
	return impl.mUSDCurrencyEstimated ? impl.mUSDCurrencyEstimatedCost : 0;
}

void LLCurrencyUIManager::setLocalEstimate(const std::string& amount)
{
	impl.mLocalCurrencyEstimatedCost = amount;
	impl.mLocalCurrencyEstimated = true;
	impl.updateUI();
	impl.mCurrencyChanged = false;
}

std::string LLCurrencyUIManager::getLocalEstimate() const
{
	return impl.getLocalEstimate();
}

void LLCurrencyUIManager::prepare()
{
	impl.prepare();
}

void LLCurrencyUIManager::updateUI(bool show)
{
	impl.mHidden = !show;
	impl.updateUI();
}

bool LLCurrencyUIManager::process()
{
	bool changed = false;
	changed |= impl.checkTransaction();
	changed |= impl.considerUpdateCurrency();
	return changed;
}

void LLCurrencyUIManager::buy(const std::string& buy_msg)
{
	if (!canBuy())
	{
		return;
	}

	LLUIString msg = buy_msg;
	msg.setArg("[LINDENS]", llformat("%d", impl.mUserCurrencyBuy));
	msg.setArg("[LOCALAMOUNT]", getLocalEstimate());
	LLConfirmationManager::confirm(impl.mSiteConfirm, msg, impl,
								   &LLCurrencyUIManager::Impl::startCurrencyBuy);
}

bool LLCurrencyUIManager::inProcess()
{
	return impl.mTransactionType != Impl::TransactionNone;
}

bool LLCurrencyUIManager::canCancel()
{
	return !buying();
}

bool LLCurrencyUIManager::canBuy()
{
	return !inProcess() && impl.hasEstimate() && impl.mUserCurrencyBuy > 0;
}

bool LLCurrencyUIManager::buying()
{
	return impl.mTransactionType == Impl::TransactionBuy;
}

bool LLCurrencyUIManager::bought()
{
	return impl.mBought;
}

void LLCurrencyUIManager::clearError()
{
	impl.clearError();
}

bool LLCurrencyUIManager::hasError()
{
	return impl.mError;
}

std::string LLCurrencyUIManager::errorMessage()
{
	return impl.mErrorMessage;
}

std::string LLCurrencyUIManager::errorURI()
{
	return impl.mErrorURI;
}
