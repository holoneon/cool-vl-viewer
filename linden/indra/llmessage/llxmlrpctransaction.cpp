/**
 * @file llxmlrpctransaction.cpp
 * @brief LLXMLRPCTransaction and related class implementations
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

#include "linden_common.h"

#include <memory>

#include "curl/curl.h"				// For Curl's typedefs and codes

#include "llxmlrpctransaction.h"

#include "llcorebufferarray.h"
#include "llcorehttphandler.h"
#include "llcorehttprequest.h"
#include "llcorehttpresponse.h"
#include "llhttpconstants.h"
#include "llsdutil.h"				// For ll_pretty_print_sd()
#include "llxmlnode.h"

///////////////////////////////////////////////////////////////////////////////
// LLXMLRPCTransaction::Handler sub-class
///////////////////////////////////////////////////////////////////////////////

class LLXMLRPCTransaction::Handler : public LLCore::HttpHandler
{
protected:
	LOG_CLASS(LLXMLRPCTransaction::Handler);

public:
	typedef std::shared_ptr<LLXMLRPCTransaction::Handler> ptr_t;

	Handler(LLCore::HttpRequest::ptr_t& request,
			LLXMLRPCTransaction::Impl* impl)
	:	mImpl(impl),
		mRequest(request)
	{
	}

	virtual ~Handler() = default;

	virtual void onCompleted(LLCore::HttpHandle handle,
							 LLCore::HttpResponse* responsep);

private:
	LLXMLRPCTransaction::Impl*	mImpl;
	LLCore::HttpRequest::ptr_t	mRequest;
};

///////////////////////////////////////////////////////////////////////////////
// LLXMLRPCTransaction::Impl sub-class
///////////////////////////////////////////////////////////////////////////////

class LLXMLRPCTransaction::Impl
{
protected:
	LOG_CLASS(LLXMLRPCTransaction::Impl);

public:
	typedef LLXMLRPCTransaction::EStatus EStatus;

	Impl(const std::string& uri, const std::string& method,
		 const LLSD& params);

	bool process();

	void setStatus(EStatus code, const std::string& message = "",
				   const std::string& uri = "");
	void setHttpStatus(const LLCore::HttpStatus& status);

private:
	bool parseResponse(LLXMLNodePtr root);
	bool parseValue(LLSD& target, LLXMLNodePtr source);

public:
	LLCore::HttpRequest::ptr_t					mHttpRequest;
	LLCore::HttpHandle							mPostH;
	Handler::ptr_t								mHandler;
	EStatus										mStatus;
	CURLcode									mCurlCode;
	std::string									mStatusMessage;
	std::string									mStatusURI;
	std::string									mURI;
	std::string									mResponseText;
	LLSD										mResponseData;
	bool										mHasResponse;
	bool										mResponseParsed;

	static std::string							sSupportURL;
	static std::string							sWebsiteURL;
	static std::string							sServerIsDownMsg;
	static std::string							sNotResolvingMsg;
	static std::string							sNotVerifiedMsg;
	static std::string							sConnectErrorMsg;

	static bool									sVerifyCert;
};

///////////////////////////////////////////////////////////////////////////////
// LLXMLRPCTransaction::Handler sub-class method
///////////////////////////////////////////////////////////////////////////////

void LLXMLRPCTransaction::Handler::onCompleted(LLCore::HttpHandle handle,
											   LLCore::HttpResponse* responsep)
{
	if (!responsep || !mImpl) return;	// Paranioa

	LLCore::HttpStatus status = responsep->getStatus();
	if (!status)
	{
		if (status.toULong() != CURLE_SSL_PEER_CERTIFICATE &&
			status.toULong() != CURLE_SSL_CACERT)
		{
			// If we have a curl error that has not already been handled (a non
			// cert error), then generate the error message as appropriate
			mImpl->setHttpStatus(status);
			llwarns << "Error " << status.toHex() << ": " << status.toString()
					<< " - Request URI: " << mImpl->mURI << llendl;
		}
		return;
	}

	// The contents of a buffer array are potentially non-contiguous, so we
	// will need to copy them into an contiguous block of memory for XMLRPC.
	LLCore::BufferArray* bodyp = responsep->getBody();
	if (!bodyp)	// This *does* happen !
	{
		mImpl->mResponseText.clear();
		mImpl->mResponseData.clear();
		llwarns << "Empty response; request URI: " << mImpl->mURI << llendl;
		return;
	}

	size_t body_size = bodyp->size();
	mImpl->mResponseText.resize(body_size, '\0');
	bodyp->read(0, mImpl->mResponseText.data(), body_size);
	LL_DEBUGS("XmlRpc") << "XMLRPC response body: " << mImpl->mResponseText
						<< LL_ENDL;

	// We do not do the parsing in the HTTP coroutine, since it could exhaust
	// the coroutine stack in extreme cases. Instead, we flag the data buffer
	// as ready, and let mImpl decode it in its process() method, on the main
	// coroutine. HB
	mImpl->mHasResponse = true;
	mImpl->setStatus(LLXMLRPCTransaction::StatusComplete);
}

///////////////////////////////////////////////////////////////////////////////
// LLXMLRPCTransaction::Impl sub-class methods
///////////////////////////////////////////////////////////////////////////////

// Static members
std::string LLXMLRPCTransaction::Impl::sSupportURL;
std::string LLXMLRPCTransaction::Impl::sWebsiteURL;
std::string	LLXMLRPCTransaction::Impl::sServerIsDownMsg;
std::string	LLXMLRPCTransaction::Impl::sNotResolvingMsg;
std::string	LLXMLRPCTransaction::Impl::sNotVerifiedMsg;
std::string	LLXMLRPCTransaction::Impl::sConnectErrorMsg;
bool LLXMLRPCTransaction::Impl::sVerifyCert = true;

LLXMLRPCTransaction::Impl::Impl(const std::string& uri,
								const std::string& method,
								const LLSD& params)
:	mStatus(LLXMLRPCTransaction::StatusNotStarted),
	mURI(uri),
	mHasResponse(false),
	mResponseParsed(false)
{
	if (!mHttpRequest)
	{
		mHttpRequest = DEFAULT_HTTP_REQUEST;
	}

	// LLRefCounted starts with a 1 ref, so do not add a ref in the smart
	// pointer
	LLCore::HttpOptions::ptr_t options = DEFAULT_HTTP_OPTIONS;
	// Be a little impatient about establishing connections.
	options->setTimeout(40L);
	options->setSSLVerifyPeer(sVerifyCert);
	options->setSSLVerifyHost(sVerifyCert);
	options->setDNSCacheTimeout(40);
	options->setRetries(3);

	// LLRefCounted starts with a 1 ref, so do not add a ref in the smart
	// pointer
	LLCore::HttpHeaders::ptr_t headers = DEFAULT_HTTP_HEADERS;
	headers->append(HTTP_OUT_HEADER_CONTENT_TYPE, HTTP_CONTENT_TEXT_XML);

	LLCore::BufferArray::ptr_t body =
		LLCore::BufferArray::ptr_t(new LLCore::BufferArray());

	std::string request = "<?xml version=\"1.0\"?><methodCall><methodName>" +
						  method + "</methodName><params><param>" +
						  params.asXMLRPCValue() +
						  "</param></params></methodCall>";
	body->append(request.c_str(), request.size());
#if LL_DEBUG
	// Note: this shows password and MFA hashes, so only enabled for debug
	// builds. HB
	LL_DEBUGS("XmlRpc") << "XMLRPC request body:\n" << request << LL_ENDL;
#endif
	mHandler = LLXMLRPCTransaction::Handler::ptr_t(new Handler(mHttpRequest,
															   this));
	mPostH = mHttpRequest->requestPost(LLCore::HttpRequest::DEFAULT_POLICY_ID,
									   mURI, body.get(), options, headers,
									   mHandler);
}

bool LLXMLRPCTransaction::Impl::parseResponse(LLXMLNodePtr root)
{
	// We have already checked in LLXMLNode::parseBuffer() that root contains
	// exactly one child.
	if (!root->hasName("methodResponse"))
	{
		llwarns << "Invalid root element in XML response; request URI: "
				<< mURI << llendl;
		return false;
	}

	LLXMLNodePtr first = root->getFirstChild();
	LLXMLNodePtr second = first->getFirstChild();
	if (first && !first->getNextSibling() && second &&
		!second->getNextSibling())
	{
		if (first->hasName("fault"))
		{
			LLSD fault;
			if (parseValue(fault, second) && fault.isMap() &&
				fault.has("faultCode") && fault.has("faultString"))
			{
				llwarns << "Request failed. faultCode: '"
						<< fault.get("faultCode").asString()
						<< "', faultString: '"
						<< fault.get("faultString").asString()
						<< "', request URI: " << mURI << llendl;
				return false;
			}
		}
		else if (first->hasName("params") &&
				 second->hasName("param") && !second->getNextSibling())
		{
			LLXMLNodePtr third = second->getFirstChild();
			if (third && !third->getNextSibling() &&
				parseValue(mResponseData, third))
			{
				return true;
			}
		}
	}

	llwarns << "Invalid response format; request URI: " << mURI << llendl;
	return false;
}

bool LLXMLRPCTransaction::Impl::parseValue(LLSD& target, LLXMLNodePtr src)
{
	bool success = src->fromXMLRPCValue(target);
	LL_DEBUGS("XmlRpc") << "XMLTreeNode parsing "
						<< (success ? "succeeded" : "failed") << LL_ENDL;
	return success;
}

bool LLXMLRPCTransaction::Impl::process()
{
	if (!mPostH || !mHttpRequest)
	{
		llwarns << "Transaction failed." << llendl;
		return true;
	}

	// Parse the response when we have one and it has not yet been parsed. HB
	if (mHasResponse && !mResponseParsed)
	{
		bool strip_escaped_strings = LLXMLNode::sStripEscapedStrings;
		LLXMLNode::sStripEscapedStrings = false;
		LLXMLNodePtr root;
		if (!LLXMLNode::parseBuffer(mResponseText.data(), mResponseText.size(),
									root, NULL))
		{
			llwarns << "XML response parsing failed; request URI: " << mURI
					<< llendl;
		}
		else if (parseResponse(root))
		{
			LL_DEBUGS("XmlRpc") << "Parsed response LLSD:\n"
								<< ll_pretty_print_sd(mResponseData)
								<< LL_ENDL;
		}
		else
		{
			llwarns << "XMLRPC response parsing failed; request URI: " << mURI
					<< llendl;
		}
		LLXMLNode::sStripEscapedStrings = strip_escaped_strings;
		mResponseParsed = true;
	}

	switch (mStatus)
	{
		case LLXMLRPCTransaction::StatusComplete:
		case LLXMLRPCTransaction::StatusCURLError:
		case LLXMLRPCTransaction::StatusXMLRPCError:
		case LLXMLRPCTransaction::StatusOtherError:
		{
			return true;
		}

		case LLXMLRPCTransaction::StatusNotStarted:
		{
			setStatus(LLXMLRPCTransaction::StatusStarted);
		}

		default:
			break;
	}

	LLCore::HttpStatus status = mHttpRequest->update(0);
	if (!status)
	{
		llwarns << "Error (1): " << status.toString() << llendl;
		return false;
	}

	status = mHttpRequest->getStatus();
	if (!status)
	{
		llwarns << "Error (2): " << status.toString() << llendl;
	}

	return false;
}

void LLXMLRPCTransaction::Impl::setStatus(EStatus status,
										  const std::string& message,
										  const std::string& uri)
{
	mStatus = status;
	mStatusMessage = message;
	mStatusURI = uri;

	if (mStatusMessage.empty())
	{
		switch (mStatus)
		{
			case StatusNotStarted:
				mStatusMessage = "(not started)";
				break;

			case StatusStarted:
				mStatusMessage = "(waiting for server response)";
				break;

			case StatusDownloading:
				mStatusMessage = "(reading server response)";
				break;

			case StatusComplete:
				mStatusMessage = "(done)";
				break;

			default:
			{
				// Usually this means that there is a problem with the login
				// server, not with the client. Direct user to status page.
				mStatusMessage = sServerIsDownMsg;
				mStatusURI = sWebsiteURL;
			}
		}
	}
}

void LLXMLRPCTransaction::Impl::setHttpStatus(const LLCore::HttpStatus& status)
{
	CURLcode code = (CURLcode)status.toULong();
	std::string message;
	switch (code)
	{
		case CURLE_COULDNT_RESOLVE_HOST:
			message = sNotResolvingMsg;
			break;

		case CURLE_SSL_CACERT:
		// Note: CURLE_SSL_CACERT and CURLE_SSL_CACERT may expand to the same
		// value in recent curl versions (seen with curl v7.68).
#if CURLE_SSL_CACERT != CURLE_SSL_PEER_CERTIFICATE
		case CURLE_SSL_PEER_CERTIFICATE:
#endif
			message = sNotVerifiedMsg;
			break;

		case CURLE_SSL_CONNECT_ERROR:
			message = sConnectErrorMsg;
			break;

		default:
			break;
	}
	mCurlCode = code;
	setStatus(StatusCURLError, message, sSupportURL);
}

///////////////////////////////////////////////////////////////////////////////
// LLXMLRPCTransaction class proper
///////////////////////////////////////////////////////////////////////////////

LLXMLRPCTransaction::LLXMLRPCTransaction(const std::string& uri,
										 const std::string& method,
										 const LLSD& params)
:	impl(*new Impl(uri, method, params))
{
}

LLXMLRPCTransaction::~LLXMLRPCTransaction()
{
	delete &impl;
}

bool LLXMLRPCTransaction::process()
{
	return impl.process();
}

LLXMLRPCTransaction::EStatus LLXMLRPCTransaction::status(S32* curl_code)
{
	if (curl_code)
	{
		*curl_code = impl.mStatus == StatusCURLError ? impl.mCurlCode
													 : CURLE_OK;
	}
	return impl.mStatus;
}

std::string LLXMLRPCTransaction::statusMessage()
{
	return impl.mStatusMessage;
}

std::string LLXMLRPCTransaction::statusURI()
{
	return impl.mStatusURI;
}

const LLSD& LLXMLRPCTransaction::response()
{
	return impl.mResponseData;
}

//static
void LLXMLRPCTransaction::setSupportURL(const std::string& url)
{
	Impl::sSupportURL = url;
}

//static
void LLXMLRPCTransaction::setWebsiteURL(const std::string& url)
{
	Impl::sWebsiteURL = url;
}

//static
void LLXMLRPCTransaction::setVerifyCert(bool verify)
{
	Impl::sVerifyCert = verify;
}

//static
void LLXMLRPCTransaction::setMessages(const std::string& server_down,
									  const std::string& not_resolving,
									  const std::string& not_verified,
									  const std::string& connect_error)
{
	Impl::sServerIsDownMsg = server_down;
	Impl::sNotResolvingMsg = not_resolving;
	Impl::sNotVerifiedMsg = not_verified;
	Impl::sConnectErrorMsg = connect_error;
}
