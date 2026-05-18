/**
 * @file llviewerassetupload.h
 * @brief Asset upload requests.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 *
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include <deque>

#include "llassetstorage.h"
#include "llcorehttputil.h"
#include "llfoldertype.h"
#include "llimage.h"
#include "llinventorytype.h"
#include "llmutex.h"

// Used to be in a separate llresourcedata.h file, but only used in
// llviewerassetupload.cpp and in llfloaterreporter.cpp, both of which
// #including llviewerassetupload.h...
struct LLResourceData
{
	LLAssetInfo				mAssetInfo;
	LLFolderType::EType		mPreferredLocation;
	LLInventoryType::EType	mInventoryType;
	U32						mNextOwnerPerm;
	S32						mExpectedUploadCost;
	void*					mUserData;

	static constexpr S8 INVALID_LOCATION = -2;
};

class LLResourceUploadInfo
{
protected:
	LOG_CLASS(LLResourceUploadInfo);

public:
	typedef std::shared_ptr<LLResourceUploadInfo> ptr_t;

	LLResourceUploadInfo(const LLTransactionID& tid,
						 LLAssetType::EType asset_type,
						 const std::string& name,
						 const std::string& description,
						 S32 compression_info,
						 LLFolderType::EType destination_type,
						 LLInventoryType::EType inventory_type,
						 U32 next_owner_perms, U32 group_perms,
						 U32 everyone_perms, S32 expected_cost);

	virtual ~LLResourceUploadInfo() = default;

	virtual S32 getExpectedUploadCost();
	virtual LLSD prepareUpload();
	virtual LLSD generatePostBody();
	virtual void logPreparedUpload();
	virtual LLUUID finishUpload(const LLSD& result);

	LL_INLINE virtual void failedUpload(const LLSD& result,
										std::string& reason)
	{
	}

	LL_INLINE const LLTransactionID& getTransactionId() const
	{
		return mTransactionId;
	}

	LL_INLINE LLAssetType::EType getAssetType() const	{ return mAssetType; }
	std::string getAssetTypeString() const;

	LL_INLINE void setName(const std::string& name)		{ mName =  name; }
	LL_INLINE const std::string& getName() const		{ return mName; }
	LL_INLINE const std::string& getDescription() const	{ return mDescription; }

	LL_INLINE S32 getCompressionInfo() const			{ return mCompressionInfo; }

	LL_INLINE LLFolderType::EType getDestinationFolderType() const
	{
		return mDestinationFolderType;
	}

	LL_INLINE LLInventoryType::EType getInventoryType() const
	{
		return mInventoryType;
	}

	std::string getInventoryTypeString() const;

	LL_INLINE U32 getNextOwnerPerms() const				{ return mNextOwnerPerms; }
	LL_INLINE U32 getGroupPerms() const					{ return mGroupPerms; }
	LL_INLINE U32 getEveryonePerms() const				{ return mEveryonePerms; }

	LL_INLINE virtual bool showUploadDialog() const		{ return true; }

	LL_INLINE void setShowInventoryPanel(bool show)		{ mShowInventoryPanel = show; }
	LL_INLINE bool showInventoryPanel() const			{ return mShowInventoryPanel; }

	virtual std::string getDisplayName() const;

	LL_INLINE const LLUUID& getFolderId() const			{ return mFolderId; }
	LL_INLINE const LLUUID& getItemId() const			{ return mItemId; }
	LL_INLINE const LLAssetID& getAssetId() const		{ return mAssetId; }

	// Additional "capability callback", for use with the global
	// upload_new_resource() utility function, when the latter creates the
	// new inventory item via the capability (the asset storage callback being
	// already passed as an optional parameter to upload_new_resource()).
	// This also can be used for any LLResourceUploadInfo and
	// LLNewFileResourceUploadInfo uploads that, unlike
	// LLBufferedAssetUploadInfo, do not have a finish callback mechanism.
	// In the case of upload_new_resource(), it will default to a callback
	// ensuring that the bulk uploads are chained one after the other. HB
	typedef void (*capability_cb_t)(const LLSD& result, void* userdatap);
	LL_INLINE void setCapCallback(capability_cb_t cb, void* userdatap)
	{
		mCapCallback = cb;
		mUserData = userdatap;
	}

	LL_INLINE bool hasCapCallback() const				{ return mCapCallback != NULL; }

	void performCallback(const LLSD& result);

protected:
	LLResourceUploadInfo(const std::string& name,
						 const std::string& description,
						 S32 compression_info,
						 LLFolderType::EType destination_type,
						 LLInventoryType::EType inventory_type,
						 U32 next_owner_perms, U32 group_perms,
						 U32 everyone_perms, S32 expected_cost);

	LLResourceUploadInfo(const LLAssetID& asset_id,
						 LLAssetType::EType asset_type,
						 const std::string& name);

	LL_INLINE void setTransactionId(const LLTransactionID& tid)
	{
		mTransactionId = tid;
	}

	LL_INLINE void setAssetType(LLAssetType::EType t)	{ mAssetType = t; }
	LL_INLINE void setItemId(const LLUUID& id)			{ mItemId = id; }
	LL_INLINE void setAssetId(const LLUUID& id)			{ mAssetId = id; }
	LL_INLINE void setExpectedUploadCost(S32 c)			{ mExpectedUploadCost = c; }

	LLAssetID generateNewAssetId();
	void incrementUploadStats() const;
	virtual void assignDefaults();

private:
	LLTransactionID			mTransactionId;
	LLAssetType::EType		mAssetType;
	std::string				mName;
	std::string				mDescription;
	S32						mCompressionInfo;
	LLFolderType::EType		mDestinationFolderType;
	LLInventoryType::EType	mInventoryType;
	U32						mNextOwnerPerms;
	U32						mGroupPerms;
	U32						mEveryonePerms;
	S32						mExpectedUploadCost;

	LLUUID					mFolderId;
	LLUUID					mItemId;
	LLAssetID				mAssetId;

	capability_cb_t			mCapCallback;
	void*					mUserData;

	bool					mShowInventoryPanel;
};

class LLNewFileResourceUploadInfo : public LLResourceUploadInfo
{
protected:
	LOG_CLASS(LLNewFileResourceUploadInfo);

public:
	LLNewFileResourceUploadInfo(const std::string& filename,
								const std::string& name,
								const std::string& description,
								S32 compression_info,
								LLFolderType::EType destination_type,
								LLInventoryType::EType inventory_type,
								U32 next_owner_perms, U32 group_perms,
								U32 everyone_perms, S32 expected_cost);

	virtual LLSD prepareUpload();

	LL_INLINE const std::string& getFileName() const	{ return mFileName; }

	LL_INLINE void setIsBulkUpload()					{ mIsBulkUpload = true; }

protected:
	virtual LLSD exportTempFile();

private:
	std::string	mFileName;
	bool		mIsBulkUpload;
};

class LLNewBufferedResourceUploadInfo : public LLResourceUploadInfo
{
protected:
	LOG_CLASS(LLNewBufferedResourceUploadInfo);

public:
	typedef std::function<void(LLUUID new_asset_id,
							   LLSD response)> uploaded_cb_t;
	typedef std::function<void(const LLUUID& asset_id, const LLSD& response,
							   std::string reason)> failed_cb_t;

	LLNewBufferedResourceUploadInfo(const std::string& buffer,
									const LLAssetID& asset_id,
									const std::string& name,
									const std::string& description,
									S32 compression_info,
									LLFolderType::EType destination_type,
									LLInventoryType::EType inventory_type,
									LLAssetType::EType asset_type,
									U32 next_owner_perms, U32 group_perms,
									U32 everyone_perms, S32 expected_cost,
									uploaded_cb_t finish,
									failed_cb_t fail = failed_cb_t());

	virtual LLSD prepareUpload();

protected:
	virtual LLSD exportTempFile();
	virtual LLUUID finishUpload(const LLSD& result);
	virtual void failedUpload(const LLSD& result, std::string& reason);

private:
	uploaded_cb_t	mFinishFn;
	failed_cb_t		mFailureFn;
	std::string		mBuffer;
};

class LLBufferedAssetUploadInfo : public LLResourceUploadInfo
{
protected:
	LOG_CLASS(LLBufferedAssetUploadInfo);

public:
	typedef std::function<void(LLUUID item_id, LLUUID new_asset_id,
							   LLUUID new_item_id,
							   LLSD response)> inv_uploaded_cb_t;

	typedef std::function<void(LLUUID item_id, LLUUID task_id,
							   LLUUID new_asset_id,
							   LLSD response)> task_uploaded_cb_t;

	typedef std::function<void(const LLUUID& item_id, const LLUUID& task_id,
							   const LLSD& response,
							   std::string reason)> failed_cb_t;

	LLBufferedAssetUploadInfo(const LLUUID& item_id,
							  LLAssetType::EType asset_type,
							  const std::string& buffer,
							  inv_uploaded_cb_t finish,
							  failed_cb_t failed = failed_cb_t());

	LLBufferedAssetUploadInfo(const LLUUID& item_id,
							  LLPointer<LLImageFormatted> image,
							  inv_uploaded_cb_t finish);

	LLBufferedAssetUploadInfo(const LLUUID& task_id, const LLUUID& item_id,
							  LLAssetType::EType assetType,
							  const std::string& buffer,
							  task_uploaded_cb_t finish,
							  failed_cb_t failed = failed_cb_t());

	virtual LLSD prepareUpload();
	virtual LLSD generatePostBody();
	virtual LLUUID finishUpload(const LLSD& result);
	virtual void failedUpload(const LLSD& result, std::string& reason);

	LL_INLINE const LLUUID& getTaskId() const			{ return mTaskId; }
	LL_INLINE const std::string& getContents() const	{ return mContents; }

	LL_INLINE virtual bool showUploadDialog() const		{ return false; }

private:
	LLUUID				mTaskId;
	std::string			mContents;
	inv_uploaded_cb_t	mInvnFinishFn;
	task_uploaded_cb_t	mTaskFinishFn;
	failed_cb_t			mFailureFn;
	bool				mTaskUpload;
	bool				mStoredToCache;
};

class LLScriptAssetUpload : public LLBufferedAssetUploadInfo
{
protected:
	LOG_CLASS(LLScriptAssetUpload);

public:
	LLScriptAssetUpload(const LLUUID& item_id, const std::string& buffer,
						const std::string& compile_target,
						inv_uploaded_cb_t finish, failed_cb_t failed);

	LLScriptAssetUpload(const LLUUID& task_id, const LLUUID& item_id,
						const std::string& compile_target, bool running,
						const LLUUID& exerience_id, const std::string& buffer,
						task_uploaded_cb_t finish, failed_cb_t failed);

	virtual LLSD generatePostBody();

	LL_INLINE const LLUUID& getExerienceId() const		{ return mExerienceId; }

	LL_INLINE const std::string& getCompileTarget() const
	{
		return mCompileTarget;
	}

	LL_INLINE bool getIsRunning() const					{ return mIsRunning; }

private:
	LLUUID		mExerienceId;
	std::string	mCompileTarget;
	bool		mIsRunning;
};

class LLViewerAssetUpload
{
protected:
	LOG_CLASS(LLViewerAssetUpload);

public:
	static LLUUID enqueueInventoryUpload(const std::string& url,
										 const LLResourceUploadInfo::ptr_t& infop);

	static void assetInventoryUploadCoproc(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t& adaptp,
										   std::string url,
										   LLResourceUploadInfo::ptr_t infop);

private:
	static void handleUploadError(LLCore::HttpStatus status,
								  const LLSD& result,
								  LLResourceUploadInfo::ptr_t& uploadInfo);
};

// Bulk uploads queue
extern std::deque<std::string> gUploadQueue;
extern LLMutex gUploadQueueMutex;

// Global utility function for uploading assets
void upload_new_resource(LLResourceUploadInfo::ptr_t& infop,
						 LLAssetStorage::LLStoreAssetCallback callback = NULL,
						 void* userdatap = NULL, bool temp_upload = false);
// Internal callback also used in llviewermenu.cpp to initiate bulk uploads.
void process_bulk_upload_queue(const LLSD& result = LLSD(),
							   void* userdatap = NULL);

// This callback was originally part of llassetuploadresponders.h/cpp
void on_new_single_inventory_upload_complete(LLAssetType::EType asset_type,
											 LLInventoryType::EType inventory_type,
											 const std::string inventory_type_string,
											 const LLUUID& item_folder_id,
											 const std::string& item_name,
											 const std::string& item_description,
											 const LLSD& server_response,
											 S32 upload_price);
