# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExtensionPackageSet_ExtensionPackageSet,misc_extensions))

ifneq ($(NUMBERTEXT_EXTENSION_PACK),)
$(eval $(call gb_ExtensionPackageSet_add_extension,misc_extensions,numbertext,$(NUMBERTEXT_EXTENSION_PACK)))
endif

ifeq ($(CPMLIBRE_EXTENSION_PACK),yes)
$(eval $(call gb_ExtensionPackageSet_add_extension,misc_extensions,cpmlibre,cpmlibre.oxt))
endif

ifeq ($(HYPERLINK_EXTENSION_PACK),yes)
$(eval $(call gb_ExtensionPackageSet_add_extension,misc_extensions,hyperlink,hyperlink.oxt))
endif

ifeq ($(ONEKEY2ODF_EXTENSION_PACK),yes)
$(eval $(call gb_ExtensionPackageSet_add_extension,misc_extensions,onekey2odf,onekey2odf.oxt))
endif

ifeq ($(NDCHELP_EXTENSION_PACK),yes)
$(eval $(call gb_ExtensionPackageSet_add_extension,misc_extensions,NDCHelp,NDCHelp.oxt))
endif

ifeq ($(VRTNETWORKEQUIPMENT_EXTENSION_PACK),yes)
$(eval $(call gb_ExtensionPackageSet_add_extension,misc_extensions,VrtNetworkEquipment,vrtnetworkequipment.oxt))
endif

ifeq ($(FORMATCHECK_EXTENSION_PACK),yes)
$(eval $(call gb_ExtensionPackageSet_add_extension,misc_extensions,FormatCheck,FormatCheck.oxt))
endif

# vim: set noet sw=4 ts=4:
