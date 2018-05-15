SHELL := /bin/bash

webpages := $(addprefix www/, $(addsuffix .html, index news demos documentation gallery downloads related_projects pvospray displaywall fiu_comparison legal))
process_version := $(addprefix tmp/, $(addsuffix .md, demos getting_ospray readme_head))
tmptexfiles := $(addprefix tmp/, $(addsuffix .tex, overview changelog compilation api mpi scenegraph examples))
images_jpg := $(addprefix images/, $(addsuffix .jpg, exampleViewer $(addprefix camera_, perspective architectual stereo orthographic panoramic) $(addprefix material_, OBJ Principled CarPaint Metal Alloy Glass ThinGlass MetallicPaint Luminous) ColoredWindow rm_volume_ao_shadows rm_volume heptane_bubbly_osp mag_pt rm_volume2 dns_small code GLuRayPage))
images_png := $(addprefix images/, $(addsuffix .png, diffuse_rooms normalmap_frustum tutorial_accumulatedframe tutorial_firstframe))
images_fig := spot_light quad_light hdri_light
images_svg := gitter_badge

images_fig2pdf := $(addprefix tmp/, $(addsuffix .pdf, $(images_fig)))
images_fig2png := $(addprefix images/, $(addsuffix .png, $(images_fig)))
webimages := $(addprefix www/, $(images_jpg) $(images_png) $(images_fig2png) $(addprefix images/, $(addsuffix .svg, $(images_svg))))
pdfimages := $(images_jpg) $(images_png) $(images_fig2pdf)

PANDOC := pandoc
PDMINVERSION := 2000000
PDOK := $(shell expr `$(PANDOC) --version|head -n 1|cut -d' ' -f 2| sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$$/&00/' -e 's/^[0-9]\{5,6\}$$/&00/'` \>= $(PDMINVERSION))
ifneq "$(PDOK)" "1"
  $(error You need at least pandoc v2.0)
endif

all: www doc pdf
www: $(webpages) www/stylesheet.css $(webimages)
doc: ../README.md
pdf: ../readme.pdf

.PHONY: all www doc pdf clean realclean submodule

submodule:
	git submodule update --init --remote

if_is_devel_then = if [[ "${CI_BUILD_REF_NAME}" == "devel" || (-d ../.git && "`git rev-parse --abbrev-ref HEAD`" == "devel") ]] ; then

tmp/version: ../cmake/ospray_options.cmake $(wildcard ../.git/HEAD)
	sed -n $< -e "s/^SET(OSPRAY_VERSION_MAJOR \([0-9]\+\))/\1./ip" | tr -d '\n' > $@
	sed -n $< -e "s/^SET(OSPRAY_VERSION_MINOR \([0-9]\+\))/\1./ip" | tr -d '\n' >> $@
	sed -n $< -e "s/^SET(OSPRAY_VERSION_PATCH \([0-9]\+\))/\1/ip" | tr -d '\n' >> $@
	$(if_is_devel_then) echo " (devel)"; else echo; fi >> $@

replace_version = sed -e "s/<OSPRAY_VERSION>/`cat tmp/version`/g" $< > $@

tmp/%.md: %.md tmp/version
	$(replace_version)

tmp/links.md: links.md
	$(if_is_devel_then) \
	  sed -e "s/OSPRay_readme.pdf/OSPRay_readme_devel.pdf/g" $< > $@;\
	  else cp $< $@ ;\
	fi

tmp/images_web.md: images.md
	sed -e "s/\.fig/.png/" -e "s@: @: https://ospray.github.io/images/@" $< > $@

tmp/images_local_png.md: images.md
	sed -e "s@: @: images/@" -e "s/\.fig/.png/" $< > $@

tmp/images_local_pdf.md: images.md
	sed -e "s@: @: images/@" -e "s@images/\(.*\)\.fig@tmp/\1.pdf@" $< > $@

changelog.md:
	ln -s ../CHANGELOG.md $@


### directories
########################################################################
$(webpages) www/stylesheet.css: | wwwdir
wwwdir:
	@mkdir -p www

$(webimages): | wwwimgdir
wwwimgdir:
	@mkdir -p www/images

$(process_version) tmp/version ../readme.pdf $(tmptexfiles) tmp/api.md tmp/api_html.md tmp/scenegraph_html.md tmp/links.md tmp/images_web.md tmp/images_local_pdf.md tmp/images_local_png.md: | tmpdir
tmpdir:
	@mkdir -p tmp


### webpages
########################################################################

markdown2web = $(PANDOC) --email-obfuscation=none -f markdown $(filter-out webtemplate.html,$+) --template webtemplate -V select_$(basename $(@F)) -o $@

$(webpages): tmp/links.md tmp/images_local_png.md webtemplate.html

www/%.html:
	$(markdown2web)

www/stylesheet.css: stylesheet.css
	cp $< $@

tmp/api_html.md: api.md tmp/links.md tmp/images_local_png.md
	$(PANDOC) $+ --indented-code-classes=cpp -t markdown-fenced_code_attributes -o $@

tmp/scenegraph_html.md: scenegraph.md tmp/links.md tmp/images_local_png.md
	$(PANDOC) $+ --indented-code-classes=cpp -t markdown-fenced_code_attributes -o $@

www/index.html: teaser.html overview.md changelog.md
www/news.html: news.md
www/demos.html: tmp/demos.md
www/gallery.html: gallery.md
www/downloads.html: tmp/getting_ospray.md compilation.md
www/related_projects.html: related_projects.md
www/pvospray.html: pvospray.md
www/displaywall.html: displaywall.md
www/fiu_comparison.html: fiu_comparison.md
www/legal.html: legal.md
www/documentation.html: documentation.md tmp/api_html.md mpi.md tmp/scenegraph_html.md examples.md


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

convert_nbsp_width = sed -e 's@\\ @ @g' -e 's/" style="width:/" width="/g'

tmp/api.md: api.md tmp/links.md tmp/images_web.md
	$(PANDOC) $+ --indented-code-classes=cpp -t markdown-fenced_code_attributes -o $@

../README.md: tmp/readme_head.md overview.md gitter.md compilation.md documentation.md tmp/api.md mpi.md scenegraph.md examples.md tmp/links.md tmp/images_web.md
	$(PANDOC) $+ -t markdown-smart+pipe_tables-simple_tables-multiline_tables-grid_tables-link_attributes | $(convert_nbsp_width) > $@

### pdf
########################################################################

markdown2tex = $(PANDOC) --columns=72 --top-level-division=chapter --filter $+ -o $@

tmp/%.tex: filter-latex.py %.md tmp/links.md tmp/images_local_pdf.md
	$(markdown2tex)

tmp/api.tex: filter-latex.py api.md tmp/links.md tmp/images_local_pdf.md
	$(markdown2tex) --indented-code-classes=cpp

tmp/scenegraph.tex: filter-latex.py scenegraph.md tmp/links.md tmp/images_local_pdf.md
	$(markdown2tex) --indented-code-classes=cpp

../readme.pdf: readme.tex $(tmptexfiles) $(wildcard ospray-doc/intel-spec.cls) preamble.tex tmp/version $(pdfimages)
	md5s=0;\
	auxf="tmp/`basename $< tex`aux";\
	newmd5s=`md5sum $$auxf 2> /dev/null`;\
	until [[ $$md5s == $$newmd5s ]]; do \
		md5s=$$newmd5s;\
		xelatex -output-directory=tmp $<;\
		newmd5s=`md5sum $$auxf`;\
	done
	cp tmp/$(basename $<).pdf $@



### clean
########################################################################

clean:
	rm -rf www tmp changelog.md __pycache__

realclean: clean
	rm -irf images
