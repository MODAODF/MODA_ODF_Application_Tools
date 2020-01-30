# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Module_Module,libnumbertext))

$(eval $(call gb_Module_add_targets,libnumbertext,\
	ExternalPackage_numbertext \
	UnpackedTarball_libnumbertext \
))
ifeq ($(COM),MSC)
$(eval $(call gb_Module_add_targets,libnumbertext,\
	StaticLibrary_libnumbertext \
))
else

ifeq ($(ENABLE_LIBNUMBERTEXT),TRUE)

$(eval $(call gb_Module_add_targets,libnumbertext,\
	ExternalProject_libnumbertext \
))

endif

endif

# vim: set noet sw=4 ts=4: