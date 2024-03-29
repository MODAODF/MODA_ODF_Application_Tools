# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExternalProject_ExternalProject,libwebp))

$(eval $(call gb_ExternalProject_register_targets,libwebp,\
	build \
))

ifeq ($(COM),MSC)
$(eval $(call gb_ExternalProject_use_nmake,libwebp,build))

$(call gb_ExternalProject_get_state_target,libwebp,build):
	$(call gb_Trace_StartRange,libwebp,EXTERNAL)
	$(call gb_ExternalProject_run,build,\
		nmake -nologo -f Makefile.vc CFG=release-static RTLIBCFG=static OBJDIR=output \
	)
	$(call gb_Trace_EndRange,libwebp,EXTERNAL)
else
$(eval $(call gb_ExternalProject_use_autoconf,libwebp,build))

$(call gb_ExternalProject_get_state_target,libwebp,build) :
	$(call gb_Trace_StartRange,libwebp,EXTERNAL)
	$(call gb_ExternalProject_run,build,\
		export PKG_CONFIG="" \
		&& MAKE=$(MAKE) $(gb_RUN_CONFIGURE) ./configure \
			--enable-static \
			--with-pic \
			--disable-shared \
			--disable-gl \
			--disable-sdl \
			--disable-png \
			--disable-jpeg \
			--disable-tiff \
			--disable-gif \
			--disable-wic \
			$(if $(verbose),--disable-silent-rules,--enable-silent-rules) \
			CXXFLAGS="$(gb_CXXFLAGS) $(if $(ENABLE_OPTIMIZED),$(gb_COMPILEROPTFLAGS),$(gb_COMPILERNOOPTFLAGS))" \
			CPPFLAGS="$(CPPFLAGS) $(BOOST_CPPFLAGS)" \
			$(if $(CROSS_COMPILING),--build=$(BUILD_PLATFORM) --host=$(HOST_PLATFORM)) \
		&& $(MAKE) \
	)
	$(call gb_Trace_EndRange,libwebp,EXTERNAL)
endif

# vim: set noet sw=4 ts=4:
