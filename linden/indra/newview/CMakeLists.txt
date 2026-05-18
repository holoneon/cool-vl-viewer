# -*- cmake -*-

project(viewer)

include(00-Common)
include(Boost)
include(CEFPlugin)
include(ELFIO)
include(FindOpenGL)
include(Glib)
include(GLOD)
include(Hunspell)
include(jemalloc)
include(Json)
include(LLWindow)
include(LLAppearance)
include(LLAudio)
include(LLCharacter)
include(LLCommon)
include(LLFilesystem)
include(LLGltf)
include(LLImage)
include(LLInventory)
include(LLMath)
include(LLMessage)
include(LLPlugin)
include(LLPrimitive)
include(LLRender)
include(LLUI)
include(LLXML)
include(Linking)
include(LUA)
include(NDOF)
include(PNG)
include(Python)
include(ViewerMiscLibs)
include(ZLIB)

use_prebuilt_binary(tinyexr)
use_prebuilt_binary(artwork-common)
use_prebuilt_binary(dictionaries)
if (WINDOWS)
  use_prebuilt_binary(runtime-libraries)
endif (WINDOWS)

set(viewer_SOURCE_FILES
  llagent.cpp
  llagentpilot.cpp
  llagentwearables.cpp
  llaisapi.cpp
  llappcorehttp.cpp
  llappearancemgr.cpp
  llappviewer.cpp
  llaudiosourcevo.cpp
  llavataractions.cpp
  llavatarproperties.cpp
  llavatartracker.cpp
  llchatbar.cpp
  llcloud.cpp
  llcolorswatch.cpp
  llcommandhandler.cpp
  llcurrencyuimanager.cpp
  lldebugview.cpp
  lldrawable.cpp
  lldrawpoolalpha.cpp
  lldrawpoolavatar.cpp
  lldrawpoolbump.cpp
  lldrawpool.cpp
  lldrawpoolmaterials.cpp
  lldrawpoolsimple.cpp
  lldrawpoolsky.cpp
  lldrawpoolterrain.cpp
  lldrawpooltree.cpp
  lldrawpoolwater.cpp
  lldrawpoolwlsky.cpp
  lldynamictexture.cpp
  llenvironment.cpp
  llenvsettings.cpp
  lleventnotifier.cpp
  lleventpoll.cpp
  llexperiencelog.cpp
  llface.cpp
  llfasttimerview.cpp
  llfeaturemanager.cpp
  llfetchedgltfmaterial.cpp
  llfirstuse.cpp
  llflexibleobject.cpp
  llfloaterabout.cpp
  llfloateractivespeakers.cpp
  llfloateranimpreview.cpp
  hbfloaterareasearch.cpp
  llfloaterauction.cpp
  llfloateravatarinfo.cpp
  llfloateravatarpicker.cpp
  llfloateravatartextures.cpp
  llfloaterbeacons.cpp
  llfloaterbulkpermission.cpp
  hbfloaterbump.cpp
  llfloaterbuycontents.cpp
  llfloaterbuy.cpp
  llfloaterbuycurrency.cpp
  llfloaterbuyland.cpp
  llfloatercamera.cpp
  llfloaterchat.cpp
  llfloaterchatterbox.cpp
  llfloaterclassified.cpp
  llfloatercolorpicker.cpp
  llfloatercustomize.cpp
  llfloaterdebugsettings.cpp
  hbfloaterdebugtags.cpp
  llfloaterdisplayname.cpp
  hbfloatereditenvsettings.cpp
  llfloatereditui.cpp
  llfloaterevent.cpp
  llfloaterexperiencepicker.cpp
  llfloaterexperienceprofile.cpp
  llfloaterexperiences.cpp
  llfloaterfriends.cpp
  llfloatergesture.cpp
  llfloatergodtools.cpp
  llfloatergroupbulkban.cpp
  llfloatergroupinfo.cpp
  llfloatergroupinvite.cpp
  llfloatergroups.cpp
  hbfloatergrouptitles.cpp
  llfloaterim.cpp
  llfloaterimagepreview.cpp
  llfloaterinspect.cpp
  llfloaterinventory.cpp
  hbfloaterinvitemspicker.cpp
  llfloaterjoystick.cpp
  llfloaterlagmeter.cpp
  llfloaterland.cpp
  llfloaterlandholdings.cpp
  hbfloatermakenewoutfit.cpp
  llfloatermarketplace.cpp
  llfloatermediabrowser.cpp
  slfloatermediafilter.cpp
  llfloatermediasettings.cpp
  llfloaterminimap.cpp
  llfloatermodelpreview.cpp
  llfloatermove.cpp
  llfloatermute.cpp
  llfloaternearbymedia.cpp
  llfloaternewim.cpp
  llfloaternotificationsconsole.cpp
  llfloaterobjectiminfo.cpp
  llfloaterobjectweights.cpp
  llfloateropenobject.cpp
  llfloaterparcel.cpp
  llfloaterpathfindingcharacters.cpp
  llfloaterpathfindinglinksets.cpp
  llfloaterpathfindingobjects.cpp
  llfloaterpay.cpp
  llfloaterperms.cpp
  llfloaterpostcard.cpp
  llfloaterpreference.cpp
  llfloaterproperties.cpp
  hbfloaterradar.cpp
  llfloaterregiondebugconsole.cpp
  llfloaterregioninfo.cpp
  llfloaterreporter.cpp
  hbfloaterrlv.cpp
  llfloatersearchreplace.cpp
  llfloaterscriptdebug.cpp
  llfloaterscriptqueue.cpp
  hbfloatersearch.cpp
  llfloatersellland.cpp
  llfloatersnapshot.cpp
  hbfloatersoundslist.cpp
  llfloaterstats.cpp
  llfloatertelehub.cpp
  hbfloaterteleporthistory.cpp
  hbfloatertextinput.cpp
  hbfloaterthumbnail.cpp
  llfloatertools.cpp
  llfloatertopobjects.cpp
  llfloatertos.cpp
  hbfloateruploadasset.cpp
  llfloaterurldisplay.cpp
  llfloaterurlentry.cpp
  llfloatervoicedevicesettings.cpp
  llfloaterwindlight.cpp
  llfloaterworldmap.cpp
  llfolderview.cpp
  llfollowcam.cpp
  llgesturemgr.cpp
  llgltfmateriallist.cpp
  llgltfmaterialpreview.cpp
  llgltfscenemanager.cpp
  llgridmanager.cpp
  llgroupmgr.cpp
  llgroupnotify.cpp
  llhoverview.cpp
  llhudeffect.cpp
  llhudeffectlookat.cpp
  llhudeffectpointat.cpp
  llhudeffectspiral.cpp
  llhudicon.cpp
  llhudmanager.cpp
  llhudobject.cpp
  llhudtext.cpp
  llhudview.cpp
  llimmgr.cpp
  llinventoryactions.cpp
  llinventorybridge.cpp
  hbinventoryclipboard.cpp
  llinventoryicon.cpp
  llinventorymodel.cpp
  llinventorymodelfetch.cpp
  lljoystickbutton.cpp
  lllandmarklist.cpp
  lllocalbitmaps.cpp
  lllocalgltfmaterials.cpp
  lllogchat.cpp
  llmanip.cpp
  llmaniprotate.cpp
  llmanipscale.cpp
  llmaniptranslate.cpp
  llmarketplacefunctions.cpp
  llmaterialmgr.cpp
  llmediactrl.cpp
  llmediadataclient.cpp
  llmediaremotectrl.cpp
  llmeshrepository.cpp
  llmorphview.cpp
  llmutelist.cpp
  llnotify.cpp
  hbobjectbackup.cpp
  lloverlaybar.cpp
  llpanelaudiovolume.cpp
  llpanelavatar.cpp
  llpanelclassified.cpp
  llpanelcontents.cpp
  llpaneldirbrowser.cpp
  llpaneldirclassified.cpp
  llpaneldirevents.cpp
  llpaneldirfind.cpp
  llpaneldirgroups.cpp
  llpaneldirland.cpp
  llpaneldirpeople.cpp
  llpaneldirplaces.cpp
  llpaneleditwearable.cpp
  llpanelenvsettings.cpp
  llpanelevent.cpp
  llpanelexperiencelisteditor.cpp
  llpanelexperiencelog.cpp
  llpanelface.cpp
  hbpanelgrids.cpp
  llpanelgroup.cpp
  llpanelgroupbulk.cpp
  llpanelgroupexperiences.cpp
  llpanelgroupgeneral.cpp
  llpanelgrouplandmoney.cpp
  llpanelgroupnotices.cpp
  llpanelgrouproles.cpp
  llpanelinventory.cpp
  llpanelland.cpp
  llpanellandaudio.cpp
  hbpanellandenvironment.cpp
  llpanellandmedia.cpp
  llpanellogin.cpp
  llpanelminimap.cpp
  llpanelobject.cpp
  llpanelpermissions.cpp
  llpanelpick.cpp
  llpanelplace.cpp
  llpanelvolume.cpp
  llpanelworldmap.cpp
  llparcelselection.cpp
  llpatchvertexarray.cpp
  llpathfindingcharacter.cpp
  llpathfindingcharacterlist.cpp
  llpathfindinglinkset.cpp
  llpathfindinglinksetlist.cpp
  llpathfindingmanager.cpp
  llpathfindingnavmesh.cpp
  llpathfindingnavmeshstatus.cpp
  llpathfindingobject.cpp
  llpathfindingobjectlist.cpp
  llphysicsmotion.cpp
  llpipeline.cpp
  llprefschat.cpp
  hbprefscool.cpp
  llprefsgeneral.cpp
  llprefsgraphics.cpp
  llprefsim.cpp
  llprefsinput.cpp
  llprefsmedia.cpp
  llprefsnetwork.cpp
  llprefsnotifications.cpp
  llprefsskins.cpp
  llprefsvoice.cpp
  hbpreprocessor.cpp
  llpreview.cpp
  llpreviewanim.cpp
  llpreviewgesture.cpp
  llpreviewlandmark.cpp
  llpreviewmaterial.cpp
  llpreviewnotecard.cpp
  llpreviewscript.cpp
  llpreviewsound.cpp
  llpreviewtexture.cpp
  llproductinforequest.cpp
  llprogressview.cpp
  llreflectionmapmanager.cpp
  mkrlinterface.cpp
  llsavedlogins.cpp
  llscrollingpanelparam.cpp
  llselectmgr.cpp
  llsky.cpp
  llslurl.cpp
  llspatialpartition.cpp
  llstartup.cpp
  llstatusbar.cpp
  llsurface.cpp
  llsurfacepatch.cpp
  lltexturecache.cpp
  lltexturectrl.cpp
  lltexturefetch.cpp
  lltextureview.cpp
  lltinygltfhelper.cpp
  qltoolalign.cpp
  lltoolbar.cpp
  lltoolbrushland.cpp
  lltoolcomp.cpp
  lltool.cpp
  lltooldraganddrop.cpp
  lltoolface.cpp
  lltoolfocus.cpp
  lltoolgrab.cpp
  lltoolmgr.cpp
  lltoolobjpicker.cpp
  lltoolpie.cpp
  lltoolpipette.cpp
  lltoolplacer.cpp
  lltoolselect.cpp
  lltoolselectland.cpp
  lltoolselectrect.cpp
  lltoolview.cpp
  lltracker.cpp
  llurldispatcher.cpp
  llvelocitybar.cpp
  llviewerassetstorage.cpp
  llviewerassetupload.cpp
  llvieweraudio.cpp
  hbviewerautomation.cpp
  llviewercamera.cpp
  llviewercontrol.cpp
  llviewerdisplay.cpp
  llviewerdisplayname.cpp
  llviewerfoldertype.cpp
  llviewergesture.cpp
  llviewerinventory.cpp
  llviewerjointattachment.cpp
  llviewerjoint.cpp
  llviewerjointmesh.cpp
  llviewerjoystick.cpp
  llviewerkeyboard.cpp
  llviewermedia.cpp
  llviewermediafocus.cpp
  llviewermenu.cpp
  llviewermessage.cpp
  llviewerobject.cpp
  llviewerobjectexport.cpp
  llviewerobjectlist.cpp
  llvieweroctree.cpp
  llviewerparcelmedia.cpp
  llviewerparcelmgr.cpp
  llviewerparceloverlay.cpp
  llviewerpartsim.cpp
  llviewerpartsource.cpp
  llviewerregion.cpp
  llviewershadermgr.cpp
  llviewerstats.cpp
  llviewertexlayer.cpp
  llviewertexteditor.cpp
  llviewertextureanim.cpp
  llviewertexture.cpp
  llviewertexturelist.cpp
  llviewerthrottle.cpp
  llviewerwearable.cpp
  llviewerwindow.cpp
  llvisualparamhint.cpp
  llvlcomposition.cpp
  llvlmanager.cpp
  llvoavatar.cpp
  llvoavatarpuppet.cpp
  llvoavatarself.cpp
  llvocache.cpp
  llvoclouds.cpp
  llvograss.cpp
  llvoicechannel.cpp
  llvoiceclient.cpp
  llvoiceremotectrl.cpp
  llvoicevisualizer.cpp
  llvoicewebrtc.cpp
  llvoinventorylistener.cpp
  llvopartgroup.cpp
  llvosky.cpp
  llvosurfacepatch.cpp
  llvotree.cpp
  llvovolume.cpp
  llvowater.cpp
  llvowlsky.cpp
  llwearablelist.cpp
  llweb.cpp
  llwind.cpp
  llwlskyparammgr.cpp
  llwlwaterparammgr.cpp
  llworld.cpp
  llworldmap.cpp
)

# This gets renamed in the packaging step
set(VIEWER_BINARY_NAME "CoolVLViewer" CACHE STRING
    "The name of the viewer executable to create.")

set(viewer_HEADER_FILES
  CMakeLists.txt

  llagent.h
  llagentpilot.h
  llagentwearables.h
  llaisapi.h
  llappcorehttp.h
  llappearancemgr.h
  llappviewer.h
  llaudiosourcevo.h
  llavataractions.h
  llavatarproperties.h
  llavatartracker.h
  llchat.h
  llchatbar.h
  llcloud.h
  llcolorswatch.h
  llcommandhandler.h
  llcurrencyuimanager.h
  lldebugview.h
  lldrawable.h
  lldrawpool.h
  lldrawpoolalpha.h
  lldrawpoolavatar.h
  lldrawpoolbump.h
  lldrawpoolmaterials.h
  lldrawpoolsimple.h
  lldrawpoolsky.h
  lldrawpoolterrain.h
  lldrawpooltree.h
  lldrawpoolwater.h
  lldrawpoolwlsky.h
  lldynamictexture.h
  llenvironment.h
  llenvsettings.h
  lleventnotifier.h
  lleventpoll.h
  llexperiencelog.h
  llface.h
  llfasttimerview.h
  llfeaturemanager.h
  llfetchedgltfmaterial.h
  llfirstuse.h
  llflexibleobject.h
  llfloaterabout.h
  llfloateractivespeakers.h
  llfloateranimpreview.h
  hbfloaterareasearch.h
  llfloaterauction.h
  llfloateravatarinfo.h
  llfloateravatarpicker.h
  llfloateravatartextures.h
  llfloaterbeacons.h
  llfloaterbulkpermission.h
  hbfloaterbump.h
  llfloaterbuy.h
  llfloaterbuycontents.h
  llfloaterbuycurrency.h
  llfloaterbuyland.h
  llfloatercamera.h
  llfloaterchat.h
  llfloaterchatterbox.h
  llfloaterclassified.h
  llfloatercolorpicker.h
  llfloatercustomize.h
  llfloaterdebugsettings.h
  hbfloaterdebugtags.h
  llfloaterdisplayname.h
  hbfloatereditenvsettings.h
  llfloatereditui.h
  llfloaterevent.h
  llfloaterexperiencepicker.h
  llfloaterexperienceprofile.h
  llfloaterexperiences.h
  llfloaterfriends.h
  llfloatergesture.h
  llfloatergodtools.h
  llfloatergroupbulkban.h
  llfloatergroupinfo.h
  llfloatergroupinvite.h
  llfloatergroups.h
  hbfloatergrouptitles.h
  llfloaterim.h
  llfloaterimagepreview.h
  llfloaterinspect.h
  llfloaterinventory.h
  hbfloaterinvitemspicker.h
  llfloaterjoystick.h
  llfloaterlagmeter.h
  llfloaterland.h
  llfloaterlandholdings.h
  hbfloatermakenewoutfit.h
  llfloatermarketplace.h
  llfloatermediabrowser.h
  slfloatermediafilter.h
  llfloatermediasettings.h
  llfloaterminimap.h
  llfloatermodelpreview.h
  llfloatermove.h
  llfloatermute.h
  llfloaternearbymedia.h
  llfloaternewim.h
  llfloaternotificationsconsole.h
  llfloaterobjectiminfo.h
  llfloaterobjectweights.h
  llfloateropenobject.h
  llfloaterparcel.h
  llfloaterpathfindingcharacters.h
  llfloaterpathfindinglinksets.h
  llfloaterpathfindingobjects.h
  llfloaterpay.h
  llfloaterpostcard.h
  llfloaterpreference.h
  llfloaterperms.h
  llfloaterproperties.h
  hbfloaterradar.h
  llfloaterregiondebugconsole.h
  llfloaterregioninfo.h
  llfloaterreporter.h
  hbfloaterrlv.h
  llfloatersearchreplace.h
  llfloaterscriptdebug.h
  llfloaterscriptqueue.h
  hbfloatersearch.h
  llfloatersellland.h
  llfloatersnapshot.h
  hbfloatersoundslist.h
  llfloaterstats.h
  llfloatertelehub.h
  hbfloaterteleporthistory.h
  hbfloatertextinput.h
  hbfloaterthumbnail.h
  llfloatertools.h
  llfloatertopobjects.h
  llfloatertos.h
  hbfloateruploadasset.h
  llfloaterurldisplay.h
  llfloaterurlentry.h
  llfloatervoicedevicesettings.h
  llfloaterwindlight.h
  llfloaterworldmap.h
  llfolderview.h
  llfollowcam.h
  llgesturemgr.h
  llgltfmateriallist.h
  llgltfmaterialpreview.h
  llgltfscenemanager.h
  llgridmanager.h
  llgroupmgr.h
  llgroupnotify.h
  llhoverview.h
  llhudeffect.h
  llhudeffectlookat.h
  llhudeffectpointat.h
  llhudeffectspiral.h
  llhudicon.h
  llhudmanager.h
  llhudobject.h
  llhudtext.h
  llhudview.h
  llimmgr.h
  llinventoryactions.h
  llinventorybridge.h
  hbinventoryclipboard.h
  llinventoryicon.h
  llinventorymodel.h
  llinventorymodelfetch.h
  lljoystickbutton.h
  lllandmarklist.h
  lllocalbitmaps.h
  lllocalgltfmaterials.h
  lllogchat.h
  llmanip.h
  llmaniprotate.h
  llmanipscale.h
  llmaniptranslate.h
  llmarketplacefunctions.h
  llmaterialmgr.h
  llmediactrl.h
  llmediadataclient.h
  llmediaremotectrl.h
  llmeshrepository.h
  llmorphview.h
  llmutelist.h
  llnotify.h
  hbobjectbackup.h
  lloverlaybar.h
  llpanelaudiovolume.h
  llpanelavatar.h
  llpanelclassified.h
  llpanelcontents.h
  llpaneldirbrowser.h
  llpaneldirclassified.h
  llpaneldirevents.h
  llpaneldirfind.h
  llpaneldirgroups.h
  llpaneldirland.h
  llpaneldirpeople.h
  llpaneldirplaces.h
  llpaneleditwearable.h
  llpanelenvsettings.h
  llpanelevent.h
  llpanelexperiencelisteditor.h
  llpanelexperiencelog.h
  llpanelface.h
  hbpanelgrids.h
  llpanelgroup.h
  llpanelgroupbulk.h
  llpanelgroupexperiences.h
  llpanelgroupgeneral.h
  llpanelgrouplandmoney.h
  llpanelgroupnotices.h
  llpanelgrouproles.h
  llpanelinventory.h
  llpanelland.h
  llpanellandaudio.h
  hbpanellandenvironment.h
  llpanellandmedia.h
  llpanellogin.h
  llpanelminimap.h
  llpanelobject.h
  llpanelpermissions.h
  llpanelpick.h
  llpanelplace.h
  llpanelvolume.h
  llpanelworldmap.h
  llparcelselection.h
  llpatchvertexarray.h
  llpathfindingcharacter.h
  llpathfindingcharacterlist.h
  llpathfindinglinkset.h
  llpathfindinglinksetlist.h
  llpathfindingmanager.h
  llpathfindingnavmesh.h
  llpathfindingnavmeshstatus.h
  llpathfindingobject.h
  llpathfindingobjectlist.h
  llphysicsmotion.h
  llpipeline.h
  llprefschat.h
  hbprefscool.h
  llprefsgeneral.h
  llprefsgraphics.h
  llprefsim.h
  llprefsinput.h
  llprefsmedia.h
  llprefsnetwork.h
  llprefsnotifications.h
  llprefsskins.h
  llprefsvoice.h
  hbpreprocessor.h
  llpreview.h
  llpreviewanim.h
  llpreviewgesture.h
  llpreviewlandmark.h
  llpreviewmaterial.h
  llpreviewnotecard.h
  llpreviewscript.h
  llpreviewsound.h
  llpreviewtexture.h
  llproductinforequest.h
  llprogressview.h
  llreflectionmapmanager.h
  mkrlinterface.h
  llsavedlogins.h
  llscrollingpanelparam.h
  llselectmgr.h
  llsky.h
  llslurl.h
  llspatialpartition.h
  llstartup.h
  llstatusbar.h
  llsurface.h
  llsurfacepatch.h
  lltexturecache.h
  lltexturectrl.h
  lltexturefetch.h
  lltextureview.h
  lltinygltfhelper.h
  lltool.h
  qltoolalign.h
  lltoolbar.h
  lltoolbrushland.h
  lltoolcomp.h
  lltooldraganddrop.h
  lltoolface.h
  lltoolfocus.h
  lltoolgrab.h
  lltoolmgr.h
  lltoolobjpicker.h
  lltoolpie.h
  lltoolpipette.h
  lltoolplacer.h
  lltoolselect.h
  lltoolselectland.h
  lltoolselectrect.h
  lltoolview.h
  lltracker.h
  llurldispatcher.h
  llvelocitybar.h
  llviewerassetstorage.h
  llviewerassetupload.h
  llvieweraudio.h
  hbviewerautomation.h
  llviewercamera.h
  llviewercontrol.h
  llviewerdisplay.h
  llviewerdisplayname.h
  llviewerfoldertype.h
  llviewergesture.h
  llviewerinventory.h
  llviewerjoint.h
  llviewerjointattachment.h
  llviewerjointmesh.h
  llviewerjoystick.h
  llviewerkeyboard.h
  llviewermedia.h
  llviewermediafocus.h
  llviewermenu.h
  llviewermessage.h
  llviewerobject.h
  llviewerobjectexport.h
  llviewerobjectlist.h
  llvieweroctree.h
  llviewerparcelmedia.h
  llviewerparcelmgr.h
  llviewerparceloverlay.h
  llviewerpartsim.h
  llviewerpartsource.h
  llviewerprecompiledheaders.h
  llviewerregion.h
  llviewershadermgr.h
  llviewerstats.h
  llviewertexlayer.h
  llviewertexteditor.h
  llviewertextureanim.h
  llviewertexture.h
  llviewertexturelist.h
  llviewerthrottle.h
  llviewerwearable.h
  llviewerwindow.h
  llvisualparamhint.h
  llvlcomposition.h
  llvlmanager.h
  llvoavatar.h
  llvoavatarpuppet.h
  llvoavatarself.h
  llvocache.h
  llvoclouds.h
  llvograss.h
  llvoicechannel.h
  llvoiceclient.h
  llvoiceremotectrl.h
  llvoicevisualizer.h
  llvoicewebrtc.h
  llvoinventorylistener.h
  llvopartgroup.h
  llvosky.h
  llvosurfacepatch.h
  llvotree.h
  llvovolume.h
  llvowater.h
  llvowlsky.h
  llweb.h
  llwind.h
  llwlskyparammgr.h
  llwlwaterparammgr.h
  llworld.h
  llworldmap.h
  roles_constants.h
)

if (ENABLE_PUPPETRY)
  list(APPEND viewer_SOURCE_FILES llpuppetmodule.cpp llpuppetmotion.cpp)
  list(APPEND viewer_HEADER_FILES llpuppetmodule.h llpuppetmotion.h)
endif (ENABLE_PUPPETRY)

if (LINUX)
  list(APPEND viewer_SOURCE_FILES llappviewerlinux.cpp)
  if (OPENMP)
    set(viewer_LIBRARIES ${GLIB_LIBRARIES} Xinerama omp)
  else (OPENMP)
    set(viewer_LIBRARIES ${GLIB_LIBRARIES} Xinerama)
  endif (OPENMP)
endif (LINUX)

if (WINDOWS)
  list(APPEND viewer_SOURCE_FILES
    llappviewerwindows.cpp
	windows.manifest
  )

  list(APPEND viewer_HEADER_FILES
    llappviewerwindows.h
  )

  # Add resource files to the project. viewerRes.rc is the only buildable file,
  # but the rest are all dependencies of it.
  set(viewer_RESOURCE_FILES
    res/arrow.cur
    res/arrowcop.cur
    res/arrowcopmulti.cur
    res/arrowdrag.cur
    res/circleandline.cur
    res/icon1.ico
    res/llarrow.cur
    res/llarrowdrag.cur
    res/llarrowdragmulti.cur
    res/llarrowlocked.cur
    res/llgrablocked.cur
    res/llno.cur
    res/llnolocked.cur
    res/lltoolcamera.cur
    res/lltoolcreate.cur
    res/lltoolfocus.cur
    res/lltoolgrab.cur
    res/lltoolland.cur
    res/lltoolpan.cur
    res/lltoolpipette.cur
    res/lltoolrotate.cur
    res/lltoolscale.cur
    res/lltooltranslate.cur
    res/lltoolzoomin.cur
    res/lltoolzoomout.cur
    res/ll_icon.BMP
    res/ll_icon.ico
    res/resource.h
    res/toolbuy.cur
    res/toolopen.cur
    res/toolpay.cur
    res/toolpickobject.cur
    res/toolpickobject2.cur
    res/toolpickobject3.cur
    res/toolpipette.cur
    res/toolsit.cur
    res/lltoolpathfinding.cur
    res/lltoolpathfindingpathend.cur
    res/lltoolpathfindingpathendadd.cur
    res/lltoolpathfindingpathstart.cur
    res/lltoolpathfindingpathstartadd.cur
    res/llno.cur
  )

  set_source_files_properties(${viewer_RESOURCE_FILES}
                              PROPERTIES HEADER_FILE_ONLY TRUE)

  set(viewer_RESOURCE_FILES
    res/resource.h
    res/viewerRes.rc
    ${viewer_RESOURCE_FILES}
  )

  source_group("Resource Files" FILES ${viewer_RESOURCE_FILES})

  list(APPEND viewer_SOURCE_FILES ${viewer_RESOURCE_FILES})

  set(viewer_LIBRARIES
    opengl32
    comdlg32
    dxguid
    kernel32
    odbc32
    odbccp32
    ole32
    oleaut32
    Vfw32
    winspool
  )

  find_library(INTEL_MEMOPS_LIBRARY
               NAMES ll_intel_memops
               PATHS
               optimized ${ARCH_PREBUILT_DIRS_RELEASE}
               debug ${ARCH_PREBUILT_DIRS_DEBUG}
               )
  mark_as_advanced(INTEL_MEMOPS_LIBRARY)

  if (INTEL_MEMOPS_LIBRARY)
    list(APPEND viewer_LIBRARIES ${INTEL_MEMOPS_LIBRARY})
  endif (INTEL_MEMOPS_LIBRARY)
endif (WINDOWS)

# Add the xui files. This is handy for searching for xui elements from within
## the IDE.

file(GLOB viewer_XUI_FILE_GLOB_LIST
  ${CMAKE_CURRENT_SOURCE_DIR}/skins/default/xui/en-us/*.xml
)

set(viewer_XUI_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/skins/default/colors_base.xml
)

list(APPEND viewer_XUI_FILES ${viewer_XUI_FILE_GLOB_LIST})
list(SORT viewer_XUI_FILES)

source_group("XUI Files" FILES ${viewer_XUI_FILES})

set_source_files_properties(${viewer_XUI_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND viewer_SOURCE_FILES ${viewer_XUI_FILES})

set(viewer_APPSETTINGS_FILES
  app_settings/anim.ini
  app_settings/cmd_line.xml
  app_settings/grass.xml
  app_settings/grids.xml
  app_settings/gpu_table.txt
  app_settings/keys.ini
  app_settings/keywords.ini
  app_settings/logcontrol.xml
  app_settings/mime_types.xml
  app_settings/message.xml
  app_settings/settings.xml
  app_settings/settings_files.xml
  app_settings/settings_per_account.xml
  app_settings/std_bump.ini
  app_settings/trees.xml
  app_settings/message_template.msg
)

source_group("App Settings" FILES ${viewer_APPSETTINGS_FILES})

set_source_files_properties(${viewer_APPSETTINGS_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND viewer_SOURCE_FILES ${viewer_APPSETTINGS_FILES})

set(viewer_CHARACTER_FILES
  character/attentions.xml
  character/avatar_lad.xml
  character/avatar_skeleton.xml
  character/avatar_constraint.llsd
)

source_group("Character File" FILES ${viewer_CHARACTER_FILES})

set_source_files_properties(${viewer_CHARACTER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND viewer_SOURCE_FILES ${viewer_CHARACTER_FILES})

list(APPEND viewer_SOURCE_FILES ${viewer_HEADER_FILES})

set_source_files_properties(${viewer_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(${VIEWER_BINARY_NAME}
  WIN32
  ${viewer_SOURCE_FILES}
)

# *NOTE: - this list is very sensitive to ordering, test carefully on all
# platforms if you change the releative order of the entries here.
# In particular, cmake 2.6.4 (when buidling with linux/makefile generators)
# appears to sometimes de-duplicate redundantly listed dependencies improperly.
# To work around this, higher level modules should be listed before the modules
# that they depend upon. -brad
#
# *NOTE: On mixing system shared libraries and updated static archives. We use
# a number of libraries that have an existence as system libraries, internal
# use libraries and applications libraries. The most-referenced one of these
# is libz where you can find four or more versions in play at once. On Linux,
# libz can be found at link and run time via a number of paths:
#
#      => -lfreetype
#        => libz.so.1 (on install machine, not build)
#      => -lSDL
#        => libz.so.1 (on install machine, not build)
#      => -lz
#
# We generally want the newest version of the library to provide all symbol
# resolution. To that end, when using static archives, the *_PRELOAD_ARCHIVES
# variables, PNG_PRELOAD_ARCHIVES and ZLIB_PRELOAD_ARCHIVES, get the archives
# dumped into the target binary and runtime lookup will find the most modern
# version.
target_link_libraries(${VIEWER_BINARY_NAME}
  # Make sure MIMALLOC_OBJECT and WINDOWS_LIBRARIES appear first in the list of
  # target link libraries. HB
  ${MIMALLOC_OBJECT}
  ${WINDOWS_LIBRARIES}
  ${PNG_PRELOAD_ARCHIVES}
  ${ZLIB_PRELOAD_ARCHIVES}
  ${LLAPPEARANCE_LIBRARIES}
  ${LLAUDIO_LIBRARIES}
  ${LLCHARACTER_LIBRARIES}
  ${LLIMAGE_LIBRARIES}
  ${LLIMAGEJ2COJ_LIBRARIES}
  ${LLINVENTORY_LIBRARIES}
  ${LLMESSAGE_LIBRARIES}
  ${LLPLUGIN_LIBRARIES}
  ${LLGLTF_LIBRARIES}
  ${LLPRIMITIVE_LIBRARIES}
  ${LLRENDER_LIBRARIES}
  ${FREETYPE2_LIBRARIES}
  ${LLUI_LIBRARIES}
  ${LLFILESYSTEM_LIBRARIES}
  ${LLWINDOW_LIBRARIES}
  ${LLXML_LIBRARIES}
  ${LLMATH_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
  ${NDOF_LIBRARY}
  ${FMODWRAPPER_LIBRARY}
  ${GLOD_LIBRARIES}
  ${HUNSPELL_LIBRARY}
  ${LUA_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${SDL_LIBRARY}
  ${SMARTHEAP_LIBRARY}
  ${UI_LIBRARIES}
  ${ELFIO_LIBRARIES}
  ${viewer_LIBRARIES}
  ${JEMALLOC_LIBRARY}
)

# Add OS-independent dependencies for plugins
add_dependencies(${VIEWER_BINARY_NAME} SLPlugin media_plugin_cef media_plugin_gstreamer)

target_precompile_headers(${VIEWER_BINARY_NAME} PRIVATE llviewerprecompiledheaders.h <llfasttimer.h> <llfloater.h>)

if (ENABLE_PUPPETRY)
  set(CUSTOM_OPTION "${CUSTOM_OPTION} puppetry")
endif (ENABLE_PUPPETRY)
if (OPENMP)
  set(CUSTOM_OPTION "${CUSTOM_OPTION} openmp")
endif (OPENMP)

set(PACKAGE_DEFAULT ON)
set(PACKAGE ${PACKAGE_DEFAULT} CACHE BOOL
    "Add a packaging target that builds an installer package.")

set(MANIFEST_SCRIPT ${CMAKE_SOURCE_DIR}/../scripts/viewer_manifest.py)

if (WINDOWS)
  set_target_properties(${VIEWER_BINARY_NAME}
    PROPERTIES
    LINK_FLAGS "/debug /NODEFAULTLIB:LIBCMT /SUBSYSTEM:WINDOWS"
    LINK_FLAGS_DEBUG "/NODEFAULTLIB:\"LIBCMT;LIBCMTD;MSVCRT\" /INCREMENTAL:NO"
    LINK_FLAGS_RELEASE "/FORCE:MULTIPLE /OPT:REF"
  )

  set(RELEASE_VIEWER_DIR "viewer-win64-release")
  add_custom_command(
    OUTPUT ${CMAKE_CFG_INTDIR}/touched.bat
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS
      ${MANIFEST_SCRIPT}
      --configuration=${CMAKE_CFG_INTDIR}
      --branding_id=${VIEWER_BRANDING_ID}
      --source=${CMAKE_CURRENT_SOURCE_DIR}
      --build=${CMAKE_CURRENT_BINARY_DIR}
      --dest=${CMAKE_SOURCE_DIR}/../${RELEASE_VIEWER_DIR}
      --touch=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/touched.bat
      --custom=${CUSTOM_OPTION}
    DEPENDS ${VIEWER_BINARY_NAME} ${MANIFEST_SCRIPT}
  )

  if (PACKAGE)
    add_custom_target(packaging ALL DEPENDS ${VIEWER_BINARY_NAME} ${CMAKE_CFG_INTDIR}/touched.bat)
  endif (PACKAGE)
endif (WINDOWS)

if (LINUX)
  find_program(EUSTRIP eu-strip)
  if (EUSTRIP)
    add_custom_command(
      OUTPUT ${VIEWER_BINARY_NAME}-stripped
      COMMAND eu-strip
      ARGS --strip-debug --remove-comment -o ${VIEWER_BINARY_NAME}-stripped -f ${VIEWER_BINARY_NAME}.debug ${VIEWER_BINARY_NAME}
      DEPENDS ${VIEWER_BINARY_NAME}
      COMMENT "Creating the stripped binary and the debug symbols files."
    )
  else (EUSTRIP)
    add_custom_command(
      OUTPUT ${VIEWER_BINARY_NAME}-stripped
      COMMAND strip
      ARGS --strip-debug -o ${VIEWER_BINARY_NAME}-stripped ${VIEWER_BINARY_NAME}
      DEPENDS ${VIEWER_BINARY_NAME}
      COMMENT "Creating the stripped binary."
    )
  endif (EUSTRIP)

  set(product ${VIEWER_BRANDING_NAME_CAMELCASE}-${ARCH}-${viewer_VER})

  add_custom_command(
    OUTPUT ${product}.tar.bz2
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS
      ${MANIFEST_SCRIPT}
      --branding_id=${VIEWER_BRANDING_ID}
      --installer_name=${product}
      --source=${CMAKE_CURRENT_SOURCE_DIR}
      --build=${CMAKE_CURRENT_BINARY_DIR}
      --dest=${CMAKE_CURRENT_BINARY_DIR}/packaged
      --touch=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/.${product}.touched
      --buildtype=${CMAKE_BUILD_TYPE}
      --custom=${CUSTOM_OPTION}
    DEPENDS ${VIEWER_BINARY_NAME}-stripped ${MANIFEST_SCRIPT}
    COMMENT "Performing viewer_manifest copy"
  )

  if (PACKAGE)
    add_custom_target(packaging ALL DEPENDS ${product}.tar.bz2)
  endif (PACKAGE)
endif (LINUX)

get_directory_property(ALLDEFINES COMPILE_DEFINITIONS)
message("newview COMPILE_DEFINITIONS = ${ALLDEFINES}")

foreach(d ${ALLDEFINES})
  if (WINDOWS)
    set(defines "${defines} /D${d}")
  else (WINDOWS)
    set(defines "${defines} -D${d}")
  endif (WINDOWS)
endforeach()

string(TOUPPER "${CMAKE_BUILD_TYPE}" build_type)
if ("${build_type}" STREQUAL "DEBUG")
  set(compiler_flags "${CMAKE_CXX_FLAGS_DEBUG}")
elseif ("${build_type}" STREQUAL "RELWITHDEBINFO")
  set(compiler_flags "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
else ()
  set(compiler_flags "${CMAKE_CXX_FLAGS_RELEASE}")
endif ()
if (USELTO AND NOT "${CMAKE_CXX_COMPILE_OPTIONS_IPO}" STREQUAL "")
  foreach(f ${CMAKE_CXX_COMPILE_OPTIONS_IPO})
    set(compiler_flags "${compiler_flags} ${f}")
  endforeach()
endif ()
set(compiler_flags "${compiler_flags} ${CMAKE_CXX_FLAGS}${defines}")

message(STATUS "Compile flags for target ${CMAKE_BUILD_TYPE} = ${compiler_flags}")
if (SELECTED_LINKER)
	message(STATUS "Using ld.${SELECTED_LINKER} as the linker.")
endif (SELECTED_LINKER)

# This is used in llfloaterabout.cpp to include the compile flags in the
# Version info tab
add_definitions(-DLL_COMPILE_FLAGS="\"${compiler_flags}\"")

# Include this here and not at the top of the cmake file, so that we do not
# "pollute" the compiler flags with GLM-specific defines.
include(GLM)

# When using new libcurl, ngttp2 is also in use, so signal that as well for use
# in llfloaterabout.cpp. Also enable the workaround for the HTTP pipelining bug
# in new libcurl when in use...
if (USE_NEW_LIBCURL)
  add_definitions(-DLL_NGHTTP2=1)
  add_definitions(-DLL_CURL_BUG=1)
endif (USE_NEW_LIBCURL)
