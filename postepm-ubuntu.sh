#!/bin/bash
# postepm for ubuntu
# used for insert 'Section:' to every .deb file
# run at top of source directory
DPKG=`which dpkg`
PKG=(OxOffice_R5_Linux_x86_deb OxOffice_R5_Linux_x86_deb_langpack_zh-TW)
PRJ=(LibreOffice LibreOffice_languagepack)

for idx in "${!PRJ[@]}" ; do
    prj="${PRJ[$idx]}" ;
    pkg="${PKG[$idx]}" ;
    pkgdir="workdir/installation/$prj/deb/install/$pkg/DEBS" ;
    for debfile in $pkgdir/*.deb ; do
        debdir="${debfile}_dir" ;
        debinfodir="${debdir}/DEBIAN" ;
        mkdir -p $debinfodir ;
        basename $debfile ;
        # 1) unpack *.deb and it's info dir: /DEBIAN
        $DPKG -x $debfile $debdir ;
        $DPKG -e $debfile $debinfodir ;
        echo "Section: OxOffice" >> $debinfodir/control ;
        # 2) re-pack to origin *.deb
        $DPKG -b $debdir $debfile ;
        rm $debdir -rf ;
    done ;
    # 3) re-tar to origin.
    pushd "workdir/installation/$prj/deb/install" ;
    pkgdowndir="${pkg}_download" ;
    tar zcvf "${pkgdowndir}/${pkg}.tar.gz" $pkg
    popd
done
