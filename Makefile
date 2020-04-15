BUILDDIR := build
RM = -rm -rf

default: debug
release:
	@$(MAKE) -f scripts/build.mk NAME=flamingo MODE=release SRCDIR=src/c
	@cp build/flamingo flamingo
debug:
	@$(MAKE) -f scripts/build.mk NAME=flamingo MODE=debug SRCDIR=src/c
run:
	@$(MAKE) clean
	@$(MAKE) release
	@./flamingo
clean:
	$(RM) $(BUILDDIR)
	$(RM) flamingo

.PHONY: clean release default run
