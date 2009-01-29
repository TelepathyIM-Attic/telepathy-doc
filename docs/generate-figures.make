# Makefile rules to rasterise figures from source formats (e.g. SVG)

# FIXME - is this already defined?
FIGURES_DIR = C/figures

%.svg.png:
	inkscape -z -e \
		$(FIGURES_DIR)/$(patsubst %.svg.png,%.png,$@) \
		$(FIGURES_DIR)/src/$(patsubst %.svg.png,%.svg,$@)

PNG_FILES = $(addsuffix .png,$(DOC_FIGURES_SRC))
figures-png: $(PNG_FILES)
