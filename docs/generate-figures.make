# Makefile rules to rasterise figures from source formats (e.g. SVG)

# FIXME - is this already defined?
FIGURES_DIR = C/figures

$(FIGURES_DIR)/%.png: $(FIGURES_DIR)/src/%.svg
	inkscape -z -e $@ $<
	grep -qx -- $@ .gitignore || echo $@ >> .gitignore

SVG_FILES = $(patsubst %.svg,$(FIGURES_DIR)/src/%.svg,$(DOC_FIGURES_SRC))
PNG_FILES = $(patsubst %.svg,$(FIGURES_DIR)/%.png,$(DOC_FIGURES_SRC))

figures-png: $(PNG_FILES)
