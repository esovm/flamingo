BUILDDIR := build
RM = -rm -rf

default: release
release:
	@$(MAKE) -f scripts/build.mk NAME=flamingo MODE=release SRCDIR=src
	@cp build/flamingo flamingo
debug:
	@$(MAKE) -f scripts/build.mk NAME=flamingo MODE=debug SRCDIR=src
clean:
	$(RM) $(BUILDDIR)
	$(RM) flamingo

.PHONY: default release debug clean
