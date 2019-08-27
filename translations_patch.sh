#!/bin/bash
# patch translations
echo patch translations....
cd translations
# cp -a ../icon-themes/ossii/zh-TW_cui_uiconfig_ui_common.po ./source/zh-TW/cui/uiconfig/ui.po
git am ../icon-themes/ossii/*.patch > /dev/null 2>&1|| git am --abort > /dev/null 2>&1

# for NDC
if test $1 = "9" ; then
    cp -a ../icon-themes/ossii/NDC/zh-TW_cui_uiconfig_ui.po ./source/zh-TW/cui/uiconfig/ui.po
    doc=`awk '/NDCGOVODFTool/{print}' ./source/zh-TW/sfx2/source/doc.po`
    if test ${#doc} = 0 ; then
        patch -p1 -f < ../icon-themes/ossii/NDC/NDCGOVODFTool.patch.po > /dev/null 2>&1
    fi
    patch -p1 -f < ../icon-themes/ossii/NDC/tooltip.patch.po > /dev/null 2>&1
fi

cd -
