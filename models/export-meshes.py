#!/usr/bin/env python

#based on 'export-sprites.py' and 'glsprite.py' from TCHOW Rainbow; code used is released into the public domain.

#Note: Script meant to be executed from within blender, as per:
#blender --background --python export-meshes.py -- <infile.blend> <outfile.p[n][c][t]>

import sys

args = []
for i in range(0,len(sys.argv)):
	if sys.argv[i] == '--':
		args = sys.argv[i+1:]

if len(args) != 2:
	print("\n\nUsage:\nblender --background --python export-meshes.py -- <infile.blend> <outfile.p[n][c][t]>\nExports the meshes referenced by all objects to a binary blob, indexed by the names of the objects that reference them.\n")
	exit(1)

infile = args[0]
outfile = args[1]

class FileType:
	def __init__(self, magic):
		self.magic = magic
		self.position = (b"p" in magic)
		self.normal = (b"n" in magic)
		self.color = (b"c" in magic)
		self.texcoord = (b"t" in magic)
		self.vertex_bytes = 0
		if self.position: self.vertex_bytes += 3 * 4
		if self.normal: self.vertex_bytes += 3 * 4
		if self.color: self.vertex_bytes += 4
		if self.texcoord: self.vertex_bytes += 2 * 4

filetypes = {
	".p" : FileType(b"p..."),
	".pn" : FileType(b"pn.."),
	".pc" : FileType(b"pc.."),
	".pt" : FileType(b"pt.."),
	".pnc" : FileType(b"pnc."),
	".pnt" : FileType(b"pnt."),
	".pnct" : FileType(b"pnct"),
}

filetype = None
for kv in filetypes.items():
	if outfile.endswith(kv[0]):
		assert(filetype == None)
		filetype = kv[1]

if filetype == None:
	print("ERROR: please name outfile with one of:")
	for k in filetypes.keys():
		print("\t\"" + k + "\"")
	exit(1)

import bpy
import struct

import argparse


bpy.ops.wm.open_mainfile(filepath=infile)

#names of objects whose meshes to write (not actually the names of the meshes):
#to_write = [
#	'House',
#	'Land',
#	'Tree',
#	'Water',
#	'Rock',
#]
to_write = []
for obj in bpy.data.objects:
	if obj.type == 'MESH':
		to_write.append(obj.name)

#data contains vertex and normal data from the meshes:
data = b''

#strings contains the mesh names:
strings = b''

#index gives offsets into the data (and names) for each mesh:
index = b''

vertex_count = 0
for name in to_write:
	print("Writing '" + name + "'...")
	bpy.ops.object.mode_set(mode='OBJECT') #get out of edit mode (just in case)
	assert(name in bpy.data.objects)
	obj = bpy.data.objects[name]

	obj.data = obj.data.copy() #make mesh single user, just in case it is shared with another object the script needs to write later.

	#make sure object is on a visible layer:
	bpy.context.scene.layers = obj.layers
	#select the object and make it the active object:
	bpy.ops.object.select_all(action='DESELECT')
	obj.select = True
	bpy.context.scene.objects.active = obj

	#subdivide object's mesh into triangles:
	bpy.ops.object.mode_set(mode='EDIT')
	bpy.ops.mesh.select_all(action='SELECT')
	bpy.ops.mesh.quads_convert_to_tris(quad_method='BEAUTY', ngon_method='BEAUTY')
	bpy.ops.object.mode_set(mode='OBJECT')

	#compute normals (respecting face smoothing):
	mesh = obj.data
	mesh.calc_normals_split()

	#record mesh name, start position and vertex count in the index:
	name_begin = len(strings)
	strings += bytes(name, "utf8")
	name_end = len(strings)
	index += struct.pack('I', name_begin)
	index += struct.pack('I', name_end)

	index += struct.pack('I', vertex_count)
	index += struct.pack('I', len(mesh.polygons) * 3)

	colors = None
	if filetype.color:
		if len(obj.data.vertex_colors) == 0:
			print("WARNING: trying to export color data, but object '" + name + "' does not have color data; will output 0xffffffff")
		else:
			colors = obj.data.vertex_colors.active.data

	uvs = None
	if filetype.texcoord:
		if len(obj.data.uv_layers) == 0:
			print("WARNING: trying to export texcoord data, but object '" + name + "' does not uv data; will output (0.0, 0.0)")
		else:
			uvs = obj.data.uv_layers.active.data

	#write the mesh:
	for poly in mesh.polygons:
		assert(len(poly.loop_indices) == 3)
		for i in range(0,3):
			assert(mesh.loops[poly.loop_indices[i]].vertex_index == poly.vertices[i])
			loop = mesh.loops[poly.loop_indices[i]]
			vertex = mesh.vertices[loop.vertex_index]
			for x in mesh.vertices[loop.vertex_index].co:
				data += struct.pack('f', x)
			if filetype.normal:
				for x in loop.normal:
					data += struct.pack('f', x)
			if filetype.color:
				if colors != None:
					col = colors[poly.loop_indices[i]].color
					data += struct.pack('BBBB', int(col.r * 255), int(col.g * 255), int(col.b * 255), 255)
				else:
					data += struct.pack('BBBB', 255, 255, 255, 255)
			if filetype.texcoord:
				if uvs != None:
					uv = uvs[poly.loop_indices[i]].uv
					data += struct.pack('ff', uv.x, uv.y)
				else:
					data += struct.pack('ff', 0, 0)

			if filetype.texcoord:
				pass
	vertex_count += len(mesh.polygons) * 3

#check that we wrote as much data as anticipated:
assert(vertex_count * filetype.vertex_bytes == len(data))

#write the data chunk and index chunk to an output blob:
blob = open(outfile, 'wb')
#first chunk: the data
blob.write(struct.pack('4s',filetype.magic)) #type
blob.write(struct.pack('I', len(data))) #length
blob.write(data)
#second chunk: the strings
blob.write(struct.pack('4s',b'str0')) #type
blob.write(struct.pack('I', len(strings))) #length
blob.write(strings)
#third chunk: the index
blob.write(struct.pack('4s',b'idx0')) #type
blob.write(struct.pack('I', len(index))) #length
blob.write(index)

print("Wrote " + str(blob.tell()) + " bytes [== " + str(len(data)+8) + " bytes of data + " + str(len(strings)+8) + " bytes of strings + " + str(len(index)+8) + " bytes of index] to '" + outfile + "'")

