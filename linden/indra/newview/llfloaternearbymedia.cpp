/**
 * @file llfloaternearbymedia.cpp
 * @brief Management interface for muting and controlling nearby media
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llfloaternearbymedia.h"

#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lllineeditor.h"
#include "llparcel.h"
#include "llscrolllistctrl.h"
#include "llsliderctrl.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"

#include "llappviewer.h"			// For gFrameTimeSeconds
#include "llfloaterpreference.h"
#include "llviewermedia.h"
#include "llviewermediafocus.h"
#include "llviewerparcelmedia.h"
#include "llviewerparcelmgr.h"

static const LLUUID PARCEL_MEDIA_LIST_ITEM_UUID = LLUUID("CAB5920F-E484-4233-8621-384CF373A321");
static const LLUUID PARCEL_AUDIO_LIST_ITEM_UUID = LLUUID("DF4B020D-8A24-4B95-AB5D-CA970D694822");

LLFloaterNearByMedia::LLFloaterNearByMedia(const LLSD&)
:	mParcelMediaItem(NULL),
	mParcelAudioItem(NULL),
	mStreamingMusic(LLCachedControl<bool>(gSavedSettings,
										  "EnableStreamingMusic")),
	mSteamingMedia(LLCachedControl<bool>(gSavedSettings,
										 "EnableStreamingMedia")),
	mSharedMedia(LLCachedControl<bool>(gSavedSettings,
									   "PrimMediaMasterEnabled"))
{
	LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_nearby_media.xml");
}

//virtual
LLFloaterNearByMedia::~LLFloaterNearByMedia()
{
	gSavedSettings.setBool("ShowNearbyMedia", false);
}

//virtual
bool LLFloaterNearByMedia::postBuild()
{
	mMediaTypeCombo = getChild<LLComboBox>("show_combo");
	mMediaTypeCombo->setCommitCallback(onCommitMediaType);
	mMediaTypeCombo->setCallbackUserData(this);

	mMediaList = getChild<LLScrollListCtrl>("media_list");
	mMediaList->setCommitOnSelectionChange(true);
	mMediaList->setCommitCallback(onSelectNewMedia);
	mMediaList->setCallbackUserData(this);
	mMediaList->setDoubleClickCallback(onClickSelectedMediaZoom);
	mMediaList->sortByColumnIndex(PROXIMITY_COLUMN, true);
	mMediaList->sortByColumnIndex(VISIBILITY_COLUMN, false);

	mMediaURLText = getChild<LLTextBox>("url_text");

	mMediaURLEditor = getChild<LLLineEditor>("media_url");
	mMediaURLEditor->setCommitCallback(onCommitMediaURL);
	mMediaURLEditor->setCallbackUserData(this);

	mPlayButton = getChild<LLButton>("play_btn");
	mPlayButton->setClickedCallback(onClickSelectedMediaPlay, this);

	mPauseButton = getChild<LLButton>("pause_btn");
	mPauseButton->setClickedCallback(onClickSelectedMediaPause, this);

	mStopButton = getChild<LLButton>("stop_btn");
	mStopButton->setClickedCallback(onClickSelectedMediaStop, this);

	mVolumeSlider = getChild<LLSliderCtrl>("volume_slider");
	mVolumeSlider->setCommitCallback(onCommitSelectedMediaVolume);
	mVolumeSlider->setCallbackUserData(this);

	mMuteButton = getChild<LLButton>("mute_btn");
	mMuteButton->setClickedCallback(onClickSelectedMediaMute, this);

	mUnmuteButton = getChild<LLButton>("unmute_btn");
	mUnmuteButton->setClickedCallback(onClickSelectedMediaUnmute, this);

	mZoomButton = getChild<LLButton>("zoom_btn");
	mZoomButton->setClickedCallback(onClickSelectedMediaZoom, this);

	mUnzoomButton = getChild<LLButton>("unzoom_btn");
	mUnzoomButton->setClickedCallback(onClickSelectedMediaUnzoom, this);

	mEnableAllButton = getChild<LLButton>("enable_all_btn");
	mEnableAllButton->setClickedCallback(onClickEnableAll, this);

	mDisableAllButton = getChild<LLButton>("disable_all_btn");
	mDisableAllButton->setClickedCallback(onClickDisableAll, this);

	mOpenPrefsButton = getChild<LLButton>("open_prefs_btn");
	mOpenPrefsButton->setClickedCallback(onOpenPrefs, this);

	mEmptyNameString = getString("empty_item_text");
	mParcelMediaName = getString("parcel_media_name");
	mParcelAudioName = getString("parcel_audio_name");
	mPlayingString = getString("playing_suffix");

	gSavedSettings.setBool("ShowNearbyMedia", true);

	refreshList(true);

	return true;
}

void LLFloaterNearByMedia::draw()
{
	static F32 last_update = 0.f;

	// Do not update every frame: that would be insane !  HB
	constexpr F32 DELAY = 0.33f;	// In seconds.
	if (gFrameTimeSeconds > last_update + DELAY)
	{
		last_update = gFrameTimeSeconds;
		refreshList();
		updateControls();
	}

	LLFloater::draw();
}

void LLFloaterNearByMedia::updateControls()
{
	LLUUID media_id = mMediaList->getValue().asUUID();
	if (media_id == PARCEL_AUDIO_LIST_ITEM_UUID)
	{
		if (!mStreamingMusic || !LLViewerParcelMedia::hasParcelAudio())
		{
			showDisabledControls();
		}
		else
		{
			static LLCachedControl<bool> muted_music(gSavedSettings,
													 "MuteMusic");
			static LLCachedControl<F32> music_volume(gSavedSettings,
													 "AudioLevelMusic");
			showTimeBasedControls(LLViewerParcelMedia::parcelMusicPlaying(),
								  LLViewerParcelMedia::parcelMusicPaused(),
								  false, false,	// no zoom on audio...
								  muted_music, music_volume);
		}
	}
	else if (media_id == PARCEL_MEDIA_LIST_ITEM_UUID)
	{
		if (!mSteamingMedia || !LLViewerParcelMedia::hasParcelMedia())
		{
			showDisabledControls();
		}
		else
		{
			// *TODO: find a way to allow zooming on parcel media...
			LLViewerMediaImpl* implp = LLViewerParcelMedia::getParcelMedia();
			if (!implp)
			{
				// It has not started yet
				showBasicControls(false, false, false, false, 1.f);
			}
			else
			{
				F32 volume = implp->getVolume();
				if (implp->isMediaTimeBased())
				{
					showTimeBasedControls(implp->isMediaPlaying(),
										  implp->isMediaPaused(),
										  false, false,
										  volume <= 0.f, volume);
				}
				else
				{
					showBasicControls(LLViewerParcelMedia::isParcelMediaPlaying(),
									  false, false, volume <= 0.f, volume);
				}
			}
		}
	}
	else if (!mSteamingMedia || !mSharedMedia)
	{
		showDisabledControls();
	}
	else
	{
		// *TODO: find a way to allow zooming on parcel media...
		LLViewerMediaImpl* implp =
			LLViewerMedia::getMediaImplFromTextureID(media_id);
		if (!implp)
		{
			showDisabledControls();
		}
		else
		{
			F32 volume = implp->getVolume();
			// *TODO: find a way to allow zooming on parcel media...
			const LLUUID& media_id = implp->getMediaTextureID();
			bool zoomed =
				LLViewerMediaFocus::getInstance()->isZoomedOnMedia(media_id);
			if (implp->isMediaTimeBased())
			{
				showTimeBasedControls(implp->isMediaPlaying(),
									  implp->isMediaPaused(),
									  !implp->isParcelMedia(), zoomed,
									  volume <= 0.f, volume);
			}
			else
			{
				showBasicControls(!implp->isMediaDisabled(),
								  !implp->isParcelMedia(), zoomed,
								  volume <= 0.f, volume);
			}
		}
	}
}

void LLFloaterNearByMedia::showBasicControls(bool playing, bool include_zoom,
											 bool is_zoomed, bool muted,
											 F32 volume)
{
	mPlayButton->setVisible(true);
	mPlayButton->setEnabled(!playing);
	mPauseButton->setVisible(false);
	mStopButton->setEnabled(playing);
	mVolumeSlider->setVisible(true);
	mMuteButton->setEnabled(true);
	mMuteButton->setVisible(!muted);
	mUnmuteButton->setVisible(muted);
	mVolumeSlider->setEnabled(true);
	mVolumeSlider->setValue(volume);
	mZoomButton->setVisible(include_zoom && !is_zoomed);
	mUnzoomButton->setVisible(include_zoom && is_zoomed);
}

void LLFloaterNearByMedia::showTimeBasedControls(bool playing, bool paused,
												 bool include_zoom,
												 bool is_zoomed, bool muted,
												 F32 volume)
{
	mPlayButton->setVisible(!playing || paused);
	mPlayButton->setEnabled(true);
	mStopButton->setEnabled(playing || paused);
	mPauseButton->setVisible(playing && !paused);
	mMuteButton->setEnabled(true);
	mMuteButton->setVisible(!muted);
	mUnmuteButton->setVisible(muted);
	mVolumeSlider->setEnabled(true);
	mVolumeSlider->setValue(volume);
	mZoomButton->setVisible(include_zoom && !is_zoomed);
	mUnzoomButton->setVisible(include_zoom && is_zoomed);
}

void LLFloaterNearByMedia::showDisabledControls()
{
	mPlayButton->setVisible(true);
	mPlayButton->setEnabled(false);
	mPauseButton->setVisible(false);
	mStopButton->setEnabled(false);
	mMuteButton->setVisible(true);
	mMuteButton->setEnabled(false);
	mUnmuteButton->setVisible(false);
	mVolumeSlider->setEnabled(false);
	mZoomButton->setVisible(false);
	mUnzoomButton->setVisible(false);
}

LLScrollListItem* LLFloaterNearByMedia::addListItem(const LLUUID& id)
{
	// Just set up the columns: the values will be filled in by
	// updateListItem().

	LLSD row;
	row["id"] = id;

	LLSD& columns = row["columns"];

	columns[CHECKBOX_COLUMN]["column"] = "media_checkbox_ctrl";
	columns[CHECKBOX_COLUMN]["type"] = "checkbox";

	columns[PROXIMITY_COLUMN]["column"] = "media_proximity";
	columns[PROXIMITY_COLUMN]["value"] = "";

	columns[VISIBILITY_COLUMN]["column"] = "media_visibility";
	columns[VISIBILITY_COLUMN]["value"] = "";

	columns[CLASS_COLUMN]["column"] = "media_class";
	columns[CLASS_COLUMN]["value"] = "";

	columns[NAME_COLUMN]["column"] = "media_name";
	columns[NAME_COLUMN]["value"] = "";

	LLScrollListItem* itemp = mMediaList->addElement(row);
	if (itemp)
	{
		LLScrollListCheck* list_checkp =
			dynamic_cast<LLScrollListCheck*>(itemp->getColumn(CHECKBOX_COLUMN));
		if (list_checkp)
		{
			LLCheckBoxCtrl* checkp = list_checkp->getCheckBox();
			checkp->setCommitCallback(onCheckItem);
			checkp->setCallbackUserData(this);
		}
	}
	return itemp;
}

void LLFloaterNearByMedia::updateListItem(LLScrollListItem* itemp,
										  LLViewerMediaImpl* implp)
{
	if (!itemp || !implp) return;	// Paranoia

	std::string item_name;
	std::string item_tooltip;
	LLFloaterNearByMedia::MediaClass media_class = MEDIA_CLASS_ALL;

	getNameAndUrlHelper(implp, item_name, item_tooltip, mEmptyNameString);

	if (implp->getUsedOnHUD())
	{
		// Used on a HUD object. Test this case first, else the media will be
		// listed as either MEDIA_CLASS_WITHIN_PARCEL (if the parcel includes
		// the (0, 0) sim position) or MEDIA_CLASS_OUTSIDE_PARCEL.
		media_class = MEDIA_CLASS_ON_HUD_OBJECT;
	}
	else if (implp->isAttachedToAnotherAvatar())
	{
		// Attached to another avatar
		media_class = MEDIA_CLASS_ON_OTHERS;
	}
	else if (implp->isInAgentParcel())
	{
		// inside parcel
		media_class = MEDIA_CLASS_WITHIN_PARCEL;
	}
	else
	{
		// Outside agent parcel
		media_class = MEDIA_CLASS_OUTSIDE_PARCEL;
	}

	updateListItem(itemp, item_name, item_tooltip, implp->getProximity(),
				   implp->isMediaDisabled(), implp->hasMedia(),
				   implp->isMediaTimeBased() && implp->isMediaPlaying(),
				   media_class, implp->hasFocus());
}

void LLFloaterNearByMedia::updateListItem(LLScrollListItem* item,
										  const std::string& item_name,
										  const std::string& item_tooltip,
										  S32 proximity,
										  bool is_disabled,
										  bool has_media,
										  bool is_time_based_and_playing,
										  LLFloaterNearByMedia::MediaClass media_class,
										  bool focused)
{
	LLScrollListCell* cell = item->getColumn(PROXIMITY_COLUMN);
	if (cell)
	{
		// Since we are forced to sort by text, encode sort order as string
		std::string proximity_string = llformat("%d", proximity);
		std::string old_proximity_string = cell->getValue().asString();
		if (proximity_string != old_proximity_string)
		{
			cell->setValue(proximity_string);
			mMediaList->setSorted(false);
		}
	}

	cell = item->getColumn(CHECKBOX_COLUMN);
	if (cell)
	{
		cell->setValue(!is_disabled);
	}

	cell = item->getColumn(VISIBILITY_COLUMN);
	if (cell)
	{
		S32 old_visibility = cell->getValue();
		// *HACK: force parcel audio to appear first, parcel media second, then
		//        other media.
		LLUUID media_id = item->getUUID();
		S32 new_visibility = -1;
		if (media_id == PARCEL_AUDIO_LIST_ITEM_UUID)
		{
			new_visibility = 3;
		}
		else if (media_id == PARCEL_MEDIA_LIST_ITEM_UUID)
		{
			new_visibility = 2;
		}
		else if (has_media)
		{
			new_visibility = 1;
		}
		else if (is_disabled)
		{
			new_visibility = 0;
		}

		cell->setValue(llformat("%d", new_visibility));
		if (new_visibility != old_visibility)
		{
			mMediaList->setSorted(false);
		}
	}

	cell = item->getColumn(NAME_COLUMN);
	if (cell)
	{
		std::string name = item_name;
		std::string old_name = cell->getValue().asString();
		if (has_media)
		{
			name += " " + mPlayingString;
		}
		if (name != old_name)
		{
			cell->setValue(name);
		}
		item->setToolTip(item_tooltip);

		// *TODO: Make these font styles/colors configurable via XUI
		U8 font_style = LLFontGL::NORMAL;
		if (!has_media)
		{
			font_style |= LLFontGL::ITALIC;
		}
		else if (focused)
		{
			font_style |= LLFontGL::BOLD;
		}

		LLColor4 font_color = LLColor4::black;
		if (media_class == MEDIA_CLASS_ON_HUD_OBJECT)
		{
			font_color = LLColor4::blue;
		}
		else if (media_class == MEDIA_CLASS_ON_OTHERS)
		{
			font_color = LLColor4::red2;
		}
		else if (media_class == MEDIA_CLASS_OUTSIDE_PARCEL)
		{
			font_color = LLColor4::orange;
		}
		else if (is_time_based_and_playing)
		{
			font_color = LLColor4::green3;
		}

		LLScrollListText* text_cell = dynamic_cast<LLScrollListText*>(cell);
		if (text_cell)
		{
			text_cell->setFontStyle(font_style);
			text_cell->setColor(font_color);
		}
	}

	cell = item->getColumn(CLASS_COLUMN);
	if (cell)
	{
		// TODO: clean this up!
		cell->setValue(llformat("%d", media_class));
	}
}

void LLFloaterNearByMedia::removeListItem(const LLUUID& id)
{
	mMediaList->deleteSingleItem(mMediaList->getItemIndex(id));
	mMediaList->updateLayout();
}

void LLFloaterNearByMedia::refreshParcelItems()
{
	// Get the filter choice.
	const LLSD& choice_llsd = mMediaTypeCombo->getSelectedValue();
	MediaClass choice = (MediaClass)choice_llsd.asInteger();

	// Only show "special parcel items" if "All" or "Within" filter
	// (and if media is "enabled")
	bool should_include = choice == MEDIA_CLASS_ALL ||
						  choice == MEDIA_CLASS_WITHIN_PARCEL;

	// Parcel Audio: add or remove it as necessary (don't show if disabled)
	if (mStreamingMusic && should_include &&
		LLViewerParcelMedia::hasParcelAudio())
	{
		// Yes, there is parcel audio.
		if (!mParcelAudioItem)
		{
			mParcelAudioItem = addListItem(PARCEL_AUDIO_LIST_ITEM_UUID);
			mMediaList->setSorted(false);
		}
	}
	else if (mParcelAudioItem)
	{
		removeListItem(PARCEL_AUDIO_LIST_ITEM_UUID);
		mParcelAudioItem = NULL;
		mMediaList->setSorted(false);
	}

	// ... then update it
	if (mParcelAudioItem)
	{
		bool is_playing = LLViewerParcelMedia::isParcelAudioPlaying();
		std::string url = LLViewerParcelMedia::getParcelAudioURL();

		updateListItem(mParcelAudioItem, mParcelAudioName, url,
					   -2, // Proximity before Parcel Media and anything else
					   !is_playing, is_playing, is_playing,
					   MEDIA_CLASS_ALL, false);
	}

	// Then Parcel Media: add or remove it as necessary
	if (mSteamingMedia && should_include &&
		LLViewerParcelMedia::hasParcelMedia())
	{
		// Yes, there is parcel media.
		if (!mParcelMediaItem)
		{
			mParcelMediaItem = addListItem(PARCEL_MEDIA_LIST_ITEM_UUID);
			mMediaList->setSorted(false);
		}
	}
	else if (mParcelMediaItem)
	{
		removeListItem(PARCEL_MEDIA_LIST_ITEM_UUID);
		mParcelMediaItem = NULL;
		mMediaList->setSorted(false);
	}

	// ... then update it
	if (mParcelMediaItem)
	{
		std::string name, url, tooltip;
		getNameAndUrlHelper(LLViewerParcelMedia::getParcelMedia(), name, url,
							"");
		if (name.empty() || name == url)
		{
			tooltip = url;
		}
		else
		{
			tooltip = name + " : " + url;
		}
		LLViewerMediaImpl* implp = LLViewerParcelMedia::getParcelMedia();
		updateListItem(mParcelMediaItem, mParcelMediaName, tooltip,
					   -1, // Proximity closer than anything but Parcel Audio
					   !implp || implp->isMediaDisabled(),
					   implp && !LLViewerParcelMedia::getURL().empty(),
					   implp && implp->isMediaTimeBased() &&
					   implp->isMediaPlaying(),
					   MEDIA_CLASS_ALL, implp && implp->hasFocus());
	}
}

void LLFloaterNearByMedia::refreshMediaURL()
{
	LLUUID media_id = mMediaList->getValue().asUUID();
	if (media_id.notNull() &&
 		// Do not allow to change parcel audio and media URL
		// *TODO: allow to do it for the parcel owner/managers ?
		media_id != PARCEL_AUDIO_LIST_ITEM_UUID &&
		media_id != PARCEL_MEDIA_LIST_ITEM_UUID)
	{
		LLViewerMediaImpl* implp =
			LLViewerMedia::getMediaImplFromTextureID(media_id);
		if (implp && !implp->isParcelMedia() && implp->hasMedia() &&
			!implp->isMediaDisabled())
		{
			std::string url = implp->getCurrentMediaURL();
			if (url != mPreviousURL)
			{
				mPreviousURL = url;
				mMediaURLEditor->setText(url);
			}
			mMediaURLText->setEnabled(true);
			mMediaURLEditor->setEnabled(true);
			return;
		}
	}

	// Not editable or NULL impl
	mMediaURLText->setEnabled(false);
	mMediaURLEditor->clear();
	mMediaURLEditor->setEnabled(false);
	mPreviousURL.clear();
}

void LLFloaterNearByMedia::refreshList(bool fresh)
{
	refreshParcelItems();

	LLViewerMediaImpl* implp;

	// Iterate over the implements list, creating rows as necessary.
	for (LLViewerMedia::impl_list::iterator
			iter = LLViewerMedia::getPriorityList().begin(),
			end = LLViewerMedia::getPriorityList().end();
		 iter != end; ++iter)
	{
		implp = *iter;

		if (implp && fresh)
		{
			implp->setInNearbyMediaList(false);
		}

		if (implp && !implp->isParcelMedia())
		{
			const LLUUID& media_id = implp->getMediaTextureID();
			S32 proximity = implp->getProximity();
			if (proximity < 0 || !shouldShow(implp))
			{
				if (implp->getInNearbyMediaList())
				{
					// There's a row for this implp, remove it.
					removeListItem(media_id);
					implp->setInNearbyMediaList(false);
				}
			}
			else if (!implp->getInNearbyMediaList())
			{
				// We do not have a row for this impl, add one.
				if (addListItem(media_id))
				{
					implp->setInNearbyMediaList(true);
				}
			}
		}
	}

	mEnableAllButton->setEnabled((mStreamingMusic || mSteamingMedia) &&
								 (LLViewerMedia::isAnyMediaDisabled() ||
								  (LLViewerParcelMedia::hasParcelMedia() &&
								   !LLViewerParcelMedia::isParcelMediaPlaying()) ||
								  (LLViewerParcelMedia::hasParcelAudio() &&
								   !LLViewerParcelMedia::isParcelAudioPlaying())));

	mDisableAllButton->setEnabled((mStreamingMusic || mSteamingMedia) &&
								  (LLViewerMedia::isAnyMediaEnabled() ||
								   LLViewerMedia::isAnyMediaShowing() ||
								   LLViewerParcelMedia::isParcelMediaPlaying() ||
								   LLViewerParcelMedia::isParcelAudioPlaying()));

	// Iterate over the rows in the control, updating ones whose impl exists,
	// and deleting ones whose impl has gone away.
	LLScrollListCtrl::items_vec_t items = mMediaList->getAllData();
	for (size_t i = 0, count = items.size(); i < count; ++i)
	{
		LLScrollListItem* itemp = items[i];	// Cannot be NULL. HB
		const LLUUID& row_id = itemp->getUUID();
		if (row_id != PARCEL_MEDIA_LIST_ITEM_UUID &&
			row_id != PARCEL_AUDIO_LIST_ITEM_UUID)
		{
			implp = LLViewerMedia::getMediaImplFromTextureID(row_id);
			if (implp)
			{
				updateListItem(itemp, implp);
			}
			else
			{
				// This item's impl has been deleted: remove the row. Removing
				// it would not throw off our iteration, since we have a local
				// copy of the array. We just need to make sure we do not
				// access this item after the delete.
				removeListItem(row_id);
			}
		}
	}
	refreshMediaURL();
}

bool LLFloaterNearByMedia::shouldShow(LLViewerMediaImpl* implp)
{
	if (!implp) return false;

	const LLSD& choice_llsd = mMediaTypeCombo->getSelectedValue();
	MediaClass choice = (MediaClass)choice_llsd.asInteger();

	switch (choice)
	{
		case MEDIA_CLASS_ALL:
			return true;

		case MEDIA_CLASS_WITHIN_PARCEL:
			return implp->isInAgentParcel() && !implp->getUsedOnHUD();

		case MEDIA_CLASS_OUTSIDE_PARCEL:
			return !implp->isInAgentParcel() && !implp->getUsedOnHUD();

		case MEDIA_CLASS_ON_OTHERS:
			return implp->isAttachedToAnotherAvatar();

		case MEDIA_CLASS_ON_HUD_OBJECT:
			return implp->getUsedOnHUD();

		default:
			break;
	}
	return true;
}

void LLFloaterNearByMedia::mediaEnable(const LLUUID& rowid, bool enable)
{
	if (rowid == PARCEL_AUDIO_LIST_ITEM_UUID)
	{
		if (enable)
		{
			LLViewerParcelMedia::playMusic();
		}
		else
		{
			LLViewerParcelMedia::stopMusic();
		}
		return;
	}

	if (rowid == PARCEL_MEDIA_LIST_ITEM_UUID)
	{
		if (enable)
		{
			LLViewerParcelMedia::play();
		}
		else
		{
			LLViewerParcelMedia::stop();
		}
		return;
	}

	LLViewerMediaImpl* implp = LLViewerMedia::getMediaImplFromTextureID(rowid);
	if (implp)
	{
		implp->setDisabled(!enable, true);
	}
}

// static
void LLFloaterNearByMedia::onSelectNewMedia(LLUICtrl*, void* datap)
{
	LLFloaterNearByMedia* self = (LLFloaterNearByMedia*)datap;
	if (self)
	{
		self->refreshMediaURL();
	}
}

//static
void LLFloaterNearByMedia::onCheckItem(LLUICtrl* ctrlp, void* datap)
{
	LLFloaterNearByMedia* self = (LLFloaterNearByMedia*)datap;
	LLCheckBoxCtrl* checkp = (LLCheckBoxCtrl*)ctrlp;
	if (self && checkp)
	{
		LLUUID media_id = self->mMediaList->getValue().asUUID();
		if (checkp)
		{
			self->mediaEnable(media_id, checkp->getValue());
		}
	}
}

//static
void LLFloaterNearByMedia::onCommitMediaType(LLUICtrl*, void* datap)
{
	LLFloaterNearByMedia* self = (LLFloaterNearByMedia*)datap;
	if (self)
	{
		self->refreshList();
	}
}

//static
void LLFloaterNearByMedia::onCommitMediaURL(LLUICtrl* ctrl, void* datap)
{
	LLFloaterNearByMedia* self = (LLFloaterNearByMedia*)datap;
	if (self && ctrl)
	{
		// Get the new URL:
		std::string url = ctrl->getValue().asString();

		// Force a refresh of the input line:
		ctrl->clear();
		self->mPreviousURL.clear();

		LLUUID media_id = self->mMediaList->getValue().asUUID();
		if (media_id != PARCEL_AUDIO_LIST_ITEM_UUID &&
			media_id != PARCEL_MEDIA_LIST_ITEM_UUID &&
			!url.empty())
		{
			LLViewerMediaImpl* implp =
				LLViewerMedia::getMediaImplFromTextureID(media_id);
			if (implp)
			{
				// Force a MIME type rediscovery
				implp->navigateTo(url, "", true);
			}
		}
	}
}

//static
void LLFloaterNearByMedia::onClickSelectedMediaPlay(void* datap)
{
	LLFloaterNearByMedia* self = (LLFloaterNearByMedia*)datap;
	if (!self) return;	// Paranoia

	LLUUID media_id = self->mMediaList->getValue().asUUID();
	// First enable it
	self->mediaEnable(media_id);

	LLViewerMediaImpl* implp;
	if (media_id == PARCEL_MEDIA_LIST_ITEM_UUID)
	{
		implp = (LLViewerMediaImpl*)LLViewerParcelMedia::getParcelMedia();
	}
	else
	{
		implp = LLViewerMedia::getMediaImplFromTextureID(media_id);
	}
	if (implp)
	{
		// Let it be known that the user requested to play this media.
		implp->mNavigateUserRequest = true;
	}

	// Special code to make play "unpause" if time-based and playing
	if (implp && media_id != PARCEL_AUDIO_LIST_ITEM_UUID)
	{
		if (implp->isMediaTimeBased() && implp->isMediaPaused())
		{
			// It is really a time-based media that was paused, so un-pause it.
			implp->play();
		}
		else if (implp->isParcelMedia())
		{
			LLParcel* parcelp = gViewerParcelMgr.getAgentParcel();
			if (parcelp)
			{
				LLViewerParcelMedia::playMedia(parcelp);
			}
		}
	}
}

//static
void LLFloaterNearByMedia::onClickSelectedMediaPause(void* datap)
{
	LLFloaterNearByMedia* self = (LLFloaterNearByMedia*)datap;
	if (self)
	{
		LLUUID media_id = self->mMediaList->getValue().asUUID();
		if (media_id == PARCEL_AUDIO_LIST_ITEM_UUID)
		{
			LLViewerParcelMedia::pauseMusic();
		}
		else if (media_id == PARCEL_MEDIA_LIST_ITEM_UUID)
		{
			LLViewerParcelMedia::pause();
		}
		else
		{
			LLViewerMediaImpl* implp =
				LLViewerMedia::getMediaImplFromTextureID(media_id);
			if (implp && implp->isMediaTimeBased() && implp->isMediaPlaying())
			{
				implp->pause();
			}
		}
	}
}

//static
void LLFloaterNearByMedia::onClickSelectedMediaStop(void* datap)
{
	LLFloaterNearByMedia* self = (LLFloaterNearByMedia*)datap;
	if (self)
	{
		self->mediaEnable(self->mMediaList->getValue().asUUID(), false);
	}
}

//static
void LLFloaterNearByMedia::onCommitSelectedMediaVolume(LLUICtrl*, void* datap)
{
	LLFloaterNearByMedia* self = (LLFloaterNearByMedia*)datap;
	if (!self) return;	// Paranoia

	F32 volume = self->mVolumeSlider->getValueF32();
	LLUUID media_id = self->mMediaList->getValue().asUUID();
	if (media_id == PARCEL_AUDIO_LIST_ITEM_UUID)
	{
		gSavedSettings.setF32("AudioLevelMusic", volume);
	}
	else
	{
		LLViewerMediaImpl* implp;
		if (media_id == PARCEL_MEDIA_LIST_ITEM_UUID)
		{
			implp = (LLViewerMediaImpl*)LLViewerParcelMedia::getParcelMedia();
		}
		else
		{
			implp = LLViewerMedia::getMediaImplFromTextureID(media_id);
		}
		if (implp)
		{
			implp->setVolume(volume);
		}
	}
}

//static
void LLFloaterNearByMedia::onClickSelectedMediaMute(void* datap)
{
	LLFloaterNearByMedia* self = (LLFloaterNearByMedia*)datap;
	if (!self) return;

	LLUUID media_id = self->mMediaList->getValue().asUUID();
	if (media_id == PARCEL_AUDIO_LIST_ITEM_UUID)
	{
		gSavedSettings.setBool("MuteMusic", true);
	}
	else
	{
		LLViewerMediaImpl* implp;
		if (media_id == PARCEL_MEDIA_LIST_ITEM_UUID)
		{
			implp = (LLViewerMediaImpl*)LLViewerParcelMedia::getParcelMedia();
		}
		else
		{
			implp = LLViewerMedia::getMediaImplFromTextureID(media_id);
		}
		if (implp)
		{
			implp->setMute();
		}
	}
}

//static
void LLFloaterNearByMedia::onClickSelectedMediaUnmute(void* datap)
{
	LLFloaterNearByMedia* self = (LLFloaterNearByMedia*)datap;
	if (!self) return;

	LLUUID media_id = self->mMediaList->getValue().asUUID();
	if (media_id == PARCEL_AUDIO_LIST_ITEM_UUID)
	{
		gSavedSettings.setBool("MuteMusic", false);
	}
	else
	{
		LLViewerMediaImpl* implp;
		if (media_id == PARCEL_MEDIA_LIST_ITEM_UUID)
		{
			implp = (LLViewerMediaImpl*)LLViewerParcelMedia::getParcelMedia();
		}
		else
		{
			implp = LLViewerMedia::getMediaImplFromTextureID(media_id);
		}
		if (implp)
		{
			F32 slider_volume = self->mVolumeSlider->getValueF32();
			if (slider_volume == 0.f)
			{
				implp->setMute(false);
				self->mVolumeSlider->setValue(implp->getVolume());
			}
			else
			{
				implp->setVolume(slider_volume);
			}
		}
	}
}

//static
void LLFloaterNearByMedia::onClickSelectedMediaZoom(void* datap)
{
	LLFloaterNearByMedia* self = (LLFloaterNearByMedia*)datap;
	if (self)
	{
		LLUUID media_id = self->mMediaList->getValue().asUUID();
		if (media_id.notNull() && media_id != PARCEL_AUDIO_LIST_ITEM_UUID &&
			media_id != PARCEL_MEDIA_LIST_ITEM_UUID)
		{
			LLViewerMediaFocus::getInstance()->focusZoomOnMedia(media_id);
		}
	}
}

//static
void LLFloaterNearByMedia::onClickSelectedMediaUnzoom(void*)
{
	LLViewerMediaFocus::getInstance()->unZoom();
}

//static
void LLFloaterNearByMedia::onClickEnableAll(void*)
{
	LLViewerMedia::setAllMediaEnabled(true);
}

//static
void LLFloaterNearByMedia::onClickDisableAll(void*)
{
	LLViewerMedia::setAllMediaEnabled(false);
}

//static
void LLFloaterNearByMedia::onOpenPrefs(void*)
{
	// To select the Media sub-tab:
	gSavedSettings.setS32("LastMediaPrefTab", 1);
	// Open the Preferences with the Audio & Media tab selected
	LLFloaterPreference::openInTab(LLFloaterPreference::AUDIO_AND_MEDIA_TAB);
}

// static
void LLFloaterNearByMedia::getNameAndUrlHelper(LLViewerMediaImpl* implp,
											   std::string& name,
											   std::string& url,
											   const std::string& default_name)
{
	if (!implp) return;

	name = implp->getName();
	// This is the URL the media impl actually has loaded
	url = implp->getCurrentMediaURL();
	if (url.empty())
	{
		// This is the current URL from the media data
		url = implp->getMediaEntryURL();
	}
	if (url.empty())
	{
		// This is the home URL from the media data
		url = implp->getHomeURL();
	}
	if (name.empty())
	{
		name = url;
	}
	if (name.empty())
	{
		name = default_name;
	}
}
