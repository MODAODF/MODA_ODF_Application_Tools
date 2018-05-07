# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExternalPackage_ExternalPackage,onekey2odf,Onekey2ODF))

$(eval $(call gb_ExternalPackage_add_unpacked_files_with_dir,onekey2odf,share/extensions/Onekey2ODF,\
    Addons.xcu \
	description.xml \
	META-INF/manifest.xml \
	Onekey2ODF/dialog.xlb \
	Onekey2ODF/onekey2ODF.xba \
	Onekey2ODF/script.xlb \
	pkg-desc/description_en-US.txt \
	pkg-desc/description_zh-TW.txt \
))

# vim: set noet sw=4 ts=4:
