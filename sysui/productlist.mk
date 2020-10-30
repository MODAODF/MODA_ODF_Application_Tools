# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

PRODUCTLIST := libreoffice oxoffice ndcodfsys libreofficedev
PKGVERSION := $(USERDIRPRODUCTVERSION).$(OXO_VERSION_MINOR)
PKGVERSIONSHORT := $(USERDIRPRODUCTVERSION)
PRODUCTNAME.libreoffice := LibreOffice
PRODUCTNAME.oxoffice := OxOffice
PRODUCTNAME.ndcodfsys := NDCODFSYS
PRODUCTNAME.libreofficedev := LibreOfficeDev
UNIXFILENAME.libreoffice := libreoffice$(PKGVERSIONSHORT)
UNIXFILENAME.oxoffice := oxoffice
UNIXFILENAME.ndcodfsys := ndcodfsys
UNIXFILENAME.libreofficedev := libreofficedev$(PKGVERSIONSHORT)

# vim: set noet sw=4 ts=4:
