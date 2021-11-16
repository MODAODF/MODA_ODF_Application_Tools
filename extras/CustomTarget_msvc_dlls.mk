# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CustomTarget_CustomTarget,extras/source/msvc_dlls))


$(eval $(call gb_CustomTarget_register_targets,extras/source/msvc_dlls,vcruntime140_1.dll))

ifneq (,$(BUILD_X64))
$(eval $(call gb_CustomTarget_register_targets,extras/source/msvc_dlls,msvcp140_32.dll))
$(call gb_CustomTarget_get_workdir,extras/source/msvc_dlls)/msvcp140.dll : \
		$(SRCDIR)/extras/source/msvc_dlls/msvcp140_32.dll
	cp $< $@
else
$(eval $(call gb_CustomTarget_register_targets,extras/source/msvc_dlls,msvcp140_64.dll))
$(call gb_CustomTarget_get_workdir,extras/source/msvc_dlls)/msvcp140.dll : \
		$(SRCDIR)/extras/source/msvc_dlls/msvcp140_64.dll
	cp $< $@
endif


$(call gb_CustomTarget_get_workdir,extras/source/msvc_dlls)/vcruntime140_1.dll : \
		$(SRCDIR)/extras/source/msvc_dlls/vcruntime140_1.dll
	cp $< $@


# vim: set noet sw=4 ts=4:
