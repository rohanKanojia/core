# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,sw_mailmerge))

$(eval $(call gb_CppunitTest_add_exception_objects,sw_mailmerge, \
    sw/qa/extras/mailmerge/mailmerge \
))

$(eval $(call gb_CppunitTest_use_libraries,sw_mailmerge, \
    comphelper \
    cppu \
    cppuhelper \
    sal \
    sfx \
    svl \
    sw \
    test \
    tl \
    unotest \
    utl \
))

$(eval $(call gb_CppunitTest_use_externals,sw_mailmerge, \
    boost_headers \
    libxml2 \
))

$(eval $(call gb_CppunitTest_use_api,sw_mailmerge,\
	udkapi \
	offapi \
	oovbaapi \
))

$(eval $(call gb_CppunitTest_use_components,sw_mailmerge, \
    basic/util/sb \
    comphelper/util/comphelp \
    configmgr/source/configmgr \
    connectivity/source/cpool/dbpool2 \
    connectivity/source/drivers/calc/calc \
    connectivity/source/drivers/writer/writer \
    connectivity/source/manager/sdbc2 \
    dbaccess/source/filter/xml/dbaxml \
    dbaccess/util/dba \
    embeddedobj/util/embobj \
    filter/source/config/cache/filterconfig1 \
    filter/source/odfflatxml/odfflatxml \
    filter/source/storagefilterdetect/storagefd \
    filter/source/xmlfilteradaptor/xmlfa \
    filter/source/xmlfilterdetect/xmlfd \
    forms/util/frm \
    framework/util/fwk \
    i18npool/util/i18npool \
    lingucomponent/source/languageguessing/guesslang \
    linguistic/source/lng \
    oox/util/oox \
    package/source/xstor/xstor \
    package/util/package2 \
    sax/source/expatwrap/expwrap \
    sc/util/sc \
    sc/util/scfilt \
    sfx2/util/sfx \
    sot/util/sot \
    svl/source/fsstor/fsstorage \
    svl/util/svl \
    svtools/util/svt \
    sw/util/sw \
    sw/util/swd \
    toolkit/util/tk \
    ucb/source/core/ucb1 \
    ucb/source/ucp/file/ucpfile1 \
    ucb/source/ucp/tdoc/ucptdoc1 \
    unotools/util/utl \
    unoxml/source/rdf/unordf \
    unoxml/source/service/unoxml \
    uui/util/uui \
    vcl/vcl.common \
    $(if $(filter-out MACOSX WNT,$(OS)), \
        $(if $(DISABLE_GUI),, \
            vcl/vcl.unx \
        ) \
    ) \
    xmloff/util/xo \
    xmlscript/util/xmlscript \
))

$(eval $(call gb_CppunitTest_use_configuration,sw_mailmerge))
$(eval $(call gb_CppunitTest_use_ure,sw_mailmerge))
$(eval $(call gb_CppunitTest_use_vcl,sw_mailmerge))

$(eval $(call gb_CppunitTest_set_include,sw_mailmerge,\
    -I$(SRCDIR)/sw/inc \
    -I$(SRCDIR)/sw/source/core/inc \
    -I$(SRCDIR)/sw/qa/extras/inc \
    -I$(SRCDIR)/sw/source/uibase/inc \
    $$(INCLUDE) \
))

$(eval $(call gb_CppunitTest_use_uiconfigs,sw_mailmerge,\
	modules/swriter \
))

# vim: set noet sw=4 ts=4:
