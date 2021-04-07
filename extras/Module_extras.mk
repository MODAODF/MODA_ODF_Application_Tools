# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Module_Module,extras))

$(eval $(call gb_Module_add_targets,extras,\
	CustomTarget_autocorr \
	CustomTarget_autotextuser \
	CustomTarget_glade \
	CustomTarget_gallsystem \
	CustomTarget_tplofficorr \
	CustomTarget_tploffimisc \
	CustomTarget_tplpersonal \
	CustomTarget_tplpresnt \
	CustomTarget_tpl_styles \
	CustomTarget_tpldraw \
	Package_autocorr \
	Package_autotextuser \
	Package_cfgsrvnolang \
	Package_cfgusr \
	Package_database \
	Package_databasebiblio \
	Package_fonts \
	Package_resource_fonts \
	Package_gallbullets \
	Package_gallmytheme \
	Package_gallroot \
	Package_gallsystem \
	Package_gallsystemstr \
	Package_gallcountry \
	Package_gallodf_icons \
	Package_gallforbidden \
	Package_gallhospital \
	Package_gallfileformat \
	Package_glade \
	Package_labels \
	$(if $(filter WNT,$(OS)),Package_newfiles) \
	Package_palettes \
	Package_tplofficorr \
	Package_tploffimisc \
	Package_tplpersonal \
	Package_tplpresnt \
	Package_tpl_styles \
	Package_tpldraw \
	Package_tplwizagenda \
	Package_tplwizbitmap \
	Package_tplwizdesktop \
	Package_tplwizfax \
	Package_tplwizletter \
	Package_tplwizreport \
	Package_tplwizstyles \
	Package_wordbook \
))

$(eval $(call gb_Module_add_l10n_targets,extras,\
	CustomTarget_autotextshare \
	AllLangPackage_autotextshare \
))

ifneq ($(WITH_GALLERY_BUILD),)
$(eval $(call gb_Module_add_targets,extras,\
	Gallery_backgrounds \
	Gallery_sound \
	Gallery_country \
	Gallery_forbidden \
	Gallery_odf_icons \
	Gallery_hospital \
	Gallery_fileformat \
))
endif

$(eval $(call gb_Module_add_targets,extras,\
	Personas \
))

$(eval $(call gb_Module_add_targets,extras,\
	CustomTarget_opensymbol \
))

ifneq ($(OS),WNT)
$(eval $(call gb_Module_add_targets,extras,\
	CustomTarget_sourcehansans_fonts_ttc \
))
else
$(eval $(call gb_Module_add_targets,extras,\
	CustomTarget_sourcehansans_fonts \
))
endif

$(eval $(call gb_Module_add_targets,extras,\
	CustomTarget_cns11643_fonts \
))

$(eval $(call gb_Module_add_targets,extras,\
	CustomTarget_wingdings2_fonts \
))

# vim: set noet sw=4 ts=4:
