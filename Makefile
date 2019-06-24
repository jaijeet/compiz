INSTALLDIR=/home/jr/local/pkgs/compiz-0.9-github-jaijeet--jr-master--arch-x86_64
MAKEOPTS="-j4"
BUILDDIR=build

build:
	- mkdir $(BUILDDIR)
	(cd $(BUILDDIR) && cmake .. -DCMAKE_BUILD_TYPE="Release" \
       -DCMAKE_INSTALL_PREFIX="$(INSTALLDIR)"\
       -DCMAKE_INSTALL_LIBDIR="$(INSTALLDIR)/lib" \
       -DCOMPIZ_DISABLE_SCHEMAS_INSTALL=On \
    -DCOMPIZ_BUILD_WITH_RPATH=Off \
    -DCOMPIZ_PACKAGING_ENABLED=On \
    -DBUILD_GTK=On \
    -DBUILD_METACITY=On \
    -DBUILD_KDE4=Off \
    -DCOMPIZ_BUILD_TESTING=Off \
    -DCOMPIZ_WERROR=Off \
    -DCOMPIZ_DEFAULT_PLUGINS="composite,opengl,decor,resize,place,move,compiztoolbox,staticswitcher,regex,animation,wall,ccp" && make $(MAKEOPTS))
.PHONY: build

install: build installCompiz installDocs installGsettingSchemas installLicenses

installCompiz:
	(cd $(BUILDDIR) && make install)
.PHONY: installCompiz

installDocs:
	- install -dm755 $(INSTALLDIR)/share/doc/compiz/
	install {AUTHORS,NEWS,README} $(INSTALLDIR)/share/doc/compiz/
.PHONY: installDocs

installGsettingSchemas:
	#ifeq(1, $(shell ls $(BUILDDIR)/generated/glib-2.0/schemas/ | grep -m1 .gschema.xml | wc -l))
	- install -dm755 "$(INSTALLDIR)/share/glib-2.0/schemas/"
	- install -m644 $(BUILDDIR)/generated/glib-2.0/schemas/*.gschema.xml $(INSTALLDIR)/share/glib-2.0/schemas/
	#endif
.PHONY: installGsettingSchemas

installLicenses:
	- install -dm755 "$(INSTALLDIR)/share/licenses/compiz/"
	- install -m644 COPYING* $(INSTALLDIR)/share/licenses/compiz/
.PHONY: installLicenses

clean:
	@ (cd build && make clean)
.PHONY: clean

cleanBuild:
	- rm -fr compizconfig/ccsm/build/
	- rm -fr compizconfig/ccsm/ccsm.desktop
	- rm -fr compizconfig/ccsm/installed_files
	- rm -fr build
.PHONY: cleanBuild

cleanInstallation:
	- rm -fr $(INSTALLDIR)
.PHONY: cleanInstallation
