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

$(eval $(call gb_Package_add_files,extras_fonts,$(LIBO_SHARE_FOLDER)/fonts/truetype,\
	SourceHanSans-Regular.ttc \
	SourceHanSans-Bold.ttc \
	SourceHanSerif-Regular.ttc \
	SourceHanSerif-Bold.ttc \
	TW-Kai-98_1.ttf \
	TW-Kai-Ext-B-98_1.ttf \
	TW-Sung-98_1.ttf \
	TW-Sung-Ext-B-98_1.ttf \
))

# vim: set noet sw=4 ts=4:
