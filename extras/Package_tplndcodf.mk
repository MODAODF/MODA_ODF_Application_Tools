# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Package_Package,extras_tplndcodf,$(SRCDIR)/extras/source/templates/NDCODFTemplate))

$(eval $(call gb_Package_add_files,extras_tplndcodf,$(LIBO_SHARE_FOLDER)/template/common/NDCODFTemplate,\
	swriter.ott \
	LibreOfficeDefault.ott \
  Graphic_Sample_Template.otp \
  apachecon_eu_2012_1.otp \
  chalkboard.otp \
  exceltemplatespro.com_33.otp \
  fedora-textures-blue.otp \
  fedora-textures-grey-blue.otp \
  mazu.otp \
  orangine-stylish.otp \
  oschloe_en_004.otp \
  oschloe_en_006.otp \
))

# vim: set noet sw=4 ts=4:
