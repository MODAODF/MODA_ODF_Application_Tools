# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Package_Package,extras_gallODF,$(SRCDIR)/extras/source/gallery/ODF))

$(eval $(call gb_Package_add_files,extras_gallODF,$(LIBO_SHARE_FOLDER)/gallery/ODF,\
	LibreOffice_6.1_Calc_Icon.svg \
	LibreOffice_6.1_Draw_Icon.svg \
	LibreOffice_6.1_Impress_Icon.svg \
	LibreOffice_6.1_Writer_Icon.svg \
	libreoffice.svg \
	odf-file-format-symbol-svgrepo-com.svg \
	odf.svg \
))


# vim: set noet sw=4 ts=4:
