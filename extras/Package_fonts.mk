# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Package_Package,extras_fonts,$(call gb_CustomTarget_get_workdir,extras/fonts)))

$(eval $(call gb_Package_use_customtarget,extras_fonts,extras/fonts))

$(eval $(call gb_Package_add_file,extras_fonts,$(LIBO_SHARE_FOLDER)/fonts/truetype/opens___.ttf,opens___.ttf))

$(eval $(call gb_Package_add_file,extras_fonts,$(LIBO_SHARE_FOLDER)/fonts/truetype/SourceHanSansTC-Regular.otf.otf,source/truetype/symbol/SourceHanSansTC-Regular.otf.otf)) \
$(eval $(call gb_Package_add_file,extras_fonts,$(LIBO_SHARE_FOLDER)/fonts/truetype/SourceHanSansTC-Bold.otf,source/truetype/symbol/SourceHanSansTC-Bold.otf)) \

$(eval $(call gb_Package_add_file,extras_fonts,$(LIBO_SHARE_FOLDER)/fonts/truetype/TW-Kai-98_1.ttf,source/truetype/symbol/TW-Kai-98_1.ttf)) \
$(eval $(call gb_Package_add_file,extras_fonts,$(LIBO_SHARE_FOLDER)/fonts/truetype/TW-Kai-Ext-B-98_1.ttf,source/truetype/symbol/TW-Kai-Ext-B-98_1.ttf)) \
$(eval $(call gb_Package_add_file,extras_fonts,$(LIBO_SHARE_FOLDER)/fonts/truetype/TW-Sung-98_1.ttf,source/truetype/symbol/TW-Sung-98_1.ttf)) \
$(eval $(call gb_Package_add_file,extras_fonts,$(LIBO_SHARE_FOLDER)/fonts/truetype/TW-Sung-Ext-B-98_1.ttf,source/truetype/symbol/TW-Sung-Ext-B-98_1.ttf)) \

# vim: set noet sw=4 ts=4:
