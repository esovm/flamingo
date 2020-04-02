BUILDDIR := build
RM = -rm -rf

default: gg
gg:
	$(MAKE) -f src/build.mk NAME=gg MODE=release SRCDIR=src
	cp build/gg gg
debug:
	$(MAKE) -f src/build.mk NAME=gg MODE=debug SRCDIR=src
clean:
	$(RM) $(BUILDDIR)
	$(RM) gg

.PHONY: clean gg default
