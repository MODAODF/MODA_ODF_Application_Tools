#!/bin/bash
VENDOR=( ossii Coretronic Bingotimes Youstar ytes Etron LOTES TCES ossii_trial)
VENDOR_R=( "" Coretronic Bingotimes Youstar YTES Etron LOTES TCES "" )
LANGS="en-US zh-TW"
OOOVENDOR="OSS Integral Institute Co., Ltd."
MAJOR_VER=8  # R8X3
MINOR_VER=3
PATCH_VER=1
MILESTONE_SIMPLE="R$MAJOR_VER"
MILESTONE="R${MAJOR_VER}W${MINOR_VER}"
MILESTONE_LINUX="R${MAJOR_VER}L${MINOR_VER}"
BASE_PATH=$PWD
PATCH_PATH=$BASE_PATH/

if test $# -lt 1 ; then
    echo $0 "[branch [-fedora|-ubuntu|bananapi]]"
    echo The branch is one integer number of:
    for i in $(seq 0 $((${#VENDOR[@]} - 1)))
    do
        echo $i - ${VENDOR[$i]}
    done
    exit
fi
if test $# -gt 2 ; then
    echo Wrong argument
    exit
fi
if test $# -eq 2 && (test $2 = "-fedora" || test $2 = "-ubuntu" || test $2 = "-bananapi") ; then
   MILESTONE=$MILESTONE_LINUX
fi

typeset -i branch_id=$1
if test $branch_id -ge ${#VENDOR[@]} || test $branch_id -lt 0; then
    echo -ge or "<0"
    exit
fi

PIC_PATH=$BASE_PATH/icon-themes/ossii/${VENDOR[$branch_id]}

# update logos
echo Update logos ...

cp $PIC_PATH/startcenter-logo.png $BASE_PATH/icon-themes/colibre/sfx2/res
cp $PIC_PATH/logo.png $BASE_PATH/icon-themes/colibre/sfx2/res
cp $PIC_PATH/[BI]*.bmp $BASE_PATH/instsetoo_native/inc_common/windows/msi_templates/Binary
cp $PIC_PATH/brand/intro.png $BASE_PATH/icon-themes/colibre/brand
cp $PIC_PATH/brand/intro-highres.png $BASE_PATH/icon-themes/colibre/brand
cp $PIC_PATH/brand/intro.png $BASE_PATH/icon-themes/colibre/brand_dev
cp $PIC_PATH/brand/intro-highres.png $BASE_PATH/icon-themes/colibre/brand_dev
cp $PIC_PATH/brand/flat_logo.svg $BASE_PATH/icon-themes/colibre/brand
cp $PIC_PATH/brand/about.svg $BASE_PATH/icon-themes/colibre/brand/shell

# patches
echo Local patches ...

# git am $PIC_PATH/../0070-sfx2-new-menu-item-for.patch || git am --abort

cd translations
if test -e $PIC_PATH/../0047-cui-aboutbox-update-intro-message-for-OSSII.patch.po ; then
    patch -p1 -sl -F2 < $PIC_PATH/../0047-cui-aboutbox-update-intro-message-for-OSSII.patch.po
fi
if test -e $PIC_PATH/../0071-officecfg_registry_data_org_open_office_Office_UI.patch.po ; then
    patch -p1 -sl -F2 < $PIC_PATH/../0071-officecfg_registry_data_org_open_office_Office_UI.patch.po
fi
if test -e $PIC_PATH/../0072-sfx2_messages.patch.po ; then
    patch -p1 -sl -F2 < $PIC_PATH/../0072-sfx2_messages.patch.po
fi
cd -

#if test -e $PIC_PATH/../0070-sfx2-new-menu-item-for.patch__po ; then
#    patch -p1 -sl -F2 < $PIC_PATH/../0070-sfx2-new-menu-item-for.patch
#fi

if test -e $PIC_PATH/diff.patch ; then
    patch -p0 -sl -F2 < $PIC_PATH/diff.patch
fi

# start building ...

echo Next edit autoget.input and run ./autogen.sh then make
#./postepm-ubuntu.sh

