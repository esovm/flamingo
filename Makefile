BUILDDIR := build
RM = -rm -rf

default: debug
release:
	$(MAKE) -f src/build.mk NAME=gg MODE=release SRCDIR=src
	cp build/gg gg
debug:
	$(MAKE) -f src/build.mk NAME=gg MODE=debug SRCDIR=src
run:
	@$(MAKE) release
	@./gg
clean:
	$(RM) $(BUILDDIR)
	$(RM) gg

.PHONY: clean release default run
