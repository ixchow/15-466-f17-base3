.PHONY : all


HOSTNAME := $(shell hostname)

ifeq ($(HOSTNAME), incepchow)
	BLENDER = ~/blender-2.78a-linux-glibc211-x86_64/blender
else
	BLENDER = blender
endif


DIST=../dist

all : \
	$(DIST)/menu.p \
	$(DIST)/pool.pnc \
	$(DIST)/pool.scene \

$(DIST)/menu.p : menu.blend export-meshes.py
	$(BLENDER) --background --python export-meshes.py -- '$<' '$@'

$(DIST)/pool.pnc : pool.blend export-meshes.py
	$(BLENDER) --background --python export-meshes.py -- '$<' '$@'

$(DIST)/pool.scene : pool.blend export-scene.py
	$(BLENDER) --background --python export-scene.py -- '$<' '$@'
