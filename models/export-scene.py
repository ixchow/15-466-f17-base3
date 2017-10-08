#!/usr/bin/env python

#based on 'export-sprites.py' and 'glsprite.py' from TCHOW Rainbow; code used is released into the public domain.

#Note: Script meant to be executed from within blender, as per:
#blender --background --python export-meshes.py

#reads 'island.blend' and writes '../dist/meshes.blob' (meshes) and '../dist/scene.blob' (scene in layer 1)

import sys

import bpy
import struct

#---------------------------------------------------------------------
#Export scene (object positions for every object on layer one)

#(re-open file because we adjusted mesh users in the export above)
bpy.ops.wm.open_mainfile(filepath='island.blend')

#strings chunk will have names
strings = b''
#these map from the *mesh* name of our the written objects to the *object* name they are stored under:
name_begin = dict()
name_end = dict()
for name in to_write:
	mesh_name = bpy.data.objects[name].data.name
	name_begin[mesh_name] = len(strings)
	strings += bytes(name, 'utf8')
	name_end[mesh_name] = len(strings)

#scene chunk will have transforms + indices into strings for name
scene = b''
for obj in bpy.data.objects:
	if obj.layers[0] == False: continue
	if not obj.data.name in name_begin:
		print("WARNING: not writing object '" + obj.name + "' because mesh not written.")
		continue
	scene += struct.pack('I', name_begin[obj.data.name])
	scene += struct.pack('I', name_end[obj.data.name])
	transform = obj.matrix_world.decompose()
	scene += struct.pack('3f', transform[0].x, transform[0].y, transform[0].z)
	scene += struct.pack('4f', transform[1].x, transform[1].y, transform[1].z, transform[1].w)
	scene += struct.pack('3f', transform[2].x, transform[2].y, transform[2].z)

#write the strings chunk and scene chunk to an output blob:
blob = open('../dist/scene.blob', 'wb')
#first chunk: the strings
blob.write(struct.pack('4s',b'str0')) #type
blob.write(struct.pack('I', len(strings))) #length
blob.write(strings)
#second chunk: the scene
blob.write(struct.pack('4s',b'scn0')) #type
blob.write(struct.pack('I', len(scene))) #length
blob.write(scene)

print("Wrote " + str(blob.tell()) + " bytes to scene.blob")

