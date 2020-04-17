INSTALL_TOP  = /usr/local
INSTALL_BIN  = $(INSTALL_TOP)/bin
INSTALL_INC  = $(INSTALL_TOP)/include/flamingo

INSTALL = install -p
INSTALL_EXEC = $(INSTALL) -m 0755
INSTALL_DATA = $(INSTALL) -m 0644

BUILDDIR := build
MKDIR = mkdir -p
RM = rm -rf

TO_BIN = flamingo
TO_INC = base.fl

default: release
release:
	@$(MAKE) -f scripts/build.mk NAME=flamingo MODE=release SRCDIR=src/c
	@cp build/flamingo flamingo
debug:
	@$(MAKE) -f scripts/build.mk NAME=flamingo MODE=debug SRCDIR=src/c
install:
	$(MKDIR) $(INSTALL_BIN) $(INSTALL_INC)
	$(INSTALL_EXEC) $(TO_BIN) $(INSTALL_BIN)
	cd src/fl && $(INSTALL_DATA) $(TO_INC) $(INSTALL_INC)
uninstall:
	cd $(INSTALL_BIN) && $(RM) $(TO_BIN)
	cd src/fl && cd $(INSTALL_INC) && $(RM) $(TO_INC)
clean:
	$(RM) $(BUILDDIR)
	$(RM) flamingo

.PHONY: default release debug install uninstall clean
