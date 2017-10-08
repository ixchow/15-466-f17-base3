.PHONY : all


HOSTNAME := $(shell hostname)

ifeq ($(HOSTNAME), incepchow)
	BLENDER = ~/blender-2.78a-linux-glibc211-x86_64/blender
else
	BLENDER = blender
endif


DIST=../dist

all : \
	$(DIST)/menu.p

$(DIST)/menu.p : menu.blend
	$(BLENDER) --background --python export-meshes.py -- '$<' '$@'
