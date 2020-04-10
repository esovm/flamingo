BUILDDIR := build
RM = -rm -rf

default: debug
release:
	$(MAKE) -f src/build.mk NAME=crane MODE=release SRCDIR=src
	cp build/crane crane
debug:
	$(MAKE) -f src/build.mk NAME=crane MODE=debug SRCDIR=src
run:
	@$(MAKE) release
	@./crane
clean:
	$(RM) $(BUILDDIR)
	$(RM) crane

.PHONY: clean release default run
