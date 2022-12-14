# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Package_Package,extras_gallsystem,$(SRCDIR)/extras/source/gallery/gallery_system))

$(eval $(call gb_Package_add_files,extras_gallsystem,$(LIBO_SHARE_FOLDER)/gallery,\
	sg1.sdg \
	sg1.sdv \
	sg1.thm \
	sg4.sdg \
	sg4.sdv \
	sg4.thm \
	sg24.sdg \
	sg24.sdv \
	sg24.thm \
	sg36.sdg \
	sg36.sdv \
	sg36.thm \
	country.sdg \
	country.sdv \
	country.thm \
	country.str \
	ODF.sdg \
	ODF.sdv \
	ODF.thm \
	ODF.str \
	hospital.sdg \
	hospital.sdv \
	hospital.thm \
	hospital.str \
	forbidden.sdg \
	forbidden.sdv \
	forbidden.thm \
	forbidden.str \
	fileformat.sdg \
	fileformat.sdv \
	fileformat.thm \
	fileformat.str \
))

# vim: set noet sw=4 ts=4:
