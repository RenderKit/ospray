SHELL := /bin/bash

webpages := $(addprefix www/, $(addsuffix .html, index tutorials documentation gallery downloads related_projects displaywall ospray2_porting_guide legal))
process_version := $(addprefix tmp/, $(addsuffix .md, tutorials getting_ospray readme_head))
tmptexfiles := $(addprefix tmp/, $(addsuffix .tex, overview changelog compilation api tutorials))
images_jpg := $(addprefix images/, $(addsuffix .jpg, exampleViewer $(addprefix camera_, perspective architectural stereo orthographic panoramic) $(addprefix material_, OBJ Principled CarPaint Metal Alloy Glass ThinGlass MetallicPaint Luminous) ColoredWindow $(addprefix ospMPIDistribTutorial, Volume Spheres _firstFrame _accumulatedFrame)))
images_png := $(addprefix images/, $(addsuffix .png, diffuse_rooms normalmap_frustum tutorial_accumulatedframe tutorial_firstframe ospExamples renderSunSky))
images_fig := spot_light c-gamma_coords quad_light hdri_light 
images_svg := gitter_badge structured_spherical_coords vdb_structure

images_fig2pdf := $(addprefix tmp/, $(addsuffix .pdf, $(images_fig)))
images_svg2pdf := $(addprefix images/, $(addsuffix .pdf, $(images_svg)))
images_fig2png := $(addprefix images/, $(addsuffix .png, $(images_fig)))
webimages := $(addprefix www/, $(images_jpg) $(images_png) $(images_fig2png) $(addprefix images/, vdb_structure.png $(addsuffix .svg, $(images_svg))))
pdfimages := $(images_jpg) $(images_png) $(images_fig2pdf) $(images_svg2pdf)

PANDOC := pandoc
PDMINVERSION := 2000000
PDVERSION := $(shell $(PANDOC) --version|head -n 1)
$(info Using $(PDVERSION))
PDOK := $(shell expr `$(PANDOC) --version|head -n 1|cut -d' ' -f 2| sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$$/&00/' -e 's/^[0-9]\{5,6\}$$/&00/'` \>= $(PDMINVERSION))
ifneq "$(PDOK)" "1"
  $(error You need at least pandoc v2.0)
endif

all: www doc pdf
www: $(webpages) www/stylesheet.css $(webimages) | wwwscripts
doc: ../README.md
pdf: ../readme.pdf
spec: ospray-spec.html ospray-spec.rst $(webimages)
checklinks: tmp/brokenlinks.txt
	@if [[ -s $^ ]] ; then \
	  echo "Found broken link:" ;\
	  cat $^ ;\
	  exit 1 ;\
	else \
	  echo "All links good." ;\
	fi


.PHONY: all www doc pdf clean realclean submodule checklinks spec

submodule:
	git submodule update --init --remote

if_is_devel_then = if [[ "${CI_BUILD_REF_NAME}" == *"devel" || (-d ../.git && "`git rev-parse --abbrev-ref HEAD`" == *"devel") ]] ; then

tmp/version: ../cmake/ospray_version.cmake $(wildcard ../.git/HEAD)
	sed -n $< -e "s/^set(OSPRAY_VERSION_MAJOR \([0-9]\+\))/\1./ip" | tr -d '\n' > $@
	sed -n $< -e "s/^set(OSPRAY_VERSION_MINOR \([0-9]\+\))/\1./ip" | tr -d '\n' >> $@
	sed -n $< -e "s/^set(OSPRAY_VERSION_PATCH \([0-9]\+\))/\1/ip" | tr -d '\n' >> $@
	sed -n $< -e 's/^set(OSPRAY_VERSION_NOTE "")//;t;s/^set(OSPRAY_VERSION_NOTE "\([^"]\+\)")/ (\1)/ip' | tr -d '\n' >> $@
	$(if_is_devel_then) echo " (devel)"; else echo; fi >> $@

replace_version = sed -e "s/<OSPRAY_VERSION>/`cat tmp/version`/g" $< > $@

tmp/%.md: %.md tmp/version tmp/links_local.md tmp/images_web.md
	$(replace_version)
	$(PANDOC) $@ tmp/links_local.md tmp/images_web.md -o $@

tmp/links.md: links.md $(wildcard ../.git/HEAD)
	$(if_is_devel_then) \
	  sed -e "s/OSPRay_readme.pdf/OSPRay_readme_devel.pdf/g" $< > $@;\
	  else cp $< $@ ;\
	fi

tmp/links_local.md: tmp/links.md
	sed -e "s@: [a-z]\+\.html#@: #@" $< > $@

tmp/images_web.md: images.md
	sed -e "s/\.fig/.png/" -e "s/vdb_structure\.svg/vdb_structure.png/" -e "s@: @: https://ospray.github.io/images/@" $< > $@

tmp/images_local_png.md: images.md
	sed -e "s@: @: images/@" -e "s/vdb_structure\.svg/vdb_structure.png/" -e "s/\.fig/.png/" $< > $@

tmp/images_local_pdf.md: images.md
	sed -e "s@: @: images/@" -e "s@images/\(.*\)\.fig@tmp/\1.pdf@" -e "s@\.svg@.pdf@" $< > $@


### directories
########################################################################
$(webpages) www/stylesheet.css: | wwwdir
wwwdir:
	@mkdir -p www

$(webimages): | wwwimgdir
wwwimgdir:
	@mkdir -p www/images

wwwscripts: | wwwdir
	if [[ -d ospray-doc/scripts ]] ; then \
	  cp -r ospray-doc/scripts/ www ;\
	fi

$(process_version) tmp/version ../readme.pdf $(tmptexfiles) $(images_fig2pdf) tmp/api.md tmp/api_web.md tmp/changelog.md tmp/compilation.md tmp/compilation_web.md tmp/links.md tmp/links_local.md tmp/images_web.md tmp/images_local_png.md tmp/images_local_pdf.md: | tmpdir
tmpdir:
	@mkdir -p tmp


### spec
########################################################################

ospray-spec.rst: api.md tmp/links_local.md tmp/images_local_png.md
	$(PANDOC) --wrap=none --indented-code-classes=cpp --email-obfuscation=none $+ -t rst | sed -e 's@½@\\frac{1}{2}@g' > $@

ospray-spec.html: ospray-spec.rst
	rst2html $+ $@

### webpages
########################################################################

markdown2web = $(PANDOC) --email-obfuscation=none -f markdown $(filter-out webtemplate.html,$+) --template webtemplate -V select_$(basename $(@F)) -o $@

$(webpages): tmp/links.md tmp/images_local_png.md webtemplate.html

www/%.html:
	$(markdown2web)

www/stylesheet.css: stylesheet.css
	cp $< $@

tmp/api_web.md: api.md tmp/links.md tmp/images_local_png.md
	$(PANDOC) $+ --indented-code-classes=cpp -t markdown-fenced_code_attributes -o $@

tmp/compilation_web.md: compilation.md tmp/links.md
	$(PANDOC) $+ --indented-code-classes=sh -t markdown-fenced_code_attributes -o $@

tmp/changelog.md: filter-sectionnumbers.py ../CHANGELOG.md
	$(PANDOC) --top-level-division=chapter --filter $+ -o $@
	echo -e '\nFor the complete history of changes have a look at the [CHANGELOG](https://github.com/ospray/ospray/blob/master/CHANGELOG.md).' >> $@

www/index.html: teaser.html overview.md tmp/changelog.md
www/tutorials.html: tutorials.md
	$(markdown2web) --indented-code-classes=sh
www/gallery.html: gallery.md
www/downloads.html: tmp/getting_ospray.md tmp/compilation_web.md
www/related_projects.html: related_projects.md
www/displaywall.html: displaywall.md
www/legal.html: legal.md
www/documentation.html: documentation.md tmp/api_web.md
	$(markdown2web) --toc
www/ospray2_porting_guide.html: ospray2_porting_guide.md
	$(markdown2web) --indented-code-classes=cpp


### images
########################################################################

www/images/%.png: %.fig
	fig2dev -L png -S 4 $+ $@

www/images/%.svg: %.svg
	cp $+ $@

tmp/%.pdf: %.fig
	fig2dev -L pdf $+ $@

www/images/%: images/%
	cp $< $@

# try to download images if not linked; images may not be up to date
images/%:
	if [[ -d ospray-doc/images ]] ; then \
	  ln -fs ospray-doc/images ;\
	else \
	  if [[ -x "`which wget 2> /dev/null`" ]] ; then \
	    mkdir -p images ;\
	    wget https://ospray.github.io/$@ -O $@ ;\
	  else \
	    curl https://ospray.github.io/$@ --create-dirs -Lo $@ ;\
	  fi \
	fi

### markdown
########################################################################

# also works around pandoc #5121, should be fixed in v2.5.1
convert_nbsp_width = sed -e 's@\\ @ @g' -e 's/" style="width:/" width="/g' -e "s@^</figure>@\0\n\n@"

tmp/api.md: api.md tmp/links_local.md tmp/images_web.md
	$(PANDOC) $+ --indented-code-classes=cpp -t markdown-fenced_code_attributes -o $@

tmp/compilation.md: compilation.md tmp/links_local.md
	$(PANDOC) $+ --indented-code-classes=sh -t markdown-fenced_code_attributes -o $@

tmp/tutorials.md: tutorials.md tmp/links_local.md tmp/images_web.md
	$(PANDOC) $+ --indented-code-classes=sh -t markdown-fenced_code_attributes -o $@

../README.md: tmp/readme_head.md tmp/overview.md gitter.md tmp/compilation.md tmp/documentation.md tmp/api.md tmp/tutorials.md
	$(PANDOC) --file-scope $+ --markdown-headings=setext -t gfm | $(convert_nbsp_width) > $@

### pdf
########################################################################

markdown2tex = $(PANDOC) --columns=72 --top-level-division=chapter --filter $+ -o $@

tmp/%.tex: filter-latex.py %.md tmp/links_local.md tmp/images_local_pdf.md
	$(markdown2tex)

tmp/api.tex: filter-latex.py api.md tmp/links_local.md tmp/images_local_pdf.md
	$(markdown2tex) --indented-code-classes=cpp

tmp/compilation.tex: filter-latex.py compilation.md tmp/links_local.md tmp/images_local_pdf.md
	$(markdown2tex) --indented-code-classes=sh

tmp/tutorials.tex: filter-latex.py tutorials.md tmp/links_local.md tmp/images_local_pdf.md
	$(markdown2tex) --indented-code-classes=sh

tmp/changelog.tex: filter-latex.py tmp/changelog.md tmp/links_local.md tmp/images_local_pdf.md
	$(markdown2tex)

../readme.pdf: readme.tex $(tmptexfiles) $(wildcard ospray-doc/intel-spec.cls) preamble.tex tmp/version $(pdfimages)
	md5s=0;\
	auxf="tmp/`basename $< tex`aux";\
	newmd5s=`md5sum $$auxf 2> /dev/null`;\
	until [[ $$md5s == $$newmd5s ]]; do \
		md5s=$$newmd5s;\
		xelatex -halt-on-error -output-directory=tmp $< || exit 2;\
		newmd5s=`md5sum $$auxf`;\
	done
	cp tmp/$(basename $<).pdf $@

# crude way to detect broken pdf links
tmp/%.txt: $(tmptexfiles)
	fgrep -h $* $^ | sed -e 's@.*$*{\([^}]*\)}.*@\1@'|sort -u > $@
tmp/brokenlinks.txt: tmp/hypertarget.txt tmp/hyperlink.txt
	-diff -u $^ | grep "^+[^+]" > $@


### clean
########################################################################

clean:
	rm -rf www tmp __pycache__ ospray-spec.html ospray-spec.rst

realclean: clean
	rm -irf images
